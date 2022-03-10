# OpenGL library for CSG 3D scenarios editing

This repository aims to build a library that allows creating 3D scenarios based on Constructive Solid Geometry (CSG).

This library is aimed to the web so it will compiled to WASM using emscripten.

For developing and testins purpose it will also compile to desktop applications with gcc

### Wasm build
```
./compileWasm.sh
```

### Native C++ build
```
cd build
cmake ..
make
```
