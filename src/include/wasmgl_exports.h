/**
 * JavaScript / WebAssembly boundary — stable C ABI for the demo embed.
 *
 * Keep this list in sync with CMake Emscripten flags:
 *   -sEXPORTED_FUNCTIONS=[_main,_addCube,...]
 *
 * From JS (classic shell script): after load, call via Module._addCube(...) inside
 * Module.onRuntimeInitialized so GL + wasm are ready.
 */

#ifndef WASMGL_EXPORTS_H
#define WASMGL_EXPORTS_H

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define WASMGL_KEEP EMSCRIPTEN_KEEPALIVE
#else
#define WASMGL_KEEP
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Adds an axis-aligned box mesh (half-extents derived from dimensions). */
WASMGL_KEEP void addCube(int width, int height, int depth);

/** Adds a UV sphere with fixed tessellation (see implementation). */
WASMGL_KEEP void addSphere(void);

/** Widen FOV (dolly out). */
WASMGL_KEEP void cameraZoomOut(void);
/** Narrow FOV (dolly in). */
WASMGL_KEEP void cameraZoomIn(void);

/** Number of meshes in the scene (same order as added). */
WASMGL_KEEP int getSceneObjectCount(void);
/** Select object by index, or a negative idx to clear selection. */
WASMGL_KEEP void setSelectedObjectIndex(int idx);
/** Current selection index, or -1 if none. */
WASMGL_KEEP int getSelectedObjectIndex(void);
/** 0 unknown, 1 cube, 2 sphere — for UI labels. */
WASMGL_KEEP int getObjectKind(int index);

/** Move selected object in world space (metres). */
WASMGL_KEEP void nudgeSelectedTranslate(float dx, float dy, float dz);
/** Rotate selected object (degrees, Euler X then Y then Z to match Mesh::rotate). */
WASMGL_KEEP void nudgeSelectedRotateDegrees(float rxDeg, float ryDeg, float rzDeg);

/** Boolean CSG: choose two distinct scene indices (negative clears). */
WASMGL_KEEP void setBooleanOperandA(int idx);
WASMGL_KEEP void setBooleanOperandB(int idx);
WASMGL_KEEP int getBooleanOperandA(void);
WASMGL_KEEP int getBooleanOperandB(void);
/** Replace both operands with one new mesh: union, A minus B, or intersection. */
WASMGL_KEEP void performBooleanUnion(void);
WASMGL_KEEP void performBooleanDifference(void);
WASMGL_KEEP void performBooleanIntersection(void);

#ifdef __cplusplus
}
#endif

#endif
