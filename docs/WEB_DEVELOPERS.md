# WasmGL — Web / JavaScript developer guide

This document explains how to **embed the WasmGL WebAssembly build** in a web page, load it correctly, and call the **C API** from JavaScript. It is the companion to the project root [`README.md`](../README.md) (build and layout) and the C header [`src/include/wasmgl_exports.h`](../src/include/wasmgl_exports.h) (source of truth for signatures).

## What you ship

After `./compileWasm.sh` (or CMake with Emscripten), you typically publish:

| Artifact | Role |
|----------|------|
| `wasmGL.js` | Emscripten glue; loads `wasmGL.wasm`, exposes `Module`, hooks WebGL |
| `wasmGL.wasm` | Compiled binary (same directory URL as `wasmGL.js`) |

Serve both over **HTTP(S)**. Opening `file://` often breaks wasm fetch or MIME types—use any static server (see root README).

## Coordinate system and behavior

- **World:** Z-up grid; many primitives follow Three.js-style Y-height then are rotated so height aligns with **+Z** (see source comments in `main.cpp`).
- **Camera:** Orbit around origin; left-drag orbit, right-drag pan, wheel zoom (implemented in the native/wasm main loop unless you build a custom shell).
- **Rendering:** WebGL **2** (`-sMIN_WEBGL_VERSION=2` in CMake). Requires a browser with WebGL2.

## Loading order (critical)

Emscripten expects a global **`Module`** object **before** `wasmGL.js` runs. The glue script reads `Module` and appends/merges configuration.

1. Create a **canvas** element (the GL drawing surface).
2. Define **`Module`** with at least:
   - **`canvas`**: that DOM element (required so GLFW/SDL canvas binding works).
   - **`onRuntimeInitialized`**: callback invoked when wasm + GL are ready—**this is the only safe place to call exported functions** the first time.

Example **minimal** shell:

```html
<!DOCTYPE html>
<html>
<head><meta charset="utf-8" /></head>
<body>
  <canvas id="c" width="800" height="600"></canvas>
  <script>
    var Module = {
      canvas: document.getElementById('c'),
      onRuntimeInitialized: function () {
        // Safe to call exports here:
        Module._addCube(1, 1, 1);
      }
    };
  </script>
  <script src="wasmGL.js"></script>
</body>
</html>
```

**Do not** call `Module._addCube` before `onRuntimeInitialized` runs. Do not load `wasmGL.js` before `Module` is defined.

### Same-origin and paths

`wasmGL.js` loads `wasmGL.wasm` with a URL **relative to the HTML page** (or as configured by Emscripten). Keep `wasmGL.js` and `wasmGL.wasm` in the **same directory** as your entry HTML, or adjust `locateFile` in `Module`:

```javascript
var Module = {
  canvas: document.getElementById('c'),
  locateFile: function (path) {
    if (path.endsWith('.wasm')) return '/assets/wasmGL.wasm';
    return path;
  },
  onRuntimeInitialized: function () { /* ... */ }
};
```

## Calling C from JavaScript (name mangling)

Exported C functions appear on **`Module`** with a **leading underscore**:

| C name (`wasmgl_exports.h`) | JavaScript |
|------------------------------|------------|
| `addCube` | `Module._addCube` |
| `getSceneObjectCount` | `Module._getSceneObjectCount` |

All arguments are passed as **numbers** for `float`/`int`; there are no string exports in the current API.

## Public API reference

Below, **“selected”** means the index set by `setSelectedObjectIndex`. Several getters return `0` or meaningless values if the selection is not of the matching primitive kind.

### Scene: add primitives

| JS call | Parameters | Notes |
|---------|------------|--------|
| `_addCube(w, h, d)` | width, height, depth (> 0) | Box; orange tint; becomes selected |
| `_addSphere(radius, widthSeg, heightSeg)` | radius; segment counts (≥ min in impl) | Blue tint |
| `_addPyramid(side, height)` | base edge; height | |
| `_addCylinder(rBottom, rTop, height, radialSeg, heightSeg)` | radii ≥ 0; height > 0 | Frustum; cone uses `_addCone` |
| `_addCone(radius, height, radialSeg, heightSeg)` | top radius 0 internally | |
| `_addTorus(majorR, minorR, radialSeg, tubularSeg)` | standard torus | |

### Scene: selection and queries

| JS call | Returns | Notes |
|---------|---------|--------|
| `_getSceneObjectCount()` | `int` | Number of meshes |
| `_setSelectedObjectIndex(idx)` | — | `idx` in `[0, n-1]` or `-1` for none |
| `_getSelectedObjectIndex()` | `int` | `-1` if none |
| `_getObjectKind(index)` | `int` | `0` unknown, `1` cube, `2` sphere, `3` CSG, `4` pyramid, `5` cylinder, `6` torus, `7` cone |

### Selected object — read dimensions (per kind)

| JS call | Returns |
|---------|---------|
| `_getSelectedBoxWidth()` … `_getSelectedBoxDepth()` | `float` |
| `_getSelectedSphereRadius()` | `float` |
| `_getSelectedSphereWidthSegments()` … `_getSelectedSphereHeightSegments()` | `int` |
| `_getSelectedPyramidSide()` … `_getSelectedPyramidHeight()` | `float` |
| `_getSelectedCylinderRadiusBottom()` … `_getSelectedCylinderHeightSegments()` | mixed |
| `_getSelectedTorusMajorRadius()` … `_getSelectedTorusTubularSegments()` | mixed |

### Selected object — resize

| JS call | Notes |
|---------|--------|
| `_resizeSelectedBox(w, h, d)` | All > 0 |
| `_resizeSelectedSphere(radius, wSeg, hSeg)` | |
| `_resizeSelectedPyramid(side, height)` | |
| `_resizeSelectedCylinder(rBottom, rTop, height, radialSeg, heightSeg)` | Cone selection may force top radius 0 in UI |
| `_resizeSelectedTorus(majorR, minorR, radialSeg, tubularSeg)` | |

### Selected object — transform

| JS call | Notes |
|---------|--------|
| `_nudgeSelectedTranslate(dx, dy, dz)` | World-space translation delta |
| `_nudgeSelectedRotateDegrees(rx, ry, rz)` | Degrees per axis |

### Camera (orbit)

These mirror UI/button behavior in the default app; they affect the global orbit camera.

| JS call | Notes |
|---------|--------|
| `_cameraZoomIn()` | Dolly in (smaller orbit radius), not FOV |
| `_cameraZoomOut()` | Dolly out |
| `_cameraNudgeViewYawDegrees(deltaDeg)` | |
| `_cameraNudgeViewPitchDegrees(deltaDeg)` | |
| `_cameraNudgePositionView(alongFront, alongRight, alongUp)` | Truck/dolly in camera frame |

### CSG (two operands)

Pick two distinct scene indices, then run one operation; implementation removes the operands and inserts the result.

| JS call | Notes |
|---------|--------|
| `_setBooleanOperandA(idx)` | `-1` clears |
| `_setBooleanOperandB(idx)` | |
| `_getBooleanOperandA()` … `_getBooleanOperandB()` | |
| `_performBooleanUnion()` | |
| `_performBooleanDifference()` | |
| `_performBooleanIntersection()` | |

### Display helpers

| JS call | Notes |
|---------|--------|
| `_setBaseGridVisible(visible)` | `1` show XY grid, `0` hide |
| `_setAxisHelperVisible(visible)` | `1` show corner RGB axes, `0` hide |

The **navigation cube** (view gizmo) is part of the C++ main loop and is **not** toggled by a separate export today.

## Adding a new export

1. Declare in [`wasmgl_exports.h`](../src/include/wasmgl_exports.h) with `WASMGL_KEEP`.
2. Implement in C++ (typically `main.cpp` `extern "C"` block).
3. Append `_yourSymbol` to `-sEXPORTED_FUNCTIONS` in [`CMakeLists.txt`](../CMakeLists.txt) under `EMSCRIPTEN` `target_link_options`.
4. Rebuild wasm; call `Module._yourSymbol` from JS after `onRuntimeInitialized`.

## Stack and memory

CMake sets a **large wasm stack** (`-sSTACK_SIZE=33554432`) because BSP/CSG can recurse deeply. If you hit `RuntimeError: memory access out of bounds` during CSG, avoid enormous meshes first or increase stack further (with maintainer review).

## Troubleshooting

| Symptom | Check |
|---------|--------|
| `Module._foo is not a function` | Symbol missing from `EXPORTED_FUNCTIONS` or typo (`_` prefix) |
| Blank canvas | WebGL2 support; console errors; `canvas` passed on `Module` |
| Nothing happens on button click | Calls run before `onRuntimeInitialized` |
| Wasm fails to load | Serve over HTTP; correct path to `.wasm`; CORS if cross-origin |

## Reference files

- [`src/include/wasmgl_exports.h`](../src/include/wasmgl_exports.h) — C declarations  
- [`wasmbuild/mypage.html`](../wasmbuild/mypage.html) — Full demo integration  
- [`CMakeLists.txt`](../CMakeLists.txt) — `EXPORTED_FUNCTIONS` list  
