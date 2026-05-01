/**
 * JavaScript / WebAssembly boundary - stable C ABI for the demo embed.
 *
 * Keep this list in sync with CMake Emscripten flags:
 *   -sEXPORTED_FUNCTIONS=[_main,_addCube,...]
 *
 * Object kind (getObjectKind): 0 unknown, 1 cube, 2 sphere, 3 CSG, 4 pyramid, 5 cylinder, 6 torus, 7 cone (solid).
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

WASMGL_KEEP void addCube(float width, float height, float depth);
WASMGL_KEEP void addSphere(float radius, int widthSeg, int heightSeg);
/** Equilateral triangular base; `side` = base edge length. */
WASMGL_KEEP void addPyramid(float side, float height);
/** Z-axis cylinder / truncated cylinder; equal radii = right cylinder. */
WASMGL_KEEP void addCylinder(float radiusBottom, float radiusTop, float height, int radialSeg, int heightSeg);
/** Solid cone (apex up): same mesh as cylinder with top radius 0 (Three.js ConeGeometry-style). */
WASMGL_KEEP void addCone(float radius, float height, int radialSeg, int heightSeg);
/** Ring in XY plane; major = hole-to-tube-center, minor = tube radius. */
WASMGL_KEEP void addTorus(float majorRadius, float minorRadius, int radialSeg, int tubularSeg);
/** Add a primitive with demo-friendly default dimensions; kind = getObjectKind values. Returns new object index or -1. */
WASMGL_KEEP int addDefaultObject(int kind);

/** Dolly orbit distance (TS zoom in/out, ~0.95^dolly per step), not FOV. */
WASMGL_KEEP void cameraZoomOut(void);
WASMGL_KEEP void cameraZoomIn(void);
WASMGL_KEEP void cameraNudgeViewYawDegrees(float deltaDeg);
WASMGL_KEEP void cameraNudgeViewPitchDegrees(float deltaDeg);
WASMGL_KEEP void cameraNudgePositionView(float alongFront, float alongRight, float alongUp);

WASMGL_KEEP int getSceneObjectCount(void);
WASMGL_KEEP void setSelectedObjectIndex(int idx);
WASMGL_KEEP int getSelectedObjectIndex(void);
WASMGL_KEEP int getObjectKind(int index);
WASMGL_KEEP int getObjectSerialId(int index);

/** Kind-specific parameter slots are documented in docs/WEB_DEVELOPERS.md. */
WASMGL_KEEP float getObjectFloatParameter(int objectIndex, int parameterIndex);
WASMGL_KEEP int getObjectIntParameter(int objectIndex, int parameterIndex);
WASMGL_KEEP int setObjectParameters(int objectIndex, float p0, float p1, float p2, int i0, int i1);

/** Transform operation type: 1 translation, 2 rotation (Euler degrees). */
WASMGL_KEEP int getObjectTransformOperationCount(int objectIndex);
WASMGL_KEEP int getObjectTransformOperationType(int objectIndex, int opIndex);
/** Reference frame: 1 scene/world, 2 object/local. */
WASMGL_KEEP int getObjectTransformOperationFrame(int objectIndex, int opIndex);
WASMGL_KEEP float getObjectTransformOperationValue(int objectIndex, int opIndex, int axis);
WASMGL_KEEP int addObjectTransformOperation(int objectIndex, int type, int frame);
WASMGL_KEEP void setObjectTransformOperationFull(int objectIndex, int opIndex, int type, int frame, float x, float y, float z);
WASMGL_KEEP void removeObjectTransformOperation(int objectIndex, int opIndex);

WASMGL_KEEP void clearBooleanSelection(void);
WASMGL_KEEP int addBooleanSelectionObject(int objectIndex);
WASMGL_KEEP int getBooleanSelectionCount(void);
WASMGL_KEEP int getBooleanSelectionObject(int selectionIndex);
/** Operation: 1 union, 2 difference (first minus rest), 3 intersection. Returns result index or -1. */
WASMGL_KEEP int performBooleanOperation(int operation);

/** 1 = show Bitbloq-style base grid (XY plane), 0 = hide. */
WASMGL_KEEP void setBaseGridVisible(int visible);

/** 1 = show bottom-right world axis helper (RGB = XYZ), 0 = hide. */
WASMGL_KEEP void setAxisHelperVisible(int visible);

#ifdef __cplusplus
}
#endif

#endif
