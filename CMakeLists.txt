cmake_minimum_required(VERSION 3.20)
project(striped_sw_simd CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(unit_test1 tests/test.cpp)
add_executable(unit_test2 tests/test2.cpp)

set(RISCV_FLAGS "-march=rv64gcv" "-mabi=lp64d")

target_compile_options(unit_test1 PRIVATE ${RISCV_FLAGS})
target_compile_options(unit_test2 PRIVATE ${RISCV_FLAGS})

target_link_options(unit_test1 PRIVATE -static)
target_link_options(unit_test2 PRIVATE -static)




enable_testing()

add_test(NAME Test1 COMMAND qemu-riscv64 -L /usr/riscv64-linux-gnu $<TARGET_FILE:unit_test1>)
add_test(NAME Test2 COMMAND qemu-riscv64 -L /usr/riscv64-linux-gnu $<TARGET_FILE:unit_test2>)
