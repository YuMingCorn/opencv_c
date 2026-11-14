# opencv_c

以 C11 親手實作基本影像處理的學習與作品集專案。API 可自由調整。

## 建置與驗證

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

效能數字一律來自 Release（`-B build-release -DCMAKE_BUILD_TYPE=Release`）。

CMake 選項：`IMPROC_CORE_ONLY`、`IMPROC_NO_STDIO`、`IMPROC_NO_MALLOC`、`IMPROC_ENABLE_FLOAT`、`IMPROC_ASSERT_TERSE`、`IMPROC_WERROR`、`IMPROC_KODAK_DIR`。意義與預設值見 `docs/porting.md`。提交前用 `-DIMPROC_WERROR=ON` 建置。

交叉編譯檢查：`bash tools/check_cross.sh`（需要時加 `--require-all`）。它對每個已安裝的交叉編譯器建出 `improc_core`、印出大小，並用 `nm -u` 檢查禁用符號。工具鏈檔在 `cmake/toolchains/`。樹莓派 64 位元的測試用 `qemu-aarch64` 執行：`--toolchain cmake/toolchains/aarch64-linux-gnu.cmake`。

Arduino 封裝檢查：`bash tools/check_arduino_src.sh`（Git Bash 加 `HOST_CC=cc HOST_CXX=c++`）。`src/` 的檔案只 include `core/`，新增核心原始檔時要在 `src/` 加對應的包裝檔。

## 支援平台

Windows x64（LLVM-MinGW）、Linux x64（GCC）、macOS arm64（Clang）。程式碼要能在這三者編譯；執行緒使用 pthread，MSVC 不在支援範圍。

邊緣裝置：樹莓派（aarch64、armhf）、Cortex-M0+／M4F／M7、ESP32、AVR（只驗證編譯與大小）。`int` 可能只有 16 位元，任何可能超過 32767 的中間值要用 `int32_t`。

## 模組與相依

兩個函式庫：

- `improc_core`：可攜核心（`color`、`pixel`、`filter`、`morph`、`hist`、`geometry`、`convert_f32`、`mat`、`utils`）。只依賴 libc 與 libm（浮點時），不含 stdio、pthread、檔案 IO。
- `improc`：`improc_core` 加上 `core/mat_io.c`（圖檔讀寫，含 stb 實作）。只有它依賴 pthread 與 `core/stb_*.h`。

演算法全部自行實作。`stb_*.h` 是第三方檔案，維持原樣。`core/improc_internal.h` 只給核心與測試使用（檢查過的大小運算、128 位元輔助函式），不屬於公開 API。

## 程式慣例

- 外部輸入錯誤（檔案不存在、格式不支援）：回傳 `NULL` 或錯誤碼。
- 程式誤用（dst 尺寸或型別不符、索引超界）：`ASSERT` 中止。
- 暫存記憶體配置失敗或大小溢位：同樣 `ASSERT`（每個運算一致）。所有暫存大小都用 `improc_scratch_mul`、`improc_scratch_add` 計算，並用 `improc_scratch_check` 檢查區塊。
- 整數運算：可能超過 `int` 的值用 `int32_t`、`uint32_t` 或 `size_t`；不對可能為負的值右移。
- 識別字與程式碼註解用英文；文件用繁體中文（台灣用語）。

## 測試

- 放在 `tests/`，由 CTest 執行，使用專案內的小型 assert 工具。
- 期望值來自手算的小矩陣，並以寬度 0～65 的掃描與測試內的 naive 參考實作比對。
- SIMD、LUT、thread 版本都與 scalar 版比對，誤差上限寫在測試內。
- 修 bug 時先寫出會失敗的測試。
- Kodak 測試（`test_kodak_<module>`）用 `datasets/kodak` 內的 24 張影像（git 忽略，來源 <https://r0k.us/graphics/kodak/>），把輸出雜湊與 `tests/golden/kodak_<module>.txt` 比對。沒有資料集時為 Skipped。輸出沒有正當原因改變時不要動 golden 檔。
- 與 OpenCV 比對：各 Kodak 測試加 `--dump <目錄>`，再執行 `python3 tools/compare_opencv.py --dump <目錄>`（參考實作在 `tools/opencv_ref/`）。

## Git 流程

每個修正或演算法一個分支。驗證通過後交給使用者審查，取得同意才 commit、push，並透過 PR 合併到 `main`。
