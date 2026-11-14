# Kodak 測試與 OpenCV 比對結果

本文記錄在 Kodak 24 張影像上，每個運算與 OpenCV 的最大差異。資料集來源與下載方式見 [README.md](../README.md)。

## 做法

1. 每個模組各有一支 `test_kodak_<module>`。它對 24 張影像各跑一遍運算，計算輸出的 FNV-1a 雜湊，與 `tests/golden/kodak_<module>.txt` 比對。同一份雜湊清單在 Windows（LLVM-MinGW）、WSL（GCC x86-64）與 `qemu-aarch64` 下都通過，所以各平台的輸出逐 byte 相同。
2. 加上 `--dump <目錄>` 時，測試把每個輸出存成 `<影像>_<運算>.png`。六個模組倒進同一個目錄。
3. `tools/compare_opencv.py --dump <目錄>` 讀入這些 PNG。每個運算用我們自己輸出的輸入餵給 OpenCV（例如 `cv2.Sobel` 的輸入是我們輸出的灰階圖），所以某一步的差異不會傳到下一步。最大差異超過容許誤差就以非零狀態結束。

```sh
for m in phase1 morph hist color2 camera area f32; do
  build/test_kodak_$m --dir datasets/kodak --manifest tests/golden/kodak_$m.txt --dump dump
done
python3 tools/compare_opencv.py --dump dump
```

`f32`（`CV_8U` 轉 `CV_32F` 再轉回的恆等性）沒有對應的 OpenCV 運算，只驗證雜湊，不在比對表內。

## 結果

OpenCV 4.10.0，24 張影像，38 個運算，全部在容許誤差內。「差異像素」是所有影像的樣本中與 OpenCV 不同的比例。

| 運算 | 模組 | 最大差異 | 容許 | 差異像素 % | 對照的 OpenCV 呼叫 |
|---|---|---|---|---|---|
| `gray` | phase1 | 1 | 1 | 3.6653 | `COLOR_RGB2GRAY` |
| `bgr` | phase1 | 0 | 0 | 0 | `COLOR_RGB2BGR` |
| `rgba` | phase1 | 0 | 0 | 0 | `COLOR_RGB2RGBA` |
| `thresh128` | phase1 | 0 | 0 | 0 | `THRESH_BINARY` |
| `invert` | phase1 | 0 | 0 | 0 | `LUT` |
| `bright_384_10` | phase1 | 1 | 1 | 21.8100 | `convertScaleAbs(1.5, 10)` |
| `box5` | phase1 | 0 | 0 | 0 | `cv2.blur` 5x5 |
| `gauss3`、`gauss5`、`gauss7` | phase1 | 0 | 0 | 0 | `GaussianBlur` |
| `sobel_x_abs`、`sobel_y_abs` | phase1 | 0 | 0 | 0 | `Sobel`（`CV_16S`，`ksize` 3） |
| `flip_h`、`flip_v`、`flip_both` | phase1 | 0 | 0 | 0 | `flip` |
| `half_linear` | phase1 | 0 | 1 | 0 | `INTER_LINEAR` |
| `nonint_linear` | phase1 | 1 | 1 | 12.2109 | `INTER_LINEAR` |
| `double_nearest` | phase1 | 0 | 0 | 0 | `INTER_NEAREST_EXACT` |
| `erode3` | morph | 0 | 0 | 0 | `cv2.erode` 3x3 |
| `dilate3` | morph | 0 | 0 | 0 | `cv2.dilate` 3x3 |
| `open5` | morph | 0 | 0 | 0 | `morphologyEx` OPEN 5x5 |
| `close5` | morph | 0 | 0 | 0 | `morphologyEx` CLOSE 5x5 |
| `gray_erode7` | morph | 0 | 0 | 0 | `cv2.erode` 7x7 |
| `equalize` | hist | 0 | 1 | 0 | `equalizeHist` |
| `otsu_binary` | hist | 0 | 0 | 0 | `THRESH_OTSU`（同時比對直方圖與 Otsu 門檻值） |
| `rgb2hsv` | color2 | 0 | 0 | 0 | `COLOR_RGB2HSV` |
| `hsv2rgb_roundtrip` | color2 | 1 | 1 | 30.9841 | `COLOR_HSV2RGB` |
| `rgb2yuv` | color2 | 1 | 1 | 10.6839 | `COLOR_RGB2YCrCb`（色度通道對調） |
| `yuv2rgb_roundtrip` | color2 | 1 | 1 | 0.3657 | `COLOR_YCrCb2RGB`（色度通道對調） |
| `bgr565_rgb` | camera | 0 | 0 | 0 | `COLOR_BGR5652RGB` |
| `bgr565be_gray` | camera | 1 | 1 | 5.7574 | `COLOR_BGR5652GRAY`（我們用 77/150/29 的灰階公式） |
| `yuyv_rgb` | camera | 0 | 0 | 0 | `COLOR_YUV2RGB_YUYV` |
| `yuyv_gray` | camera | 0 | 0 | 0 | `COLOR_YUV2GRAY_YUYV` |
| `area_half` | area | 0 | 0 | 0 | `INTER_AREA` |
| `area_third` | area | 1 | 1 | 0.0469 | `INTER_AREA` |
| `area_quarter` | area | 1 | 1 | 3.0672 | `INTER_AREA` |
| `area_nonint` | area | 1 | 1 | 0.0009 | `INTER_AREA` |
| `area_up_1_5` | area | 0 | 1 | 0 | `INTER_AREA` |

## 解讀

- 逐 byte 相同（最大差異 0）的運算：色彩順序轉換、查表、濾波（box、gauss、Sobel）、翻轉、整數倍的 nearest、形態學、直方圖等化、Otsu、RGB 轉 HSV、半倍縮小的 `INTER_AREA`。`equalize`、`half_linear`、`area_up_1_5` 的容許誤差是 1，實測也是 0。
- 差 1 個階的運算都是定點捨入與 OpenCV 的捨入方式不同：灰階、`convertScaleAbs`、非整數倍的雙線性縮放、HSV 轉回 RGB、YUV、三分之一與四分之一的 `INTER_AREA`。差異都在容許範圍內，沒有放寬任何容許誤差。
- 相機格式（camera 模組）的輸入不是真的相機畫面：測試把 Kodak 的 RGB 打包成 RGB565 或 YUYV 位元組，再交給解碼函式。`tools/opencv_ref/camera.py` 用同樣的方式打包，再交給 OpenCV。RGB565 與 YUYV 轉 RGB 和 OpenCV 逐 byte 相同；RGB565 轉灰階沿用本專案的灰階公式，所以和 OpenCV 差 1。
- 每個運算的容許誤差寫在 `tools/opencv_ref/*.py`。
