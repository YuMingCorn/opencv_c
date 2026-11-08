# opencv_c

以 C11 親手實作基本影像處理的學習與作品集專案。API 可自由調整。

## 建置與驗證

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

效能數字一律來自 Release（`-B build-release -DCMAKE_BUILD_TYPE=Release`）。

## 支援平台

Windows x64（LLVM-MinGW）、Linux x64（GCC）、macOS arm64（Clang）。程式碼要能在這三者編譯；執行緒使用 pthread，MSVC 不在支援範圍。

## 相依

正式程式只依賴 libc、pthread、libm，以及 `core/stb_*.h`（僅用於圖檔編解碼）。演算法全部自行實作。`stb_*.h` 是第三方檔案，維持原樣。

## 程式慣例

- 外部輸入錯誤（檔案不存在、格式不支援）：回傳 `NULL` 或錯誤碼。
- 程式誤用（dst 尺寸或型別不符、索引超界）：`ASSERT` 中止。
- 識別字與程式碼註解用英文；文件用繁體中文（台灣用語）。

## 測試

- 放在 `tests/`，由 CTest 執行，使用專案內的小型 assert 工具。
- 期望值來自手算的小矩陣。
- SIMD、LUT、thread 版本都與 scalar 版比對，誤差上限寫在測試內。
- 修 bug 時先寫出會失敗的測試。

## Git 流程

每個修正或演算法一個分支。驗證通過後交給使用者審查，取得同意才 commit、push，並透過 PR 合併到 `main`。
