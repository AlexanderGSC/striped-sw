cmake_minimum_required(VERSION 3.20)
project(striped_sw_simd CXX)

set(CMAKE_CXX_STANDARD 23)

include_directories(include)

# 2. Creamos el ejecutable de pruebas
add_executable(unit_tests tests/test.cpp)
