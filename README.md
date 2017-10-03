# Mandelbrot for Wii U

## Requirements

Needs [wut](https://github.com/decaf-emu/wut)

## Building

```
cd mandelbrot-wiiu
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$WUT_ROOT/cmake/wut-toolchain.cmake ../
make
```

## Running

Find `build/mandelbrot.rpx` and run it with [decaf](https://github.com/decaf-emu/decaf-emu) or on a real console.
