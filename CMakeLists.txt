cmake_minimum_required(VERSION 3.2)
project(mandelbrot)

add_rpx(mandelbrot src/main.c src/memory.c src/draw.c)
target_link_libraries(
    mandelbrot
    coreinit
    proc_ui
    sysapp
    vpad
    )
