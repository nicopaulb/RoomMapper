cmake_minimum_required(VERSION 3.10.0)
project(rplidar VERSION 0.1.0 LANGUAGES C)

add_executable(rplidar ../Core/Src/rplidar.c main.c mock/stm32f4xx_hal.c)
target_include_directories(rplidar PRIVATE mock ../Core/Inc)