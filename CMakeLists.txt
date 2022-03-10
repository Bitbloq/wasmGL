cmake_minimum_required(VERSION 3.10)

# set the project name
project(WasmGL VERSION 1.0)

FILE(GLOB SOURCES src/*.cpp)
set(CMAKE_CXX_FLAGS "-fdiagnostics-color=always -std=c++17")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)



# add the executable
add_executable(wasmgl ${SOURCES})
target_link_libraries(wasmgl -lGLEW -lglfw -lGL)

