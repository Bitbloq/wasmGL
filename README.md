# OpenGL/WebGL CSG library

This repository aims to build a library that allows creating 3D scenarios based on Constructive Solid Geometry (CSG).

This library is aimed to the web so it will compiled to WASM using emscripten.

For developing and testins purpose it will also compile to desktop applications with gcc

### Wasm build
```
./compileWasm.sh
```

### Native C++ build
Install dependencies

```
sudo apt install libglew-dev libglm-dev libglfw3-dev 
```

Build using cmake
```
cd build
cmake ..
make
```
