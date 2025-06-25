cmake_minimum_required(VERSION 3.10)

project(wizardc)
aux_source_directory(. SRC)
add_compile_options(-std=c++20 -Wall -Wextra)


add_executable(wizardc ${SRC})
