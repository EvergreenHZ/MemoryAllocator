cmake_minimum_required (VERSION 3.6.1)

project(mm)

include_directories (../include/)

set (SRC ./src/mm_implicit_list.c
        ./src/memlib.c
        ./src/main.c)

add_executable (mm ${SRC})
