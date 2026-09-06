cmake_minimum_required(VERSION 3.7)
project(Filters)

include(filters.cmake)

add_definitions(-fpic)

SET(LIBPGF_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/dec_pgf.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pgf_decode.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/pgf_stubs.cpp
)

SET(LIBPGF_LIBS
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libpgf.a
)

SET(LIBPGF_INC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_filter(libpgf
        "${LIBPGF_SRC}"
        "${LIBPGF_LIBS}"
        []
        ""
        "${LIBPGF_INC}"
        ""
        "1")

# libpgf's public headers still carry dynamic exception specifications
# (throw(IOException)), removed in C++17 - which is emcc's default - so any
# translation unit that includes them has to be compiled as C++14. Same
# standard the library itself is built with. Only pgf_decode.cpp includes
# PGFimage.h; dec_pgf.c is plain C and pgf_stubs.cpp only needs <new>.
set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/pgf_decode.cpp
        PROPERTIES COMPILE_FLAGS "-std=c++14")
