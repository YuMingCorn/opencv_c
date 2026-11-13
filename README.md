# OpenCV C implementation

以 C11 親手實作基本影像處理。目前完成第一階段的 scalar 版本，支援 8 位元的 1、3、4 通道影像，以 RGB 為主。

## 功能

| 模組 | 函式 | 說明 |
|---|---|---|
| `color.h` | `cvt_color` | GRAY、RGB、BGR、RGBA、BGRA 互轉；灰階用 `(77R + 150G + 29B + 128) >> 8` |
| `pixel.h` | `threshold` | BINARY、BINARY_INV、TRUNC、TOZERO、TOZERO_INV |
| `pixel.h` | `mat_lut` | 256 項查表 |
| `pixel.h` | `convert_scale`、`convert_scale_abs` | `src * alpha_q8 / 256 + beta`，結果飽和到 0～255 |
| `filter.h` | `box_filter`、`gaussian_blur`、`sobel` | 邊界一律 BORDER_REFLECT_101；Sobel 輸入灰階、輸出 `CV_16SC1` |
| `geometry.h` | `flip`、`resize` | resize 支援 nearest 與 bilinear，採半像素中心 |
| `mat.h` | `alloc_mat_from_file`、`mat_write` | 讀寫 PNG 與 JPG，依副檔名選格式 |

全部使用整數定點運算，所以各平台的輸出逐 byte 相同。

```c
struct Mat *src  = alloc_mat_from_file("input.png");   // 讀入後為 RGB 順序
struct Mat *gray = alloc_mat(src->rows, src->cols, CV_8UC1);
cvt_color(gray, src, COLOR_RGB2GRAY);
mat_write("gray.png", gray);
free_mat(&gray);
free_mat(&src);
```

## 建置與測試

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## 執行

```sh
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release

# 不帶參數時使用內建的合成測試圖；對每種運算各輸出一張 <prefix><name>.png（預設 prefix 為 out_）
./build-release/demo [input-image] [output-prefix]

# 量測 RGB 轉灰階的耗時
./build-release/bench_gray
```

## 架構圖

模組關係、`struct Mat` 資料模型與各演算法流程的圖解見 [docs/README.md](docs/README.md)。
