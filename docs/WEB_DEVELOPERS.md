# WasmGL - Web / JavaScript developer guide

This document explains how to **embed the WasmGL WebAssembly build** in a web page, load it correctly, and call the **C API** from JavaScript. It is the companion to the project root [`README.md`](../README.md) (build and layout) and the C header [`src/include/wasmgl_exports.h`](../src/include/wasmgl_exports.h) (source of truth for signatures).

## What you ship

After `./compileWasm.sh` (or CMake with Emscripten), you typically publish:

| Artifact | Role |
|----------|------|
| `wasmGL.js` | Emscripten glue; loads `wasmGL.wasm`, exposes `Module`, hooks WebGL |
| `wasmGL.wasm` | Compiled binary (same directory URL as `wasmGL.js`) |

Serve both over **HTTP(S)**. Opening `file://` often breaks wasm fetch or MIME types - use any static server (see root README).

## Coordinate system and behavior

- **World and objects:** Z-up. New primitives are created with object-local **+Z** as height/up; no primitive needs a hidden startup rotation to match the world grid.
- **Selected object axes:** Selecting one object draws RGB local axes from that object's self-reference system. Axis length is based on the object's local bounds so the helper stays visible after dimension edits.
- **Camera:** Orbit around origin; left-drag orbit, right-drag pan, wheel zoom (implemented in the native/wasm main loop unless you build a custom shell).
- **Rendering:** WebGL **2** (`-sMIN_WEBGL_VERSION=2` in CMake). Requires a browser with WebGL2.

## Loading order (critical)

Emscripten expects a global **`Module`** object **before** `wasmGL.js` runs. The glue script reads `Module` and appends/merges configuration.

1. Create a **canvas** element (the GL drawing surface).
2. Define **`Module`** with at least:
   - **`canvas`**: that DOM element (required so GLFW/SDL canvas binding works).
   - **`onRuntimeInitialized`**: callback invoked when wasm + GL are ready - **this is the only safe place to call exported functions** the first time.

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

The public API is intentionally small and engine-owned: the browser should query wasm for object state, operation stacks, and CSG selection state instead of keeping parallel object models in JavaScript.

All scene and object self-reference systems are **Z-up**. Box, cylinder, and cone geometry is baked into Z-up object space at creation time; there is no hidden initial model rotation.

### Scene: add primitives

Each call returns an unsigned **serial id** for the new object.

| JS call | Parameters | Notes |
|---------|------------|--------|
| `_addCube(w, h, d)` | width, height, depth (> 0) | Box; orange tint; becomes selected |
| `_addSphere(radius, widthSeg, heightSeg)` | radius; segment counts (>= min in impl) | Blue tint |
| `_addPyramid(side, height)` | base edge; height | |
| `_addCylinder(rBottom, rTop, height, radialSeg, heightSeg)` | radii >= 0; height > 0 | Frustum; cone uses `_addCone` |
| `_addCone(radius, height, radialSeg, heightSeg)` | top radius 0 internally | |
| `_addTorus(majorR, minorR, radialSeg, tubularSeg)` | standard torus | |
| `_addDefaultObject(kind)` | kind from `getObjectKind` values | Adds demo-friendly 1-ish default dimensions and returns the new object **serial id**, or `0` on failure |

Each `add*` call returns the new object's **serial id** (unsigned). Prefer referencing objects by serial from JavaScript; scene array indices still appear in parameter/transform APIs as `objectIndex`.

### Scene: selection and queries

| JS call | Returns | Notes |
|---------|---------|--------|
| `_getSceneObjectCount()` | `int` | Number of meshes |
| `_setSelectedObjectId(serialId)` | none | `0` clears selection |
| `_getSelectedObjectId()` | unsigned | `0` if none |
| `_getObjectKind(index)` | `int` | Kind by scene index; `0` unknown |
| `_getObjectKindById(serialId)` | `int` | Kind by serial id |
| `_getObjectSerialId(index)` | `int` | Stable engine-owned id (same as slot below) |
| `_getSceneObjectId(index)` | unsigned | Serial id at list index; `0` if out of range |
| `_removeSceneObject(serialId)` | `int` | `1` if removed |

### Object parameters

Use generic parameter slots instead of kind-specific selected-object getters/setters.

| JS call | Notes |
|---------|--------|
| `_getObjectFloatParameter(objectIndex, slot)` | Reads one float parameter |
| `_getObjectIntParameter(objectIndex, slot)` | Reads one integer parameter |
| `_setObjectParameters(objectIndex, p0, p1, p2, i0, i1)` | Rebuilds that object's geometry; returns `1` on success, `0` on invalid input/kind |

Parameter slots:

| Kind | Float slots | Int slots |
|------|-------------|-----------|
| Box | `p0` width X, `p1` height Z, `p2` depth Y | unused |
| Sphere | `p0` radius | `i0` width/ring segments, `i1` height/pole segments |
| Pyramid | `p0` base edge, `p1` height Z | unused |
| Cylinder | `p0` bottom radius, `p1` top radius, `p2` height Z | `i0` radial segments, `i1` height segments |
| Cone | `p0` bottom radius, `p2` height Z; top radius forced to `0` | `i0` radial segments, `i1` height segments |
| Torus | `p0` major radius, `p1` tube/minor radius | `i0` tube/radial segments, `i1` ring/tubular segments |

### Selected object - transform

Transform operation stacks are owned by the wasm engine. The browser should query these functions to render the list and call setters when the user edits an operation. The engine caches cumulative transform matrices per operation; editing operation `N` recalculates only operation `N` and later cached matrices. BSP/CSG recomputation stays lazy until a boolean operation needs it.

Each operation has a reference frame. Frame `1` is scene/world and pre-multiplies the object matrix. Frame `2` is object/self and post-multiplies the object matrix, so translation and rotation follow the object's current local axes.

| JS call | Notes |
|---------|--------|
| `_getObjectTransformOperationCount(objectIndex)` | Number of operations |
| `_getObjectTransformOperationType(objectIndex, opIndex)` | `1` translation, `2` rotation |
| `_getObjectTransformOperationFrame(objectIndex, opIndex)` | `1` scene/world, `2` object/self |
| `_getObjectTransformOperationValue(objectIndex, opIndex, axis)` | `axis`: `0` X, `1` Y, `2` Z; units or degrees depending on type |
| `_addObjectTransformOperation(objectIndex, type, frame)` | Adds a zero-valued operation and returns its index |
| `_setObjectTransformOperationFull(objectIndex, opIndex, type, frame, x, y, z)` | Replaces operation type/frame/values and updates cached transforms from this operation onward |
| `_removeObjectTransformOperation(objectIndex, opIndex)` | Removes one operation and updates cached transforms from that position onward |

### CSG

The engine owns the boolean selection. Add operands in order, then run one operation. Difference uses the first operand minus every later operand.

| JS call | Notes |
|---------|--------|
| `_clearBooleanSelection()` | Clears engine CSG selection |
| `_addBooleanSelectionObject(objectIndex)` | Adds an object if valid/not already selected; returns `1` or `0` |
| `_getBooleanSelectionCount()` | Current valid operand count |
| `_getBooleanSelectionObject(selectionIndex)` | Returns scene object index at that selection position, or `-1` |
| `_performBooleanOperation(operation)` | `1` union, `2` difference, `3` intersection; returns result index or `-1` |

### Camera (orbit)

These mirror UI/button behavior in the default app; they affect the global orbit camera.

| JS call | Notes |
|---------|--------|
| `_cameraZoomIn()` | Dolly in (smaller orbit radius), not FOV |
| `_cameraZoomOut()` | Dolly out |
| `_cameraNudgeViewYawDegrees(deltaDeg)` | |
| `_cameraNudgeViewPitchDegrees(deltaDeg)` | |
| `_cameraNudgePositionView(alongFront, alongRight, alongUp)` | Truck/dolly in camera frame |

### Display helpers

| JS call | Notes |
|---------|--------|
| `_setBaseGridVisible(visible)` | `1` show XY grid, `0` hide |
| `_setAxisHelperVisible(visible)` | `1` show corner RGB axes, `0` hide |

The **navigation cube** (view gizmo) is part of the C++ main loop and is **not** toggled by a separate export today.

## Adding a new export

1. Declare in [`wasmgl_exports.h`](../src/include/wasmgl_exports.h) with `WASMGL_KEEP`.
2. Implement in C++ (typically `main.cpp` `extern "C"` block).
3. Append `_yourSymbol` to the `WASMGL_EXPORTED_FUNCTIONS` list in [`CMakeLists.txt`](../CMakeLists.txt).
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

- [`src/include/wasmgl_exports.h`](../src/include/wasmgl_exports.h) - C declarations
- [`wasmbuild/mypage.html`](../wasmbuild/mypage.html) - Full demo integration
- [`CMakeLists.txt`](../CMakeLists.txt) - `EXPORTED_FUNCTIONS` list
