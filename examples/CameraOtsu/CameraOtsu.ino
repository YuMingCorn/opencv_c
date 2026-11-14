// improc example for the AI-Thinker ESP32-CAM: grabs QVGA (320x240)
// grayscale frames and binarizes them with a 5x5 Gaussian and Otsu's
// threshold. Prints the time of each step, the threshold and the share of
// white pixels.
//
// Arduino IDE: install the "esp32" boards package (Espressif), pick the board
// "AI Thinker ESP32-CAM", and open the Serial Monitor at 115200 baud.
//
// The camera driver delivers grayscale frames directly (PIXFORMAT_GRAYSCALE),
// so no color conversion is needed. The frame buffer is wrapped with mat_init
// and never copied.

#if !defined(ARDUINO_ARCH_ESP32)
#error "CameraOtsu needs an ESP32 board with a camera (AI-Thinker ESP32-CAM)"
#endif

#include <improc.h>

#include "esp_camera.h"

// AI-Thinker ESP32-CAM pin map (the CAMERA_MODEL_AI_THINKER block of the
// CameraWebServer example in the esp32 Arduino core).
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

static struct Mat *blur = NULL;
static struct Mat *bin  = NULL;

static bool start_camera() {
    camera_config_t config = {};
    config.ledc_channel    = LEDC_CHANNEL_0;
    config.ledc_timer      = LEDC_TIMER_0;
    config.pin_d0          = Y2_GPIO_NUM;
    config.pin_d1          = Y3_GPIO_NUM;
    config.pin_d2          = Y4_GPIO_NUM;
    config.pin_d3          = Y5_GPIO_NUM;
    config.pin_d4          = Y6_GPIO_NUM;
    config.pin_d5          = Y7_GPIO_NUM;
    config.pin_d6          = Y8_GPIO_NUM;
    config.pin_d7          = Y9_GPIO_NUM;
    config.pin_xclk        = XCLK_GPIO_NUM;
    config.pin_pclk        = PCLK_GPIO_NUM;
    config.pin_vsync       = VSYNC_GPIO_NUM;
    config.pin_href        = HREF_GPIO_NUM;
    // Older esp32 cores (before about 2.0.5) name these two fields
    // pin_sscb_sda / pin_sscb_scl; if the build fails here, use those names.
    config.pin_sccb_sda    = SIOD_GPIO_NUM;
    config.pin_sccb_scl    = SIOC_GPIO_NUM;
    config.pin_pwdn        = PWDN_GPIO_NUM;
    config.pin_reset       = RESET_GPIO_NUM;
    config.xclk_freq_hz    = 20000000;
    config.pixel_format    = PIXFORMAT_GRAYSCALE;
    config.frame_size      = FRAMESIZE_QVGA;
    config.jpeg_quality    = 12;
    config.fb_count        = 1;
    config.fb_location =
        psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY; // one buffer: LATEST has no effect
    esp_err_t err    = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("camera init failed: 0x%x\n", (unsigned)err);
        return false;
    }
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("improc CameraOtsu example");
    if (!start_camera()) {
        while (true) {
            delay(1000);
        }
    }
}

void loop() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == NULL || fb->format != PIXFORMAT_GRAYSCALE) {
        Serial.println("no grayscale frame");
        if (fb != NULL) {
            esp_camera_fb_return(fb);
        }
        delay(1000);
        return;
    }

    struct Mat frame;
    if (mat_init(&frame, (int)fb->height, (int)fb->width, CV_8UC1, fb->buf,
                 fb->len) != 0) {
        Serial.println("frame buffer does not match its size");
        esp_camera_fb_return(fb);
        delay(1000);
        return;
    }
    if (blur != NULL && (blur->rows != frame.rows || blur->cols != frame.cols)) {
        free_mat(&blur);
        free_mat(&bin);
    }
    if (blur == NULL) {
        // alloc_mat stops with ASSERT when memory runs out.
        blur = alloc_mat(frame.rows, frame.cols, CV_8UC1);
        bin  = alloc_mat(frame.rows, frame.cols, CV_8UC1);
    }

    unsigned long t0 = micros();
    gaussian_blur(blur, &frame, 5);
    unsigned long t1 = micros();
    uchar         t  = otsu_threshold(blur);
    threshold(bin, blur, t, 255, THRESH_BINARY);
    unsigned long t2 = micros();
    esp_camera_fb_return(fb);

    uint32_t white = 0;
    for (int y = 0; y < bin->rows; y++) {
        const uchar *row = mat_row(bin, y);
        for (int x = 0; x < bin->cols; x++) {
            white += row[x] != 0;
        }
    }
    uint32_t total = (uint32_t)bin->rows * (uint32_t)bin->cols;
    Serial.printf("%dx%d  gaussian %lu us  otsu+threshold %lu us  "
                  "threshold %u  white %lu%%\n",
                  bin->cols, bin->rows, t1 - t0, t2 - t1, (unsigned)t,
                  (unsigned long)(white * 100u / total));
    delay(500);
}
