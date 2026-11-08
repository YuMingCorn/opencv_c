# OpenCV C implementation

## Color conversions

### Orignial implementation

```cpp
cvt_color(gray_mat, src, COLOR_RGB2GRAY);
```

### SIMD implementation

```cpp
cvt_color(gray_mat1, src, COLOR_RGB2GRAY_INTRIN);
```

### 執行

```sh
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release

# 不帶參數時使用內建的合成測試圖，輸出 gray.jpg
./build-release/demo [input-image] [output-image]

# 比較各版本 RGB 轉灰階的耗時
./build-release/bench_gray
```