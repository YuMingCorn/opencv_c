# 移植指南

本文說明如何把 `improc_core` 編進樹莓派、Cortex-M 與 AVR 這類目標。專案只提供編譯選項與 CMake 工具鏈檔。不提供模擬器、連結腳本或 semihosting，因為各板子的啟動碼與記憶體配置各不相同，應由使用者的專案負責。

## 兩個函式庫

| 目標 | 內容 | 相依 |
|---|---|---|
| `improc_core` | 色彩、像素、濾波、幾何、Mat、工具函式 | 只用 libc 的 `memcpy`、`abort` 等；不含 stdio、pthread、檔案 IO |
| `improc` | `improc_core` 加上圖檔讀寫（`mat_io.c`、stb） | pthread、stdio、檔案系統 |

裸機目標（`CMAKE_SYSTEM_NAME` 為 `Generic`）預設只建 `improc_core`。

## CMake 選項

| 選項 | 預設 | 作用 |
|---|---|---|
| `IMPROC_CORE_ONLY` | Generic 為 ON，其餘 OFF | 只建 `improc_core`，不找 Threads，也不建 demo、benchmark、測試 |
| `IMPROC_NO_STDIO` | Generic 為 ON，其餘 OFF | 核心不引入 `<stdio.h>`；ASSERT 失敗時呼叫 `abort()` |
| `IMPROC_NO_MALLOC` | OFF | 沒有預設配置器；不編譯 `alloc_mat` 與 `free_mat`，改用 `mat_init` 包裝呼叫端的緩衝區 |
| `IMPROC_ENABLE_FLOAT` | ON | 定義 `IMPROC_HAS_FLOAT=1` 並編譯浮點程式；沒有 FPU 的目標請關閉 |
| `IMPROC_ASSERT_TERSE` | OFF | ASSERT 只傳行號給處理函式（運算式與檔名為 `NULL`），省下字串常數 |
| `IMPROC_WERROR` | OFF | 警告視為錯誤 |
| `IMPROC_KODAK_DIR` | `datasets/kodak` | Kodak 測試資料集位置 |

## 在沒有 malloc 的目標上使用

```c
static uchar src_buf[64 * 48];
static uchar dst_buf[64 * 48];
struct Mat src, dst;
mat_init(&src, 48, 64, CV_8UC1, src_buf, sizeof(src_buf));
mat_init(&dst, 48, 64, CV_8UC1, dst_buf, sizeof(dst_buf));
threshold(&dst, &src, 128, 255, THRESH_BINARY);
```

`mat_init` 不配置記憶體，成功回傳 0。緩衝區必須對齊通道大小。有些濾波函式內部仍會呼叫 `fast_malloc` 取得暫存列；請用 `improc_set_allocator` 提供自己的配置器（例如固定大小的記憶體池）。沒有配置器時 `fast_malloc` 回傳 `NULL`，函式會以 ASSERT 中止。

## 自訂 ASSERT 與配置器

```c
void on_assert(const char *expr, const char *file, int line) {
    /* 記錄後重開機，或 longjmp 回安全點 */
}
improc_set_assert_handler(on_assert);
improc_set_allocator(pool_alloc, pool_free);
```

處理函式若返回，核心仍會依預設動作結束（`exit` 或 `abort`）。更換配置器時，不能有尚未釋放的 `fast_malloc` 區塊。

## 工具鏈與編譯旗標

工具鏈檔在 `cmake/toolchains/`。用法：

```sh
cmake -S . -B build-m4f -G Ninja --toolchain cmake/toolchains/cortex-m4f.cmake -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build-m4f --target improc_core
```

| 目標 | 編譯器 | 旗標 | 浮點 | 驗證方式 |
|---|---|---|---|---|
| 樹莓派 64 位元 | `aarch64-linux-gnu-gcc` | `-march=armv8-a` | ON | 編譯；測試在 `qemu-aarch64` 下執行 |
| 樹莓派 32 位元 | `arm-linux-gnueabihf-gcc` | `-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard` | ON | 只在 CI 編譯；本機沒有編譯器時腳本略過 |
| Cortex-M4F | `arm-none-eabi-gcc` | `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -ffunction-sections -fdata-sections --specs=nano.specs` | ON | 編譯 |
| Cortex-M7 | `arm-none-eabi-gcc` | `-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard` 加上同樣的 section 旗標 | ON | 編譯 |
| Cortex-M0+ | `arm-none-eabi-gcc` | `-mcpu=cortex-m0plus -mthumb` 加上 section 旗標 | OFF | 編譯；另外以 `IMPROC_NO_MALLOC=ON` 編譯一次 |
| Arduino Uno（AVR） | `avr-gcc` | `-mmcu=atmega328p -ffunction-sections -fdata-sections` | OFF | 編譯與 `size`；使用 `IMPROC_NO_MALLOC=ON`、`IMPROC_ASSERT_TERSE=ON` |
| ESP32 | `xtensa-esp32-elf-gcc` | `-mlongcalls -ffunction-sections -fdata-sections` | ON | 只有文件，尚未驗證 |

ESP32 請用 ESP-IDF 元件，把核心原始檔列進 `idf_component_register`：

```cmake
idf_component_register(
    SRCS core/color.c core/pixel.c core/filter.c core/geometry.c
         core/mat.c core/utils.c
    INCLUDE_DIRS core)
```

## 檢查腳本

`tools/check_cross.sh` 對每個已安裝的編譯器：以 `MinSizeRel` 加 `-DIMPROC_WERROR=ON` 設定專案、建出 `improc_core`、印出 `size -t`，再用 `nm -u` 檢查未定義符號。沒有 stdio 的目標不得引用 `printf`、`fprintf`、`puts`、`fputs`、`fopen`、`exit`；沒有 malloc 的目標另外不得引用 `malloc`、`calloc`、`realloc`、`free`；所有目標都不得引用 `pthread_*`。Cortex-M0+ 的建置會加上 `-fstack-usage`，並列出最大的五個堆疊框。

## 大小與暫存記憶體

大小為 `libimproc_core.a` 各目的檔 `.text`、`.data`、`.bss` 的總和（位元組，`MinSizeRel`）。ASSERT 會把 `__FILE__` 與運算式字串放進程式，所以原始碼路徑越長數字越大（差距可達數百位元組）；要縮小請開啟 `IMPROC_ASSERT_TERSE`。數字隨版本改變，整合階段會重新量測。

| 目標 | 編譯器版本 | text | data | bss |
|---|---|---|---|---|
| aarch64 | GCC 15.2.0 | 13304 | 304 | 8 |
| Cortex-M4F | GCC 14.2.1 | 8468 | 8 | 4 |
| Cortex-M7 | GCC 14.2.1 | 8468 | 8 | 4 |
| Cortex-M0+ | GCC 14.2.1 | 8542 | 8 | 4 |
| Cortex-M0+（無 malloc） | GCC 14.2.1 | 8037 | 0 | 12 |
| AVR ATmega328P（無 malloc） | GCC 14.3.0 | 11128 | 0 | 6 |

下表列出各運算向 `fast_malloc` 要求的暫存記憶體（`rows`、`cols` 為影像尺寸，`cn` 為通道數，`n = cols * cn`，`r = ksize / 2`）。

| 運算 | 暫存位元組 |
|---|---|
| `threshold`、`mat_lut`、`convert_scale`、`cvt_color`、`flip` | 0 |
| `box_filter` | `rows * n * 2 + n * 4 + (cols + 2r) * cn` |
| `gaussian_blur` | `rows * n * 2 + (cols + 2r) * cn` |
| `sobel` | `rows * cols * 2 + cols + 2` |
| `resize` | `dst_cols * (2 * sizeof(int) + 2)` |

濾波類運算目前以整張影像大小的 16 位元暫存區運作，256 KB 的 RAM 放不下大影像。之後的工作會改成只保留 `ksize` 列。

## 已知限制

- QEMU 只用來驗證結果正確，它的速度沒有參考價值。
- ESP32 與 armhf 沒有在本機驗證。ESP32 只有文件，armhf 只在 CI 編譯。
- `int` 只有 16 位元的目標（AVR）上，任何可能超過 32767 的中間值都必須使用 `int32_t`。
