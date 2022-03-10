cmake_minimum_required(VERSION 3.10)

# set the project name
project(WasmGL VERSION 1.0)


# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

FILE(GLOB_RECURSE SOURCES src/*.cpp)
set(CMAKE_CXX_FLAGS "-fdiagnostics-color=always -std=c++17")

include_directories(${CMAKE_SOURCE_DIR}/include)

# add the executable
add_executable(wasmgl ${SOURCES})
target_link_libraries(wasmgl -lGLEW -lglfw -lGL)

