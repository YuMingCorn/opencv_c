# OpenCV C implementation

以 C11 親手實作基本影像處理。支援 8 位元的 1、3、4 通道影像，以 RGB 為主；影像運算全部以整數定點運算完成（只有選用的 `convert_to` 用單精度浮點，且不使用 FMA），所以各平台的輸出逐 byte 相同。程式可編進樹莓派、Cortex-M 與 AVR 這類邊緣裝置（見下方「邊緣裝置建置」）。

## 功能

| 模組 | 函式 | 說明 |
|---|---|---|
| `color.h` | `cvt_color` | GRAY、RGB、BGR、RGBA、BGRA 互轉；灰階用 `(77R + 150G + 29B + 128) >> 8` |
| `color.h` | `cvt_color`（HSV、YUV） | RGB／BGR 與 HSV（H 0～179）互轉；RGB／BGR 與 YUV（JFIF 全範圍 BT.601）互轉 |
| `pixel.h` | `threshold` | BINARY、BINARY_INV、TRUNC、TOZERO、TOZERO_INV |
| `pixel.h` | `mat_lut` | 256 項查表 |
| `pixel.h` | `convert_scale`、`convert_scale_abs` | `src * alpha_q8 / 256 + beta`，結果飽和到 0～255 |
| `filter.h` | `box_filter`、`gaussian_blur`、`sobel` | 邊界一律 BORDER_REFLECT_101；以逐列滾動的環形緩衝區運作，暫存記憶體只與 `ksize` 和列寬有關；Sobel 輸入灰階、輸出 `CV_16SC1` |
| `morph.h` | `erode`、`dilate`、`morph_open`、`morph_close` | 矩形結構元素，`ksize` 為 1～255 的奇數；視窗超出影像的部分不計入 |
| `hist.h` | `calc_hist`、`equalize_hist` | 256 階直方圖與直方圖等化 |
| `hist.h` | `otsu_threshold`、`otsu_threshold_hist` | Otsu 門檻；判準以精確的整數與分數比較，沒有浮點捨入造成的平手 |
| `geometry.h` | `flip` | 水平、垂直、雙向翻轉 |
| `geometry.h` | `resize` | `INTER_NEAREST`、`INTER_LINEAR`（半像素中心）與 `INTER_AREA`（整數面積權重，放大縮小皆可） |
| `convert_f32.h` | `convert_to` | `CV_8UCn` 與 `CV_32FCn` 互轉並套用 `alpha`、`beta`；需要 `IMPROC_ENABLE_FLOAT` |
| `mat.h` | `alloc_mat`、`mat_init` | `alloc_mat` 從堆積配置；`mat_init` 包裝呼叫端的緩衝區，不配置記憶體 |
| `mat_io.h` | `alloc_mat_from_file`、`mat_write` | 讀寫 PNG 與 JPG，依副檔名選格式（只在 `improc` 函式庫，不在 `improc_core`） |

```c
struct Mat *src  = alloc_mat_from_file("input.png");   // 讀入後為 RGB 順序
struct Mat *gray = alloc_mat(src->rows, src->cols, CV_8UC1);
struct Mat *bin  = alloc_mat(src->rows, src->cols, CV_8UC1);
cvt_color(gray, src, COLOR_RGB2GRAY);
threshold(bin, gray, otsu_threshold(gray), 255, THRESH_BINARY);
mat_write("binary.png", bin);
free_mat(&bin);
free_mat(&gray);
free_mat(&src);
```

## 建置與測試

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

常用 CMake 選項（完整說明見 [docs/porting.md](docs/porting.md)）：

| 選項 | 預設 | 作用 |
|---|---|---|
| `IMPROC_CORE_ONLY` | 裸機為 ON | 只建 `improc_core`，不建圖檔 IO、demo、benchmark 與測試 |
| `IMPROC_NO_STDIO` | 裸機為 ON | 核心不引入 `<stdio.h>` |
| `IMPROC_NO_MALLOC` | OFF | 沒有預設配置器，改用 `mat_init` |
| `IMPROC_ENABLE_FLOAT` | ON | 編譯 `convert_to` 等浮點程式 |
| `IMPROC_ASSERT_TERSE` | OFF | ASSERT 只傳行號，省下字串常數 |
| `IMPROC_WERROR` | OFF | 警告視為錯誤 |
| `IMPROC_KODAK_DIR` | `datasets/kodak` | Kodak 資料集位置 |

### Kodak 測試

`test_kodak_*` 用 24 張 Kodak 影像跑每個運算，把輸出的 FNV-1a 雜湊與 `tests/golden/kodak_*.txt` 比對，確認各平台逐 byte 相同。資料集不在版本庫內（`datasets/` 已被 git 忽略）。請自行下載 24 張 `kodim01.png`～`kodim24.png` 放進 `datasets/kodak`，來源為 <https://r0k.us/graphics/kodak/>。沒有資料集時這些測試會顯示為 Skipped，其餘測試照常執行。也可以用環境變數 `IMPROC_KODAK_DIR` 或 CMake 選項 `-DIMPROC_KODAK_DIR=<目錄>` 指定其他位置。

### 與 OpenCV 比對

比對腳本需要 Python 3、`opencv-python` 與 `numpy`。先用各測試的 `--dump` 輸出影像，再讓腳本逐運算算出與 OpenCV 的最大差異：

```sh
for m in phase1 morph hist color2 area f32; do
  build/test_kodak_$m --dir datasets/kodak --manifest tests/golden/kodak_$m.txt --dump dump
done
python3 tools/compare_opencv.py --dump dump
```

每個運算有各自的容許誤差，超過就以非零狀態結束。結果與解讀見 [docs/kodak-results.md](docs/kodak-results.md)。

## 邊緣裝置建置

`improc_core` 只依賴 libc 的 `memcpy`、`abort` 這類函式，可用 `cmake/toolchains/` 內的工具鏈檔交叉編譯：

```sh
cmake -S . -B build-m4f -G Ninja --toolchain cmake/toolchains/cortex-m4f.cmake -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build-m4f --target improc_core
bash tools/check_cross.sh      # 對所有已安裝的編譯器編譯並檢查未定義符號
```

支援樹莓派（aarch64、armhf）、Cortex-M0+／M4F／M7 與 AVR（只驗證編譯與大小）。旗標表、函式庫大小、各運算的暫存記憶體與沒有 malloc 時的用法見 [docs/porting.md](docs/porting.md)。

## Arduino 與 PlatformIO

這個 repo 同時也是一個 Arduino 函式庫，名稱為 `improc`，只包含可攜核心，不包含圖檔讀寫。

- **Arduino IDE：** 在 GitHub 下載 ZIP，再選「草稿碼 → 匯入程式庫 → 加入 .ZIP 程式庫」。
- **PlatformIO：** 在 `platformio.ini` 加上 `lib_deps = https://github.com/YuMingCorn/opencv_c.git`，要指定分支時在網址後面加 `#<分支>`。

在程式中寫 `#include <improc.h>` 即可使用，C 與 C++（`.ino`）都可以。範例有兩個：

| 範例 | 板子 | 內容 |
|---|---|---|
| `examples/Synthetic` | 任何 RAM 約 128 KB 以上的板子（ESP32、ESP32-S3、Cortex-M） | 用內建的合成圖跑「灰階 → Gaussian 5x5 → Otsu」，印出每一步的時間與雜湊。雜湊和 PC 相同時印出 `PASS`，代表這塊板子的輸出和 PC 逐 byte 相同 |
| `examples/CameraOtsu` | AI-Thinker ESP32-CAM | 從相機抓 QVGA 灰階畫面，做 Gaussian 與 Otsu 二值化，印出時間、門檻值與白色像素比例 |

> 目前狀態：`src/` 已用 Cortex-M、AVR 與主機的編譯器檢查過，Synthetic 的流程也在主機上以 C++ 執行並比對雜湊；但兩個範例都還沒用 Arduino IDE、arduino-cli 或 PlatformIO 實際編譯，`CameraOtsu` 也還沒在 ESP32 上編譯或執行。

CMake 用 `-D` 旗標設定的選項，在 Arduino 上改由 `src/improc_config.h` 提供預設值：一律定義 `IMPROC_CORE_ONLY`；AVR 關閉浮點，並開啟 `IMPROC_NO_STDIO` 與 `IMPROC_ASSERT_TERSE`；其他架構開啟浮點。要改設定時，在建置旗標中定義同名巨集（例如 PlatformIO 的 `build_flags = -DIMPROC_ASSERT_TERSE=1`）。`src/` 內的 `.c` 檔只 include `core/` 的原始檔，所以 Arduino 與 CMake 編譯的是同一份程式碼。

沒有 arduino-cli 時，可用 `bash tools/check_arduino_src.sh` 檢查這個結構。它先確認 `core/` 的每個核心原始檔在 `src/` 都有包裝檔，再用清單內已安裝的編譯器（Cortex-M4F、Cortex-M0+、AVR、ESP32 與主機）在不加任何 `-D` 旗標的情況下編譯 `src/`，再用主機的 C++ 編譯器執行 Synthetic 範例的流程並比對雜湊；在有 FPU 的目標上，另外確認浮點程式沒有被合併成 FMA 指令。用 Git Bash 時請加 `HOST_CC=cc HOST_CXX=c++`。

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

## 授權

MIT，見 [LICENSE](LICENSE)。`core/stb_image.h` 與 `core/stb_image_write.h` 是第三方檔案，沿用各自檔內的授權（只用於 `improc` 的圖檔讀寫，不在 Arduino 函式庫內）。
