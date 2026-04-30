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

/** Adds an axis-aligned box: full width, height, depth in world units (see createBox). */
WASMGL_KEEP void addCube(float width, float height, float depth);

/** UV sphere: radius, segment counts (clamped: width ≥ 3, height ≥ 2). */
WASMGL_KEEP void addSphere(float radius, int widthSeg, int heightSeg);

WASMGL_KEEP void cameraZoomOut(void);
WASMGL_KEEP void cameraZoomIn(void);
WASMGL_KEEP void cameraNudgeViewYawDegrees(float deltaDeg);
WASMGL_KEEP void cameraNudgeViewPitchDegrees(float deltaDeg);
/** Pan / dolly camera in view space: forward, right (strafe), up — world units per step. */
WASMGL_KEEP void cameraNudgePositionView(float alongFront, float alongRight, float alongUp);

WASMGL_KEEP int getSceneObjectCount(void);
WASMGL_KEEP void setSelectedObjectIndex(int idx);
WASMGL_KEEP int getSelectedObjectIndex(void);
/** 0 unknown, 1 cube, 2 sphere, 3 CSG result — for UI labels. */
WASMGL_KEEP int getObjectKind(int index);

/** Read selected mesh dimensions; 0 if not a box / not selected. */
WASMGL_KEEP float getSelectedBoxWidth(void);
WASMGL_KEEP float getSelectedBoxHeight(void);
WASMGL_KEEP float getSelectedBoxDepth(void);
WASMGL_KEEP float getSelectedSphereRadius(void);
WASMGL_KEEP int getSelectedSphereWidthSegments(void);
WASMGL_KEEP int getSelectedSphereHeightSegments(void);
/** Rebuild selected box or sphere from new parameters (no-op for CSG or wrong type). */
WASMGL_KEEP void resizeSelectedBox(float width, float height, float depth);
WASMGL_KEEP void resizeSelectedSphere(float radius, int widthSeg, int heightSeg);

WASMGL_KEEP void nudgeSelectedTranslate(float dx, float dy, float dz);
WASMGL_KEEP void nudgeSelectedRotateDegrees(float rxDeg, float ryDeg, float rzDeg);

WASMGL_KEEP void setBooleanOperandA(int idx);
WASMGL_KEEP void setBooleanOperandB(int idx);
WASMGL_KEEP int getBooleanOperandA(void);
WASMGL_KEEP int getBooleanOperandB(void);
WASMGL_KEEP void performBooleanUnion(void);
WASMGL_KEEP void performBooleanDifference(void);
WASMGL_KEEP void performBooleanIntersection(void);

#ifdef __cplusplus
}
#endif

#endif
