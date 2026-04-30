# WasmGL — CSG-oriented WebGL2 / OpenGL demo

This project builds a small **Constructive Solid Geometry (CSG)** demo: BSP-style union, subtract, and intersect on meshes, plus translate / rotate / scale. The primary target is **WebAssembly** with **Emscripten** and **WebGL2**; the same sources compile to a **native** OpenGL 3.3 + GLFW application for debugging.

## Documentation

| Document | Audience |
|----------|----------|
| This README | Build, layout, run demo, quick JS note |
| **[`docs/WEB_DEVELOPERS.md`](docs/WEB_DEVELOPERS.md)** | **Web developers:** embed `wasmGL.js` / `wasmGL.wasm`, `Module` lifecycle, full exported API reference |

## Layout (maintainability)

| Area | Role |
|------|------|
| [`src/include/wasmgl_gl.h`](src/include/wasmgl_gl.h) | GL headers: GLES3 on wasm, GLEW + GL on desktop. |
| [`src/include/wasmgl_exports.h`](src/include/wasmgl_exports.h) | **Stable C API** exposed to JavaScript; keep in sync with CMake `EXPORTED_FUNCTIONS`. |
| [`src/core/`](src/core/), [`src/primitives/`](src/primitives/) | Geometry, mesh, transforms, factory helpers. |
| [`src/threecsg/`](src/threecsg/) | BSP CSG (ported Three.js–style pipeline). |
| [`src/window/`](src/window/), [`src/shaders/`](src/shaders/) | GLFW window and shader program. |
| [`wasmbuild/mypage.html`](wasmbuild/mypage.html) | Minimal shell: `Module.canvas` + `onRuntimeInitialized` before calling `Module._addCube`. |

**Web integration rule:** call exported functions only after **`Module.onRuntimeInitialized`** (and pass the canvas via `Module` before loading `wasmGL.js`). That avoids racing the GL context and wasm startup.

## Dependencies

### Linux (native)

```bash
sudo apt install build-essential cmake pkg-config \
  libglew-dev libglfw3-dev libgl1-mesa-dev
```

GLM is vendored under [`src/include/glm`](src/include/glm).

There are **no additional packages** beyond the above for UI overlays (axis helper, navigation cube): they use the same OpenGL / GLFW stack and procedural textures only.

### Emscripten (wasm)

Install the official SDK ([Emscripten downloads](https://emscripten.org/docs/getting_started/downloads.html)):

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh   # use this shell for wasm builds
```

This repo can keep a local `emsdk/` clone (gitignored) at the project root; `compileWasm.sh` only needs `emcmake` on your `PATH` after `source emsdk/emsdk_env.sh`.

Verify: `emcc --version` and `emcmake --version`.

## Build

### WebAssembly (CMake)

From the repo root, with `emsdk_env.sh` sourced:

```bash
chmod +x compileWasm.sh
./compileWasm.sh
```

Artifacts (default): `wasmbuild/wasmGL.js`, `wasmbuild/wasmGL.wasm`.

Optional: `WASMGL_BUILD_WASM_DIR=/tmp/my-build ./compileWasm.sh` to change the CMake build directory.

### Native (CMake)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/wasmgl
```

## Run the wasm demo

**Emscripten (opens a browser if available):**

```bash
emrun --port 8080 wasmbuild/mypage.html
```

**Python (no Emscripten required):** from the repo root, serve the `wasmbuild` folder so `mypage.html` and `wasmGL.js` sit at the same URL path level:

```bash
python3 -m http.server 8080 --directory wasmbuild
```

Then open `http://localhost:8080/mypage.html`.

Any other static HTTP server works as long as its document root is `wasmbuild/` (so `wasmGL.js` resolves next to `mypage.html`).

### pthreads (optional)

This default build does **not** use pthreads, so you do **not** need `Cross-Origin-Opener-Policy` / `Cross-Origin-Embedder-Policy` for `SharedArrayBuffer`. If you later enable `-pthread` in CMake, see [Emscripten pthreads](https://emscripten.org/docs/porting/pthreads.html) and serve with cross-origin isolation.

## JavaScript API (exported C functions)

For **embedding, `Module` setup, and a table of every public function**, see **[`docs/WEB_DEVELOPERS.md`](docs/WEB_DEVELOPERS.md)**.

Declared in [`wasmgl_exports.h`](src/include/wasmgl_exports.h). Minimal usage:

```javascript
Module.onRuntimeInitialized = function () {
  Module._addCube(1, 1, 1);
  Module._addSphere(0.4, 16, 16);
};
```

Add new exports by: (1) declare in `wasmgl_exports.h`, (2) implement in C++, (3) append `_symbolName` to `-sEXPORTED_FUNCTIONS` in [`CMakeLists.txt`](CMakeLists.txt).

## Troubleshooting

- **Blank canvas / WebGL errors:** open the browser devtools console; ensure WebGL2 is available.
- **Missing GLM / includes:** headers expect `-I src/include` (CMake sets this).
- **Exports undefined on `Module`:** confirm `EXPORTED_FUNCTIONS` and `EMSCRIPTEN_KEEPALIVE` (`wasmgl_exports.h`) match; rebuild wasm.
