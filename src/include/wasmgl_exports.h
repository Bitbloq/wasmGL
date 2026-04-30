/**
 * JavaScript / WebAssembly boundary — stable C ABI for the demo embed.
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
/** Y-axis cylinder / truncated cylinder; equal radii = right cylinder. */
WASMGL_KEEP void addCylinder(float radiusBottom, float radiusTop, float height, int radialSeg, int heightSeg);
/** Solid cone (apex up): same mesh as cylinder with top radius 0 (Three.js ConeGeometry-style). */
WASMGL_KEEP void addCone(float radius, float height, int radialSeg, int heightSeg);
/** Ring in XY plane; major = hole-to-tube-center, minor = tube radius. */
WASMGL_KEEP void addTorus(float majorRadius, float minorRadius, int radialSeg, int tubularSeg);

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

WASMGL_KEEP float getSelectedBoxWidth(void);
WASMGL_KEEP float getSelectedBoxHeight(void);
WASMGL_KEEP float getSelectedBoxDepth(void);
WASMGL_KEEP float getSelectedSphereRadius(void);
WASMGL_KEEP int getSelectedSphereWidthSegments(void);
WASMGL_KEEP int getSelectedSphereHeightSegments(void);
WASMGL_KEEP float getSelectedPyramidSide(void);
WASMGL_KEEP float getSelectedPyramidHeight(void);
WASMGL_KEEP float getSelectedCylinderRadiusBottom(void);
WASMGL_KEEP float getSelectedCylinderRadiusTop(void);
WASMGL_KEEP float getSelectedCylinderHeight(void);
WASMGL_KEEP int getSelectedCylinderRadialSegments(void);
WASMGL_KEEP int getSelectedCylinderHeightSegments(void);
WASMGL_KEEP float getSelectedTorusMajorRadius(void);
WASMGL_KEEP float getSelectedTorusMinorRadius(void);
WASMGL_KEEP int getSelectedTorusRadialSegments(void);
WASMGL_KEEP int getSelectedTorusTubularSegments(void);

WASMGL_KEEP void resizeSelectedBox(float width, float height, float depth);
WASMGL_KEEP void resizeSelectedSphere(float radius, int widthSeg, int heightSeg);
WASMGL_KEEP void resizeSelectedPyramid(float side, float height);
WASMGL_KEEP void resizeSelectedCylinder(float radiusBottom, float radiusTop, float height, int radialSeg, int heightSeg);
WASMGL_KEEP void resizeSelectedTorus(float majorRadius, float minorRadius, int radialSeg, int tubularSeg);

WASMGL_KEEP void nudgeSelectedTranslate(float dx, float dy, float dz);
WASMGL_KEEP void nudgeSelectedRotateDegrees(float rxDeg, float ryDeg, float rzDeg);

WASMGL_KEEP void setBooleanOperandA(int idx);
WASMGL_KEEP void setBooleanOperandB(int idx);
WASMGL_KEEP int getBooleanOperandA(void);
WASMGL_KEEP int getBooleanOperandB(void);
WASMGL_KEEP void performBooleanUnion(void);
WASMGL_KEEP void performBooleanDifference(void);
WASMGL_KEEP void performBooleanIntersection(void);

/** 1 = show Bitbloq-style base grid (XY plane), 0 = hide. */
WASMGL_KEEP void setBaseGridVisible(int visible);

/** 1 = show bottom-right world axis helper (RGB = XYZ), 0 = hide. */
WASMGL_KEEP void setAxisHelperVisible(int visible);

#ifdef __cplusplus
}
#endif

#endif
