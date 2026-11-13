// Kodak regression harness. Runs every op of kodak_ops[] on the 24 Kodak images
// and compares a 64-bit FNV-1a hash of each result with a golden manifest.
//
//   test_kodak_<module> --dir <kodak dir> --manifest <file> [--update]
//                       [--dump <dir>]
//
// IMPROC_KODAK_DIR in the environment overrides --dir. Exit codes: 0 pass,
// 1 failure, 77 dataset absent (CTest reports Skipped).

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MAKE_DIR(path) mkdir((path), 0777)
#endif

#include "improc.h"
#include "kodak.h"

#ifndef KODAK_MODULE
#define KODAK_MODULE "module"
#endif

#define KODAK_IMAGE_COUNT 24
#define EXIT_SKIP         77
#define MAX_ENTRIES       4096

struct Entry {
    char     image[16];
    char     op[48];
    uint64_t hash;
    int      used;
};

static struct Entry entries[MAX_ENTRIES];
static int          entry_count;

static const char *dump_dir;
static FILE       *values_file;
static int         mismatches;
static int         checked;

static uint64_t fnv_bytes(uint64_t h, const uchar *p, size_t n) {
    for (size_t i = 0; i < n; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static uint64_t fnv_u32_le(uint64_t h, uint32_t v) {
    uchar b[4] = {(uchar)(v & 0xFF), (uchar)((v >> 8) & 0xFF),
                  (uchar)((v >> 16) & 0xFF), (uchar)((v >> 24) & 0xFF)};
    return fnv_bytes(h, b, 4);
}

// rows, cols, type as little-endian u32, then the pixel bytes of every row.
static uint64_t hash_mat(const struct Mat *m) {
    uint64_t h       = 14695981039346656037ULL;
    h                = fnv_u32_le(h, (uint32_t)m->rows);
    h                = fnv_u32_le(h, (uint32_t)m->cols);
    h                = fnv_u32_le(h, (uint32_t)mat_type(m));
    size_t row_bytes = (size_t)m->cols * mat_pixel_size(m);
    for (int y = 0; y < m->rows; y++) {
        h = fnv_bytes(h, mat_row(m, y), row_bytes);
    }
    return h;
}

static int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return 0;
    }
    fclose(f);
    return 1;
}

static void image_name(int index, char *out, size_t size) {
    snprintf(out, size, "kodim%02d", index);
}

static void image_path(const char *dir, const char *name, char *out,
                       size_t size) {
    snprintf(out, size, "%s/%s.png", dir, name);
}

static int load_manifest(const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "Cannot open manifest %s\n", path);
        return -1;
    }
    char line[256];
    while (fgets(line, sizeof line, f) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        struct Entry       e;
        char               image[64], op[128];
        unsigned long long hash = 0;
        if (sscanf(line, "%63s %127s %llx", image, op, &hash) != 3) {
            fprintf(stderr, "Bad manifest line: %s", line);
            fclose(f);
            return -1;
        }
        if (strlen(image) >= sizeof e.image || strlen(op) >= sizeof e.op) {
            fprintf(stderr,
                    "Manifest name too long (image max %zu, op max %zu "
                    "chars): %s",
                    sizeof e.image - 1, sizeof e.op - 1, line);
            fclose(f);
            return -1;
        }
        if (entry_count >= MAX_ENTRIES) {
            fprintf(stderr,
                    "Manifest %s has more than %d entries; raise MAX_ENTRIES "
                    "in kodak_main.c\n",
                    path, MAX_ENTRIES);
            fclose(f);
            return -1;
        }
        memcpy(e.image, image, strlen(image) + 1);
        memcpy(e.op, op, strlen(op) + 1);
        e.hash                 = (uint64_t)hash;
        e.used                 = 0;
        entries[entry_count++] = e;
    }
    fclose(f);
    return 0;
}

static struct Entry *find_entry(const char *image, const char *op) {
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].image, image) == 0 &&
            strcmp(entries[i].op, op) == 0) {
            return &entries[i];
        }
    }
    return NULL;
}

// Verify mode compares with the manifest. Update mode records the hash.
static int update_mode;

static void record(const char *image, const char *op, uint64_t hash) {
    checked++;
    if (update_mode) {
        if (entry_count >= MAX_ENTRIES) {
            fprintf(stderr,
                    "More than %d manifest entries; raise MAX_ENTRIES in "
                    "kodak_main.c\n",
                    MAX_ENTRIES);
            exit(1);
        }
        if (strlen(image) >= sizeof entries[0].image ||
            strlen(op) >= sizeof entries[0].op) {
            fprintf(stderr,
                    "Name too long for the manifest (image max %zu, op max "
                    "%zu chars): %s %s\n",
                    sizeof entries[0].image - 1, sizeof entries[0].op - 1,
                    image, op);
            exit(1);
        }
        struct Entry *e = &entries[entry_count++];
        snprintf(e->image, sizeof e->image, "%s", image);
        snprintf(e->op, sizeof e->op, "%s", op);
        e->hash = hash;
        e->used = 1;
        return;
    }
    struct Entry *e = find_entry(image, op);
    if (e == NULL) {
        printf("MISSING  %s %s (no manifest entry, got %016llx)\n", image, op,
               (unsigned long long)hash);
        mismatches++;
        return;
    }
    e->used = 1;
    if (e->hash != hash) {
        printf("MISMATCH %s %s expected %016llx got %016llx\n", image, op,
               (unsigned long long)e->hash, (unsigned long long)hash);
        mismatches++;
    }
}

static void write_values(const char *image, const char *op,
                         const struct Mat *m) {
    if (values_file == NULL) {
        char path[1024];
        snprintf(path, sizeof path, "%s/values_%s.txt", dump_dir, KODAK_MODULE);
        values_file = fopen(path, "w");
        if (values_file == NULL) {
            fprintf(stderr, "Cannot write %s\n", path);
            return;
        }
    }
    int depth = (int)mat_depth(m);
    int cn    = (int)mat_channel_num(m);
    fprintf(values_file, "%s %s %d %d %d", image, op, m->rows, m->cols,
            (int)mat_type(m));
    for (int y = 0; y < m->rows; y++) {
        const uchar *row = mat_row(m, y);
        for (int i = 0; i < m->cols * cn; i++) {
            switch (depth) {
            case CV_8S:
                fprintf(values_file, " %d", (int)((const signed char *)row)[i]);
                break;
            case CV_16U: {
                uint16_t v;
                memcpy(&v, row + 2 * (size_t)i, sizeof v);
                fprintf(values_file, " %u", (unsigned)v);
                break;
            }
            case CV_16S: {
                int16_t v;
                memcpy(&v, row + 2 * (size_t)i, sizeof v);
                fprintf(values_file, " %d", (int)v);
                break;
            }
            case CV_32S: {
                int32_t v;
                memcpy(&v, row + 4 * (size_t)i, sizeof v);
                fprintf(values_file, " %ld", (long)v);
                break;
            }
            case CV_32F: {
                float v;
                memcpy(&v, row + 4 * (size_t)i, sizeof v);
                fprintf(values_file, " %.9g", (double)v);
                break;
            }
            case CV_64F: {
                double v;
                memcpy(&v, row + 8 * (size_t)i, sizeof v);
                fprintf(values_file, " %.17g", v);
                break;
            }
            default:
                break;
            }
        }
    }
    fprintf(values_file, "\n");
}

static void dump_mat(const char *image, const char *op, const struct Mat *m) {
    if (dump_dir == NULL) {
        return;
    }
    if (mat_depth(m) == CV_8U) {
        char path[1024];
        snprintf(path, sizeof path, "%s/%s_%s.png", dump_dir, image, op);
        if (mat_write(path, m) != 0) {
            fprintf(stderr, "Cannot write %s\n", path);
        }
    } else {
        write_values(image, op, m);
    }
}

static int write_manifest(const char *path) {
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        fprintf(stderr, "Cannot write manifest %s\n", path);
        return -1;
    }
    fprintf(f, "# Kodak golden hashes for module %s.\n", KODAK_MODULE);
    fprintf(f, "# FNV-1a 64 over rows, cols, type (little-endian u32), then "
               "pixel bytes.\n");
    fprintf(f, "# Regenerate with --update. Lines: <image> <op> <hash>.\n");
    for (int i = 0; i < entry_count; i++) {
        fprintf(f, "%s %s %016llx\n", entries[i].image, entries[i].op,
                (unsigned long long)entries[i].hash);
    }
    fclose(f);
    return 0;
}

int main(int argc, char **argv) {
    const char *dir           = NULL;
    const char *manifest_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dir") == 0 && i + 1 < argc) {
            dir = argv[++i];
        } else if (strcmp(argv[i], "--manifest") == 0 && i + 1 < argc) {
            manifest_path = argv[++i];
        } else if (strcmp(argv[i], "--dump") == 0 && i + 1 < argc) {
            dump_dir = argv[++i];
        } else if (strcmp(argv[i], "--update") == 0) {
            update_mode = 1;
        } else {
            fprintf(stderr,
                    "Usage: %s --dir <kodak> --manifest <file> [--update] "
                    "[--dump <dir>]\n",
                    argv[0]);
            return 1;
        }
    }
    const char *env = getenv("IMPROC_KODAK_DIR");
    if (env != NULL && env[0] != '\0') {
        dir = env;
    }
    if (manifest_path == NULL) {
        fprintf(stderr, "--manifest is required\n");
        return 1;
    }

    char path[1024];
    char name[16];
    if (dir == NULL) {
        printf("Skipped: no Kodak directory given\n");
        return EXIT_SKIP;
    }
    image_path(dir, "kodim01", path, sizeof path);
    if (!file_exists(path)) {
        printf("Skipped: %s not found\n", path);
        return EXIT_SKIP;
    }
    int present = 0;
    for (int i = 1; i <= KODAK_IMAGE_COUNT; i++) {
        image_name(i, name, sizeof name);
        image_path(dir, name, path, sizeof path);
        if (file_exists(path)) {
            present++;
        } else {
            printf("FAIL: %s is missing\n", path);
        }
    }
    if (present != KODAK_IMAGE_COUNT) {
        printf("FAIL: only %d of %d Kodak images present in %s\n", present,
               KODAK_IMAGE_COUNT, dir);
        return 1;
    }

    if (!update_mode && load_manifest(manifest_path) != 0) {
        return 1;
    }
    if (dump_dir != NULL) {
        MAKE_DIR(dump_dir);
    }

    for (int i = 1; i <= KODAK_IMAGE_COUNT; i++) {
        image_name(i, name, sizeof name);
        image_path(dir, name, path, sizeof path);
        struct Mat *rgb = alloc_mat_from_file(path);
        if (rgb == NULL || mat_type(rgb) != (size_t)CV_8UC3) {
            printf("FAIL: %s is not a decodable 3-channel image\n", path);
            return 1;
        }
        record(name, "src", hash_mat(rgb));
        dump_mat(name, "src", rgb);
        for (int k = 0; k < kodak_op_count; k++) {
            struct Mat *out = kodak_ops[k].run(rgb);
            if (out == NULL) {
                printf("FAIL     %s %s returned NULL\n", name,
                       kodak_ops[k].name);
                mismatches++;
                checked++;
                continue;
            }
            record(name, kodak_ops[k].name, hash_mat(out));
            dump_mat(name, kodak_ops[k].name, out);
            free_mat(&out);
        }
        free_mat(&rgb);
    }
    if (values_file != NULL) {
        fclose(values_file);
    }

    if (update_mode) {
        if (write_manifest(manifest_path) != 0) {
            return 1;
        }
        printf("Wrote %d entries to %s\n", entry_count, manifest_path);
        return 0;
    }

    int stale = 0;
    for (int i = 0; i < entry_count; i++) {
        if (!entries[i].used) {
            printf("STALE    %s %s (manifest entry never produced)\n",
                   entries[i].image, entries[i].op);
            stale++;
        }
    }
    printf("%d hashes checked, %d mismatches, %d stale entries\n", checked,
           mismatches, stale);
    return (mismatches == 0 && stale == 0) ? 0 : 1;
}
