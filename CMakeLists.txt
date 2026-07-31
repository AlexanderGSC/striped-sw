cmake_minimum_required(VERSION 3.20)
project(striped_sw_simd CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Flags for RISC-V Vector
set(RISCV_FLAGS "-march=rv64gcv" "-mabi=lp64d")

set(OPTIMIZATION_FLAGS 
    "-O3"
    "-flto"                     
    "-fno-fat-lto-objects"
    "-fomit-frame-pointer"      
)

# Executable: Benchmarks
set(LMUL_VALUES 1 2 4 8)

foreach(lmul ${LMUL_VALUES})
    set(EXEC_NAME "bench1m${lmul}")
    
    add_executable(${EXEC_NAME} benchmarks/bench1.cpp)
    target_include_directories(${EXEC_NAME} PRIVATE include)
    
    # -DCONFIG_LMUL
    target_compile_options(${EXEC_NAME} PRIVATE ${RISCV_FLAGS} ${OPTIMIZATION_FLAGS} -DCONFIG_LMUL=${lmul})
    target_link_options(${EXEC_NAME} PRIVATE ${RISCV_FLAGS} ${OPTIMIZATION_FLAGS} -static)
endforeach()

foreach(lmul ${LMUL_VALUES})
    set(EXEC_NAME "bench2m${lmul}")
    
    add_executable(${EXEC_NAME} benchmarks/bench2.cpp)
    target_include_directories(${EXEC_NAME} PRIVATE include)
    
    # -DCONFIG_LMUL
    target_compile_options(${EXEC_NAME} PRIVATE ${RISCV_FLAGS} ${OPTIMIZATION_FLAGS} -DCONFIG_LMUL=${lmul})
    target_link_options(${EXEC_NAME} PRIVATE ${RISCV_FLAGS} ${OPTIMIZATION_FLAGS} -static)
endforeach()

foreach(lmul ${LMUL_VALUES})
    set(EXEC_NAME "bench3m${lmul}")
    
    add_executable(${EXEC_NAME} benchmarks/bench3.cpp)
    target_include_directories(${EXEC_NAME} PRIVATE include)
    
    # -DCONFIG_LMUL
    target_compile_options(${EXEC_NAME} PRIVATE ${RISCV_FLAGS} ${OPTIMIZATION_FLAGS} -DCONFIG_LMUL=${lmul})
    target_link_options(${EXEC_NAME} PRIVATE ${RISCV_FLAGS} ${OPTIMIZATION_FLAGS} -static)
endforeach()

# Executables: Unit Tests
add_executable(unit_test1 tests/test.cpp)
target_include_directories(unit_test1 PRIVATE include)
target_compile_options(unit_test1 PRIVATE ${RISCV_FLAGS})
target_link_options(unit_test1 PRIVATE -static)

add_executable(unit_test2 tests/test2.cpp)
target_include_directories(unit_test2 PRIVATE include)
target_compile_options(unit_test2 PRIVATE ${RISCV_FLAGS})
target_link_options(unit_test2 PRIVATE -static)

add_executable(unit_test3 tests/test3.cpp)
target_include_directories(unit_test3 PRIVATE include)
target_compile_options(unit_test3 PRIVATE ${RISCV_FLAGS})
target_link_options(unit_test3 PRIVATE -static)

# Testing:  CTest + QEMU
enable_testing()
set(QEMU_CMD qemu-riscv64 -cpu rv64,v=true,vlen=256,vext_spec=v1.0)

add_test(NAME Test1 COMMAND ${QEMU_CMD} $<TARGET_FILE:unit_test1>)
add_test(NAME Test2 COMMAND ${QEMU_CMD} $<TARGET_FILE:unit_test2>)
add_test(NAME Test3 COMMAND ${QEMU_CMD} $<TARGET_FILE:unit_test3>)