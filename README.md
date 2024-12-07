# OpenCV C implementation

## Table of Contents

[TOC]

## Color conversions

### Orignial implementation

```cpp
cvt_color(gray_mat, src, COLOR_RGB2GRAY);
```

### SIMD implementation

```cpp
cvt_color(gray_mat1, src, COLOR_RGB2GRAY_INTRIN);
```

### Result

```cpp
Image successfully written to Lenna_gray_v1.png
Execution time of RGB2GRAY: 0.742150ms 
Image successfully written to Lenna_gray_v2.png
Execution time of RGB2GRAY: 0.000070ms 
```