#!/bin/bash

# compile with docker image
docker run \
  --rm \
  -v $(pwd):/src \
  -u $(id -u):$(id -g) \
  emscripten/emsdk \
  mkdir wasmbuild
  emcc -std=c++17 ./src/*.cpp -s USE_GLFW=3 -s FULL_ES3=1 -s GL_ASSERTIONS=1 -s GL_DEBUG=1  -s WASM=1 -o wasmbuild/main.html
  emrun --port=8080 wasmbuild/main.html