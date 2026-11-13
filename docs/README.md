# 程式架構圖

這些圖對應目前的 scalar 版本。C 沒有繼承，所以「關係圖」畫的是連結相依、`#include` 關係與 `struct Mat` 的資料模型，而不是類別階層。

標示慣例：識別字、型別、算式照程式碼用英文；說明文字用繁體中文。

| 顏色 | 意義 |
|---|---|
| 藍 | 公開 API |
| 綠 | 資料（`Mat`、緩衝區、表） |
| 黃 | 內部 `static` 函式與輔助程式 |
| 紫 | 執行檔、CMake 目標、呼叫端 |
| 灰 | 第三方或系統（stb、libc、檔案） |
| 紅 | `ASSERT` 與錯誤路徑 |

## 1. 模組與建置目標

![模組與建置目標](01-architecture.svg)

左半部是 CMake 目標：`improc` 靜態函式庫收攏 `core/` 內六個模組，`demo`、`bench_gray` 與六個測試執行檔都連結它。`improc.h` 是 umbrella header，只負責 include 其餘六個 header。四個運算模組（color、pixel、filter、geometry）都依賴 `mat` 與 `utils`，只有 `mat.c` 會 include 第三方的 stb。右半部是 `#include` 矩陣，實心點為直接 include，空心圈為經由其他 header 間接取得。

對應檔案：`CMakeLists.txt`、`core/improc.h`。

## 2. struct Mat 資料模型

![struct Mat 資料模型](02-mat-layout.svg)

`flags` 由三個欄位組成：高 16 位是魔術數字 `MAGIC_VAL`，位元 11..3 存「通道數減 1」，位元 2..0 存 depth；低 12 位合起來就是 type。`step[0]` 是一列的位元組數，`step[1]` 是一個像素的位元組數；資料是 interleaved 排列，列尾沒有 padding，所以 `mat_row` 與 `mat_at` 都用同一條位址算式。`struct Mat` 本體用 `malloc`，`size`、`step`、`data` 用 16-byte 對齊的 `fast_malloc`，由 `free_mat` 一次釋放。

對應檔案：`core/mat.h`、`core/mat.c`、`core/utils.c`。

## 3. 運算函式的共同流程

![運算函式的共同流程](03-op-call-flow.svg)

呼叫端先配置 `dst`，運算函式再用 `ASSERT` 檢查前置條件，失敗就中止程式。通過後逐列呼叫帶有 `restrict` 的 `static` row kernel，每個 kernel 只處理一列、不做檢查。`cvt_color` 用一張轉換表挑 kernel，其他運算各自選擇 kernel 的方式見圖中表格。

對應檔案：`core/color.c`、`core/pixel.c`、`core/geometry.c`、`core/utils.h`。

## 4. 可分離濾波

![可分離濾波](04-separable-filter.svg)

`box_filter`、`gaussian_blur`、`sobel` 都分成水平與垂直兩個 pass，中間用 `tmp` 緩衝區銜接。三者的 `tmp` 型別、捨入方式各不相同，邊界一律使用 reflect 101。圖中並排列出三條流程，方便對照。

對應檔案：`core/filter.c`。

## 5. resize 座標對應

![resize 座標對應](05-resize-mapping.svg)

`resize` 使用半像素中心的座標對應公式。只有 x 方向預先建表，因為每一列都會重複用到同一組 x 座標；y 方向每列只算一次。雙線性內插的權重是 Q8 定點數，最後一次右移 16 位並四捨五入。

對應檔案：`core/geometry.c`。

## 6. demo 流程與圖檔 I/O

![demo 流程與圖檔 I/O](06-demo-pipeline.svg)

`demo` 讀入圖檔（或產生合成圖）、轉成 RGB，再依序做 13 項運算並各存成一張 PNG。部分 `Mat` 會重複使用。圖檔讀寫都經由 `core/mat.c` 內的 stb；失敗時回傳 `NULL` 或 `-1`，`demo` 只累計失敗次數。

對應檔案：`main.c`、`core/mat.c`。

## 維護方式

SVG 為手寫。程式改動時，要一併更新下列圖：

| 改動 | 要更新的圖 |
|---|---|
| 改 `flags` 或 type 巨集 | 圖 2 |
| 新增運算函式 | 圖 1、3、6 |
| 改濾波演算法 | 圖 4 |
| 改 `resize` | 圖 5 |
