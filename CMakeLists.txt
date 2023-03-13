cmake_minimum_required(VERSION 3.10)
set(CMAKE_BUILD_TYPE Debug)

project(alttpcolorfix)

add_executable(alttpcolorfix alttpcolorfix.c)

target_link_libraries(alttpcolorfix png)