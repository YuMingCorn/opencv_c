# 移植指南

本文說明如何把 `improc_core` 編進樹莓派、Cortex-M 與 AVR 這類目標。專案只提供編譯選項與 CMake 工具鏈檔。不提供模擬器、連結腳本或 semihosting，因為各板子的啟動碼與記憶體配置各不相同，應由使用者的專案負責。

## 兩個函式庫

| 目標 | 內容 | 相依 |
|---|---|---|
| `improc_core` | 色彩（含 HSV、YUV）、像素、濾波、形態學、直方圖、幾何（含 `INTER_AREA`）、`CV_32F` 轉換（`convert_to`）、Mat、工具函式 | 只用 libc 的 `memcpy`、`abort` 等；不含 stdio、pthread、檔案 IO |
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

## 暫存記憶體不足的處理方式

所有需要暫存記憶體的運算（濾波、形態學、`resize`、`equalize_hist`、`otsu_threshold`）採同一個規則：向 `fast_malloc` 要不到記憶體，或暫存大小的計算超出 `size_t`（AVR 為 16 位元，Cortex-M 為 32 位元），就以 `ASSERT` 中止。大小計算全部經過溢位檢查（`core/improc_internal.h` 的 `improc_scratch_mul`、`improc_scratch_add`），所以溢位不會變成太小的緩衝區。ASSERT 之前會先釋放已經取得的區塊，處理函式即使 `longjmp` 也不會洩漏。

嵌入式使用者若不能接受直接中止，請用 `improc_set_assert_handler` 安裝處理函式，在裡面重開機或 `longjmp` 回安全點。`calc_hist` 與 `otsu_threshold_hist` 使用呼叫端提供的直方圖，不需要暫存記憶體，沒有配置器也能執行。`mat_create` 與圖檔讀寫則回傳 `NULL`，因為這些輸入錯誤是預期內的狀況。

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
| ESP32 | `xtensa-esp32-elf-gcc` | `-mlongcalls -ffunction-sections -fdata-sections`（`cmake/toolchains/esp32.cmake`） | ON | 編譯與 `size`；編譯器取自 esp32 Arduino core（`~/.arduino15/packages/esp32/tools/esp-x32/<版本>/bin`），放進 `PATH` 後由 `check_cross.sh` 檢查 |

ESP32 請用 ESP-IDF 元件，把核心原始檔列進 `idf_component_register`：

```cmake
idf_component_register(
    SRCS core/color.c core/pixel.c core/filter.c core/geometry.c
         core/mat.c core/utils.c core/morph.c core/hist.c
         core/convert_f32.c
    INCLUDE_DIRS core)
```

`convert_f32.c` 需要 `IMPROC_HAS_FLOAT=1`（ESP32 有單精度 FPU）。沒有 FPU 的目標請不要列入，並定義 `IMPROC_HAS_FLOAT=0`。

## 檢查腳本

`tools/check_cross.sh` 對每個已安裝的編譯器：以 `MinSizeRel` 加 `-DIMPROC_WERROR=ON` 設定專案、建出 `improc_core`、印出 `size -t`，再用 `nm -u` 檢查未定義符號。沒有 stdio 的目標不得引用 `printf`、`fprintf`、`puts`、`fputs`、`fopen`、`exit`；沒有 malloc 的目標另外不得引用 `malloc`、`calloc`、`realloc`、`free`；所有目標都不得引用 `pthread_*`。Cortex-M0+ 的建置會加上 `-fstack-usage`，並列出最大的五個堆疊框。

Arduino 與 PlatformIO 的建置不經過 CMake，設定改由 `src/improc_config.h` 提供，用法見 [README.md](../README.md) 的「Arduino 與 PlatformIO」。`tools/check_arduino_src.sh` 檢查這個結構，它有自己的目標清單（Cortex-M4F、Cortex-M0+、AVR、ESP32 與主機），沒安裝的編譯器會略過。

## 大小與暫存記憶體

大小為 `libimproc_core.a` 各目的檔 `.text`、`.data`、`.bss` 的總和（位元組，`MinSizeRel`），由合併後的完整程式樹（含形態學、直方圖、HSV／YUV、`INTER_AREA`；有 FPU 的目標另含 `convert_to`）執行 `bash tools/check_cross.sh` 取得。ASSERT 會把 `__FILE__` 與運算式字串放進程式，所以原始碼路徑越長數字越大（差距可達數百位元組）；要縮小請開啟 `IMPROC_ASSERT_TERSE`。這是整個函式庫的總和；連結時加 `--gc-sections` 會丟掉沒用到的函式與表格。

| 目標 | 編譯器版本 | text | data | bss |
|---|---|---|---|---|
| aarch64 | GCC 15.2.0 | 24332 | 496 | 8 |
| Cortex-M4F | GCC 14.2.1 | 16648 | 8 | 4 |
| Cortex-M7 | GCC 14.2.1 | 16642 | 8 | 4 |
| Cortex-M0+ | GCC 14.2.1 | 16307 | 8 | 4 |
| Cortex-M0+（無 malloc） | GCC 14.2.1 | 15702 | 0 | 12 |
| AVR ATmega328P（無 malloc） | GCC 14.3.0 | 27472 | 0 | 6 |

armhf 本機沒有編譯器，未量測。Cortex-M0+ 與 AVR 關閉浮點，所以不含 `convert_to`。

ESP32（`xtensa-esp32-elf-gcc` 14.2.0，esp32 Arduino core 3.3.12 附帶）：text 18913、data 8、bss 4。以 arduino-cli 編譯範例的結果：`Synthetic` 在 ESP32-CAM 上程式 292 KB、全域變數 22 KB；`CameraOtsu` 程式 377 KB、全域變數 34 KB（大部分是 Arduino core 與相機驅動）。Arduino Uno 編譯 `Synthetic` 會因 RAM 不足而失敗，因為 `cvt_color` 的轉換表連帶引入 HSV 的 2 KB 除法表。

### 堆疊用量

Cortex-M0+（`-fstack-usage`）最大的五個堆疊框：

| 函式 | 位元組 |
|---|---|
| `convert_scale`（`pixel.c`） | 288 |
| `resize`（`geometry.c`） | 248 |
| `otsu_threshold_hist`（`hist.c`） | 208 |
| `gaussian_blur`（`filter.c`） | 128 |
| `morph`（`morph.c`） | 112 |

### 暫存記憶體

下表列出各運算向 `fast_malloc` 要求的暫存記憶體（`rows`、`cols` 為影像尺寸，`cn` 為通道數，`n = cols * cn`，`r = ksize / 2`，`ksize'` 為 `min(ksize, rows)`，也就是環形緩衝區的列數）。所有大小的計算都經過溢位檢查。

| 運算 | 暫存位元組 |
|---|---|
| `threshold`、`mat_lut`、`convert_scale`、`cvt_color`、`flip`、`convert_to` | 0 |
| `calc_hist`、`otsu_threshold_hist` | 0（直方圖由呼叫端提供） |
| `box_filter` | `ksize' * n * 2 + n * 4 + (cols + 2r) * cn` |
| `gaussian_blur` | `ksize' * n * 2 + (cols + 2r) * cn` |
| `sobel` | `min(3, rows) * cols * 2 + cols + 2` |
| `erode`、`dilate`、`morph_open`、`morph_close` | `n`（一列；`morph_open`／`morph_close` 另需呼叫端提供一張同尺寸的 `tmp`） |
| `equalize_hist` | 1280（直方圖 1024 加查表 256） |
| `otsu_threshold` | 1024 |
| `resize`（`INTER_NEAREST`、`INTER_LINEAR`） | `dst_cols * (2 * sizeof(int) + 2)` |
| `resize`（`INTER_AREA`） | `2 * dst_cols * cn * 4` |

濾波類運算只保留 `ksize` 列的水平 pass 結果（環形緩衝區），所以暫存記憶體與影像高度無關，只與列寬和 `ksize` 有關。`INTER_AREA` 另外限制來源影像不超過 2^24 個像素，直方圖函式也一樣。

### AVR 的 RAM

HSV 轉換用兩張 `uint32_t[256]` 的除法表（共 2 KB）。AVR 沒有 `PROGMEM` 時，`const` 表格會被連結器放進 `.data`，要佔同樣大小的 RAM 與 flash 初值；上表的 `size` 把它們算在 `text`。ATmega328P 只有 2 KB RAM，用不起 HSV。不呼叫 HSV 轉換時，加 `--gc-sections` 會把表格丟掉。

## 與 OpenCV 的差異

Kodak 24 張影像、34 個運算與 OpenCV 4.10.0 的逐運算最大差異見 [kodak-results.md](kodak-results.md)。

## 已知限制

- QEMU 只用來驗證結果正確，它的速度沒有參考價值。
- armhf 沒有在本機驗證，只在 CI 編譯。
- `int` 只有 16 位元的目標（AVR）上，任何可能超過 32767 的中間值都必須使用 `int32_t`。
- 測試 `test_io` 在 UBSan 下會報告 `stb_image_write.h` 內的左移溢位（`left shift of ... by 8 places cannot be represented in type 'int'`）。那是第三方檔案，維持原樣，屬於預期內的訊息；其餘測試在 ASan 與 UBSan 下沒有任何報告。
- 暫存記憶體不足與大小溢位一律以 `ASSERT` 中止，見上文「暫存記憶體不足的處理方式」。
