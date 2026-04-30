#define LOGGING
// #define FPS

#include <memory>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>

#include "wasmgl_gl.h"
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "window/window.h"
#include "shaders/shader.h"
#include "camera/orbit_camera.h"
#include "grid/base_grid.h"
#include "complexobjects/csgmesh.h"
#include "threecsg/threebsp.h"
#include "core/mesh.h"
#include "primitives/box.h"
#include "primitives/sphere.h"
#include "primitives/pyramid.h"
#include "primitives/cylinder.h"
#include "primitives/torus.h"

#include "./core/functions.h"
#include "wasmgl_exports.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

Window mainWindow;
std::vector<std::shared_ptr<Mesh>> meshList;
/** UI labels: 1 cube, 2 sphere, 3 CSG, 4 pyramid, 5 cylinder, 6 torus, 7 cone (wasmgl_exports.h). */
std::vector<int> meshObjectKinds;
static int g_selectedIndex{-1};
/** Indices for constructive solid ops (union / subtract / intersect). Must differ. */
static int g_boolOperandA{-1};
static int g_boolOperandB{-1};

Shader litShader;
Shader lineShader;
BaseGridRenderer g_baseGrid;
OrbitCamera orbitCamera(glm::vec3(0.0f, 0.0f, 2.4f), glm::vec3(0.0f));
GLfloat deltaTime{0.0f};
GLfloat lastTime{0.0f};
int loops{0};
GLfloat totaltime{0};

static glm::mat4 g_projection{1.0f};
static float g_fovDegrees = 48.0f;

static void refreshProjection()
{
	GLfloat aspect = static_cast<GLfloat>(mainWindow.getBufferWidth()) /
									 std::max(1, mainWindow.getBufferHeight());
	g_projection = glm::perspective(glm::radians(g_fovDegrees), aspect, 0.1f, 100.0f);
}

#ifdef __EMSCRIPTEN__
static const GLchar *vLit =
		"#version 300 es                                          \n"
		"layout(location = 0) in vec3 pos;                        \n"
		"layout(location = 1) in vec3 normal;                   \n"
		"uniform mat4 model;                                    \n"
		"uniform mat4 view;                                     \n"
		"uniform mat4 projection;                               \n"
		"uniform mat3 normalMatrix;                             \n"
		"out vec3 vWorldPos;                                    \n"
		"out vec3 vNormal;                                      \n"
		"void main() {                                          \n"
		"  vec4 wp = model * vec4(pos, 1.0);                    \n"
		"  vWorldPos = wp.xyz;                                  \n"
		"  vNormal = normalMatrix * normal;                     \n"
		"  gl_Position = projection * view * wp;                \n"
		"}                                                      \n";

static const char *fLit =
		"#version 300 es                                          \n"
		"precision highp float;                                 \n"
		"in vec3 vWorldPos;                                     \n"
		"in vec3 vNormal;                                       \n"
		"uniform vec3 objectColor;                              \n"
		"uniform vec3 viewPos;                                  \n"
		"uniform vec3 lightDir;                                 \n"
		"uniform float ambientStrength;                         \n"
		"uniform float specularStrength;                        \n"
		"uniform float shininess;                               \n"
		"out vec4 fragColor;                                    \n"
		"void main() {                                          \n"
		"  vec3 N = normalize(vNormal);                         \n"
		"  vec3 L = normalize(lightDir);                       \n"
		"  vec3 V = normalize(viewPos - vWorldPos);            \n"
		"  vec3 H = normalize(L + V);                           \n"
		"  float ndl = max(dot(N, L), 0.0);                     \n"
		"  float ndh = max(dot(N, H), 0.0);                     \n"
		"  vec3 amb = ambientStrength * objectColor;            \n"
		"  vec3 diff = ndl * objectColor;                       \n"
		"  float spec = pow(ndh, shininess) * specularStrength;\n"
		"  vec3 col = amb + diff * 0.92 + vec3(spec * 0.55);    \n"
		"  fragColor = vec4(col, 1.0);                          \n"
		"}                                                      \n";

static const GLchar *vLine =
		"#version 300 es                                          \n"
		"uniform mat4 mvp;                                      \n"
		"layout(location = 0) in vec3 pos;                      \n"
		"void main() {                                          \n"
		"  gl_Position = mvp * vec4(pos, 1.0);                  \n"
		"}                                                      \n";

static const char *fLine =
		"#version 300 es                                          \n"
		"precision mediump float;                               \n"
		"out vec4 fragColor;                                    \n"
		"void main() { fragColor = vec4(0.06, 0.06, 0.08, 1.0); }\n";

#else

static const GLchar *vLit =
		"#version 330 core                                        \n"
		"layout(location = 0) in vec3 pos;                      \n"
		"layout(location = 1) in vec3 normal;                     \n"
		"uniform mat4 model;                                    \n"
		"uniform mat4 view;                                     \n"
		"uniform mat4 projection;                               \n"
		"uniform mat3 normalMatrix;                             \n"
		"out vec3 vWorldPos;                                    \n"
		"out vec3 vNormal;                                      \n"
		"void main() {                                          \n"
		"  vec4 wp = model * vec4(pos, 1.0);                    \n"
		"  vWorldPos = wp.xyz;                                  \n"
		"  vNormal = normalMatrix * normal;                     \n"
		"  gl_Position = projection * view * wp;                \n"
		"}                                                      \n";

static const char *fLit =
		"#version 330 core                                        \n"
		"in vec3 vWorldPos;                                     \n"
		"in vec3 vNormal;                                       \n"
		"uniform vec3 objectColor;                              \n"
		"uniform vec3 viewPos;                                  \n"
		"uniform vec3 lightDir;                                 \n"
		"uniform float ambientStrength;                         \n"
		"uniform float specularStrength;                        \n"
		"uniform float shininess;                               \n"
		"out vec4 fragColor;                                    \n"
		"void main() {                                          \n"
		"  vec3 N = normalize(vNormal);                         \n"
		"  vec3 L = normalize(lightDir);                         \n"
		"  vec3 V = normalize(viewPos - vWorldPos);              \n"
		"  vec3 H = normalize(L + V);                             \n"
		"  float ndl = max(dot(N, L), 0.0);                       \n"
		"  float ndh = max(dot(N, H), 0.0);                       \n"
		"  vec3 amb = ambientStrength * objectColor;            \n"
		"  vec3 diff = ndl * objectColor;                         \n"
		"  float spec = pow(ndh, shininess) * specularStrength; \n"
		"  vec3 col = amb + diff * 0.92 + vec3(spec * 0.55);    \n"
		"  fragColor = vec4(col, 1.0);                            \n"
		"}                                                      \n";

static const GLchar *vLine =
		"#version 330 core                                        \n"
		"uniform mat4 mvp;                                      \n"
		"layout(location = 0) in vec3 pos;                      \n"
		"void main() {                                          \n"
		"  gl_Position = mvp * vec4(pos, 1.0);                  \n"
		"}                                                      \n";

static const char *fLine =
		"#version 330 core                                        \n"
		"out vec4 fragColor;                                    \n"
		"void main() { fragColor = vec4(0.06, 0.06, 0.08, 1.0); }\n";

#endif

static glm::vec3 g_lightDirWorld;

void CreateShaders()
{
	litShader.CreateFromString(vLit, fLit);
	lineShader.CreateFromString(vLine, fLine);
	g_lightDirWorld = glm::normalize(glm::vec3(0.42f, 0.85f, 0.35f));
}

void emcmainloop(void *mainLoopArg);
void mainloop();

int main()
{
	mainWindow = Window(800, 600);
	mainWindow.Initialise();

	CreateShaders();
	g_baseGrid.initShaders();

	refreshProjection();

#ifdef __EMSCRIPTEN__
	int fps = 0;
	emscripten_set_main_loop_arg(emcmainloop, nullptr, fps, true);
#else
	while (!mainWindow.getShouldClose())
	{
		mainloop();
	}
#endif

	return 0;
}

void emcmainloop(void *mainLoopArg)
{
	(void)mainLoopArg;
	mainloop();
}

void mainloop()
{
	GLfloat currentTime = glfwGetTime();
	deltaTime = currentTime - lastTime;
	lastTime = currentTime;
	totaltime += deltaTime;
	loops++;
	GLfloat meanTime = loops > 0 ? totaltime / static_cast<GLfloat>(loops) : 0.0f;
	int fps = meanTime > 0.0f ? static_cast<int>(1.0f / meanTime) : 0;
#ifdef FPS
	std::cout << "FPS: " << fps << std::endl;
#endif

	glfwPollEvents();

	GLfloat const canvasW = static_cast<GLfloat>(std::max(1, mainWindow.getBufferWidth()));
	GLfloat const canvasH = static_cast<GLfloat>(std::max(1, mainWindow.getBufferHeight()));
	GLfloat const xDelta = mainWindow.getXChange();
	GLfloat const yDelta = mainWindow.getYChange();

	bool const leftDown = mainWindow.isLeftButtonPressed();
	bool const rightDown = mainWindow.isRightButtonPressed();
	orbitCamera.setDragging(leftDown || rightDown);

	orbitCamera.keyControl(mainWindow.getKeys(), deltaTime);
	if (leftDown)
		orbitCamera.applyRotatePixels(xDelta, yDelta, canvasW, canvasH);
	else if (rightDown)
		orbitCamera.applyPanPixels(xDelta, yDelta, canvasW, canvasH);
	orbitCamera.applyWheel(mainWindow.getScrollY());
	orbitCamera.setFovDegrees(g_fovDegrees);
	orbitCamera.update(deltaTime);

	refreshProjection();

	glClearColor(0.72f, 0.74f, 0.78f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 view = orbitCamera.calculateViewMatrix();
	g_baseGrid.render(g_projection, view);
	glm::vec3 viewPos = orbitCamera.getPosition();

	litShader.UseShader();

	glUniformMatrix4fv(litShader.GetProjectionLocation(), 1, GL_FALSE, glm::value_ptr(g_projection));
	glUniformMatrix4fv(litShader.GetViewLocation(), 1, GL_FALSE, glm::value_ptr(view));
	glUniform3fv(litShader.GetViewPosLocation(), 1, glm::value_ptr(viewPos));
	glUniform3fv(litShader.GetLightDirLocation(), 1, glm::value_ptr(g_lightDirWorld));
	glUniform1f(litShader.GetAmbientLocation(), 0.22f);
	glUniform1f(litShader.GetSpecStrengthLocation(), 0.65f);
	glUniform1f(litShader.GetShininessLocation(), 48.0f);

	for (size_t i = 0; i < meshList.size(); ++i)
	{
		auto const &mesh = meshList[i];
		glm::mat4 const model = *mesh->getModelMatrix();
		glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(model)));
		glUniformMatrix4fv(litShader.GetModelLocation(), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix3fv(litShader.GetNormalMatrixLocation(), 1, GL_FALSE, glm::value_ptr(normalMat));
		glm::vec3 col = mesh->getSolidColor();
		if (g_selectedIndex >= 0 && static_cast<int>(i) == g_selectedIndex)
			col = glm::min(col * 1.16f, glm::vec3(1.0f));
		glUniform3fv(litShader.GetObjectColorLocation(), 1, glm::value_ptr(col));
		mesh->RenderMesh();
	}

	lineShader.UseShader();
	/* GLES/WebGL2 has no GL_POLYGON_OFFSET_LINE; depth bias omitted for portability. */
	for (auto const &mesh : meshList)
	{
		glm::mat4 mvp = g_projection * view * (*mesh->getModelMatrix());
		glUniformMatrix4fv(lineShader.GetMVPLocation(), 1, GL_FALSE, glm::value_ptr(mvp));
		mesh->RenderWireframe();
	}

	glUseProgram(0);
	mainWindow.swapBuffers();
}

extern "C" {

void addCube(float width, float height, float depth)
{
	GLfloat const w = std::max(1e-4f, width);
	GLfloat const h = std::max(1e-4f, height);
	GLfloat const d = std::max(1e-4f, depth);
	auto cube = createBox(BoxDimensions{w, h, d});
	cube->setSolidColor(glm::vec3(0.92f, 0.48f, 0.18f));
	cube->rotate(glm::vec3(0.0f, glm::radians(45.0f), 0.0f));
	cube->computeThreeBSP();
	meshList.push_back(cube);
	meshObjectKinds.push_back(1);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
}

void addSphere(float radius, int widthSeg, int heightSeg)
{
	int const ws = std::max(3, widthSeg);
	int const hs = std::max(2, heightSeg);
	auto sphere = createSphere(
			SphereDimensions{std::max(0.01f, radius)},
			SphereParameters{ws, hs, 0.0f, 2.0f * static_cast<float>(M_PI), 0.0f, static_cast<float>(M_PI)});
	sphere->setSolidColor(glm::vec3(0.22f, 0.52f, 0.95f));
	meshList.push_back(sphere);
	meshObjectKinds.push_back(2);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
}

void addPyramid(float side, float height)
{
	GLfloat const s = std::max(1e-4f, side);
	GLfloat const h = std::max(1e-4f, height);
	auto pyr = createPyramid(PyramidDimensions{s, h});
	pyr->setSolidColor(glm::vec3(0.75f, 0.55f, 0.22f));
	meshList.push_back(pyr);
	meshObjectKinds.push_back(4);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
}

void addCylinder(float radiusBottom, float radiusTop, float height, int radialSeg, int heightSeg)
{
	int const rseg = std::max(3, radialSeg);
	int const hseg = std::max(1, heightSeg);
	auto cyl = createCylinder(
			CylinderDimensions{std::max(1e-4f, radiusBottom), std::max(1e-4f, radiusTop), std::max(1e-4f, height)},
			CylinderParameters{rseg, hseg});
	cyl->setSolidColor(glm::vec3(0.24f, 0.78f, 0.45f));
	meshList.push_back(cyl);
	meshObjectKinds.push_back(5);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
}

void addCone(float radius, float height, int radialSeg, int heightSeg)
{
	int const rseg = std::max(3, radialSeg);
	int const hseg = std::max(1, heightSeg);
	auto cone = createCylinder(
			CylinderDimensions{std::max(1e-4f, radius), 0.0f, std::max(1e-4f, height)},
			CylinderParameters{rseg, hseg});
	cone->setSolidColor(glm::vec3(0.95f, 0.42f, 0.28f));
	meshList.push_back(cone);
	meshObjectKinds.push_back(7);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
}

void addTorus(float majorRadius, float minorRadius, int radialSeg, int tubularSeg)
{
	int const rs = std::max(3, radialSeg);
	int const ts = std::max(3, tubularSeg);
	auto t = createTorus(
			TorusDimensions{std::max(1e-4f, majorRadius), std::max(1e-4f, minorRadius)},
			TorusParameters{rs, ts});
	t->setSolidColor(glm::vec3(0.72f, 0.38f, 0.88f));
	meshList.push_back(t);
	meshObjectKinds.push_back(6);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
}

int getSceneObjectCount(void)
{
	return static_cast<int>(meshList.size());
}

void setSelectedObjectIndex(int idx)
{
	if (meshList.empty())
	{
		g_selectedIndex = -1;
		return;
	}
	if (idx < 0 || idx >= static_cast<int>(meshList.size()))
		g_selectedIndex = -1;
	else
		g_selectedIndex = idx;
}

int getSelectedObjectIndex(void)
{
	return g_selectedIndex;
}

int getObjectKind(int index)
{
	if (index < 0 || index >= static_cast<int>(meshObjectKinds.size()))
		return 0;
	return meshObjectKinds[static_cast<size_t>(index)];
}

float getSelectedBoxWidth(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0.0f;
	auto b = std::dynamic_pointer_cast<Box>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!b)
		return 0.0f;
	return b->getDimensions().width;
}

float getSelectedBoxHeight(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0.0f;
	auto b = std::dynamic_pointer_cast<Box>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!b)
		return 0.0f;
	return b->getDimensions().height;
}

float getSelectedBoxDepth(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0.0f;
	auto b = std::dynamic_pointer_cast<Box>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!b)
		return 0.0f;
	return b->getDimensions().depth;
}

float getSelectedSphereRadius(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0.0f;
	auto s = std::dynamic_pointer_cast<Sphere>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!s)
		return 0.0f;
	return s->getDimensions().radius;
}

int getSelectedSphereWidthSegments(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0;
	auto s = std::dynamic_pointer_cast<Sphere>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!s)
		return 0;
	return s->getParameters().widthSegments;
}

int getSelectedSphereHeightSegments(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0;
	auto s = std::dynamic_pointer_cast<Sphere>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!s)
		return 0;
	return s->getParameters().heightSegments;
}

void resizeSelectedBox(float width, float height, float depth)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return;
	auto b = std::dynamic_pointer_cast<Box>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!b)
		return;
	GLfloat const w = std::max(1e-4f, width);
	GLfloat const h = std::max(1e-4f, height);
	GLfloat const d = std::max(1e-4f, depth);
	b->setDimensions(BoxDimensions{w, h, d});
	b->rebuildGeometry();
}

void resizeSelectedSphere(float radius, int widthSeg, int heightSeg)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return;
	auto s = std::dynamic_pointer_cast<Sphere>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!s)
		return;
	int const ws = std::max(3, widthSeg);
	int const hs = std::max(2, heightSeg);
	s->setDimensions(SphereDimensions{std::max(0.01f, radius)});
	SphereParameters p = s->getParameters();
	p.widthSegments = ws;
	p.heightSegments = hs;
	s->setParameters(p);
	s->rebuildGeometry();
}

float getSelectedPyramidSide(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0.0f;
	auto p = std::dynamic_pointer_cast<Pyramid>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!p)
		return 0.0f;
	return p->getDimensions().side;
}

float getSelectedPyramidHeight(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0.0f;
	auto p = std::dynamic_pointer_cast<Pyramid>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!p)
		return 0.0f;
	return p->getDimensions().height;
}

void resizeSelectedPyramid(float side, float height)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return;
	auto p = std::dynamic_pointer_cast<Pyramid>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!p)
		return;
	p->setDimensions(PyramidDimensions{std::max(1e-4f, side), std::max(1e-4f, height)});
	p->rebuildGeometry();
}

float getSelectedCylinderRadiusBottom(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0.0f;
	auto c = std::dynamic_pointer_cast<Cylinder>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!c)
		return 0.0f;
	return c->getDimensions().radiusBottom;
}

float getSelectedCylinderRadiusTop(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0.0f;
	auto c = std::dynamic_pointer_cast<Cylinder>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!c)
		return 0.0f;
	return c->getDimensions().radiusTop;
}

float getSelectedCylinderHeight(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0.0f;
	auto c = std::dynamic_pointer_cast<Cylinder>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!c)
		return 0.0f;
	return c->getDimensions().height;
}

int getSelectedCylinderRadialSegments(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0;
	auto c = std::dynamic_pointer_cast<Cylinder>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!c)
		return 0;
	return c->getParameters().radialSegments;
}

int getSelectedCylinderHeightSegments(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0;
	auto c = std::dynamic_pointer_cast<Cylinder>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!c)
		return 0;
	return c->getParameters().heightSegments;
}

void resizeSelectedCylinder(float radiusBottom, float radiusTop, float height, int radialSeg, int heightSeg)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return;
	auto c = std::dynamic_pointer_cast<Cylinder>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!c)
		return;
	int const rseg = std::max(3, radialSeg);
	int const hseg = std::max(1, heightSeg);
	c->setDimensions(CylinderDimensions{std::max(1e-4f, radiusBottom), std::max(1e-4f, radiusTop), std::max(1e-4f, height)});
	CylinderParameters p = c->getParameters();
	p.radialSegments = rseg;
	p.heightSegments = hseg;
	c->setParameters(p);
	c->rebuildGeometry();
}

float getSelectedTorusMajorRadius(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0.0f;
	auto t = std::dynamic_pointer_cast<Torus>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!t)
		return 0.0f;
	return t->getDimensions().majorRadius;
}

float getSelectedTorusMinorRadius(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0.0f;
	auto t = std::dynamic_pointer_cast<Torus>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!t)
		return 0.0f;
	return t->getDimensions().minorRadius;
}

int getSelectedTorusRadialSegments(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0;
	auto t = std::dynamic_pointer_cast<Torus>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!t)
		return 0;
	return t->getParameters().radialSegments;
}

int getSelectedTorusTubularSegments(void)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return 0;
	auto t = std::dynamic_pointer_cast<Torus>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!t)
		return 0;
	return t->getParameters().tubularSegments;
}

void resizeSelectedTorus(float majorRadius, float minorRadius, int radialSeg, int tubularSeg)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return;
	auto t = std::dynamic_pointer_cast<Torus>(meshList[static_cast<size_t>(g_selectedIndex)]);
	if (!t)
		return;
	int const rs = std::max(3, radialSeg);
	int const ts = std::max(3, tubularSeg);
	t->setDimensions(TorusDimensions{std::max(1e-4f, majorRadius), std::max(1e-4f, minorRadius)});
	TorusParameters p = t->getParameters();
	p.radialSegments = rs;
	p.tubularSegments = ts;
	t->setParameters(p);
	t->rebuildGeometry();
}

void nudgeSelectedTranslate(float dx, float dy, float dz)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return;
	auto mesh = meshList[static_cast<size_t>(g_selectedIndex)];
	glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(dx, dy, dz));
	*mesh->getModelMatrix() = T * (*mesh->getModelMatrix());
	mesh->computeThreeBSP();
}

void nudgeSelectedRotateDegrees(float rxDeg, float ryDeg, float rzDeg)
{
	if (g_selectedIndex < 0 || g_selectedIndex >= static_cast<int>(meshList.size()))
		return;
	auto mesh = meshList[static_cast<size_t>(g_selectedIndex)];
	mesh->rotate(glm::vec3(glm::radians(rxDeg), glm::radians(ryDeg), glm::radians(rzDeg)));
	mesh->computeThreeBSP();
}

void setBooleanOperandA(int idx)
{
	if (meshList.empty())
	{
		g_boolOperandA = -1;
		return;
	}
	if (idx < 0 || idx >= static_cast<int>(meshList.size()))
		g_boolOperandA = -1;
	else
		g_boolOperandA = idx;
}

void setBooleanOperandB(int idx)
{
	if (meshList.empty())
	{
		g_boolOperandB = -1;
		return;
	}
	if (idx < 0 || idx >= static_cast<int>(meshList.size()))
		g_boolOperandB = -1;
	else
		g_boolOperandB = idx;
}

int getBooleanOperandA(void)
{
	return g_boolOperandA;
}

int getBooleanOperandB(void)
{
	return g_boolOperandB;
}

static void applyBooleanResult(std::shared_ptr<Mesh> res, int indexA, int indexB)
{
	if (!res)
		return;
	int const lo = std::min(indexA, indexB);
	int const hi = std::max(indexA, indexB);
	meshList.erase(meshList.begin() + hi);
	meshObjectKinds.erase(meshObjectKinds.begin() + hi);
	meshList.erase(meshList.begin() + lo);
	meshObjectKinds.erase(meshObjectKinds.begin() + lo);
	res->setSolidColor(glm::vec3(0.30f, 0.72f, 0.48f));
	meshList.push_back(res);
	meshObjectKinds.push_back(3);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
	g_boolOperandA = -1;
	g_boolOperandB = -1;
}

void performBooleanUnion(void)
{
	int const n = static_cast<int>(meshList.size());
	int const a = g_boolOperandA;
	int const b = g_boolOperandB;
	if (a < 0 || b < 0 || a == b || a >= n || b >= n)
		return;
	auto ma = meshList[static_cast<size_t>(a)];
	auto mb = meshList[static_cast<size_t>(b)];
	applyBooleanResult(ma->add(mb), a, b);
}

void performBooleanDifference(void)
{
	int const n = static_cast<int>(meshList.size());
	int const a = g_boolOperandA;
	int const b = g_boolOperandB;
	if (a < 0 || b < 0 || a == b || a >= n || b >= n)
		return;
	auto ma = meshList[static_cast<size_t>(a)];
	auto mb = meshList[static_cast<size_t>(b)];
	applyBooleanResult(ma->subtract(mb), a, b);
}

void performBooleanIntersection(void)
{
	int const n = static_cast<int>(meshList.size());
	int const a = g_boolOperandA;
	int const b = g_boolOperandB;
	if (a < 0 || b < 0 || a == b || a >= n || b >= n)
		return;
	auto ma = meshList[static_cast<size_t>(a)];
	auto mb = meshList[static_cast<size_t>(b)];
	applyBooleanResult(ma->intersect(mb), a, b);
}

void cameraZoomIn(void)
{
	orbitCamera.zoomInButton();
}

void cameraZoomOut(void)
{
	orbitCamera.zoomOutButton();
}

void cameraNudgeViewYawDegrees(float deltaDeg)
{
	orbitCamera.nudgeViewYawDegrees(deltaDeg);
}

void cameraNudgeViewPitchDegrees(float deltaDeg)
{
	orbitCamera.nudgeViewPitchDegrees(deltaDeg);
}

void cameraNudgePositionView(float alongFront, float alongRight, float alongUp)
{
	orbitCamera.nudgePositionView(alongFront, alongRight, alongUp);
}

void setBaseGridVisible(int visible)
{
	g_baseGrid.setVisible(visible != 0);
}

}
