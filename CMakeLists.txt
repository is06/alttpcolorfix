cmake_minimum_required(VERSION 3.10)

project(alttpcolorfix)

add_executable(alttpcolorfix alttpcolorfix.c)

target_link_libraries(alttpcolorfix png)