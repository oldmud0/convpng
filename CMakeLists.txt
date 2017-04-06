project(ConvPNG)
cmake_minimum_required(VERSION 2.9.5)

file(GLOB convpng_libs_sources libs/*.c)
file(GLOB convpng_sources *.c)
add_executable(convpng ${convpng_libs_sources} ${convpng_sources})
target_compile_options(convpng PUBLIC -Wall -Wextra -O3 -std=c11)
target_link_libraries(convpng -flto)
