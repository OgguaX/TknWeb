cmake_minimum_required(VERSION 3.26)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# WebAssembly target
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Clang + wasm-ld linker
set(CMAKE_C_COMPILER /opt/homebrew/opt/llvm/bin/clang)
set(CMAKE_C_FLAGS "-target wasm32-wasip1 --sysroot=/Users/oggua/Documents/GitHub/emsdk/upstream/emscripten/cache/sysroot -fvisibility=hidden -nostdlib")
set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=/opt/homebrew/bin/wasm-ld -Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined")

project(TickernelWeb VERSION 1.1 LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()


# Build WebAssembly module
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
add_executable(${PROJECT_NAME}.wasm ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME}.wasm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

