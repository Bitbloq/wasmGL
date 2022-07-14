#!/bin/bash

# compile with docker image
#docker run \
  #--rm \
  #-v $(pwd):/src \
  #-u $(id -u):$(id -g) \
  #emscripten/emsdk \
  [ ! -d "./wasmbuild" ] && mkdir -p wasmbuild 
  emcc -O3 -std=c++17 -pthread -sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency ./src/*.cpp ./src/*/*.cpp -I./src/include -s USE_GLFW=3 -s FULL_ES3=1 -s GL_ASSERTIONS=1 -s GL_DEBUG=1  -s WASM=1 -o wasmbuild/main.html 
  #npx http-server -p=8080 wasmbuild -o main.html
  emrun --port=8080 wasmbuild/main.html