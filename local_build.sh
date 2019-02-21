#!/bin/bash

export GTEST_ROOT=/usr/src/gtest

rm -rf build*
mkdir -p build-gcc
mkdir -p build-clang

cd build-gcc
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/mikaelr/1_install/cmake/host_gcc_tc.cmake
make
make test

cd ../build-clang
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/mikaelr/1_install/cmake/host_clang_tc.cmake
make
make test

