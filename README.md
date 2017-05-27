# Introduction

qpalette is a simple tool for converting textures to the Quake 1 color 
palette.

# Compiling

Dependencies: libpng

1)  make

# Usage

qpalette [options] <sourcefile>

Examples: 
```
./qpalette -b -t png lightning.bmp
```
  Will convert RGB "lightning.bmp" into paletted "lightning_conv.png" with fullbrights enabled

```
./qpalette -t bmp -o output.bmp texture01.png
```
  Will convert RGB "texture01.png" into palletted "output.bmp"

## Options:
  -b   -  Allow use of fullbright colors from Quake 1 colormap
  
  -o   -  Output file name, e.g -o out.png - default is input_conv.ext
  
  -t   -  Output file type, Valid values are bmp, png - default is input filetype
  
  -h   -  Print usage help
