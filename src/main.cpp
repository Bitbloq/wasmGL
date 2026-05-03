#define LOGGING
// #define FPS

#include <memory>
#include <algorithm>
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
#include "overlay/axis_helper.h"
#include "overlay/navigation_cube.h"
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
enum ObjectKind
{
	KIND_BOX = 1,
	KIND_SPHERE = 2,
	KIND_CSG = 3,
	KIND_PYRAMID = 4,
	KIND_CYLINDER = 5,
	KIND_TORUS = 6,
	KIND_CONE = 7,
};

enum TransformOperationType
{
	TRANSFORM_TRANSLATION = 1,
	TRANSFORM_ROTATION = 2,
};

enum TransformReferenceFrame
{
	TRANSFORM_FRAME_SCENE = 1,
	TRANSFORM_FRAME_OBJECT = 2,
};

enum BooleanOperation
{
	BOOLEAN_UNION = 1,
	BOOLEAN_DIFFERENCE = 2,
	BOOLEAN_INTERSECTION = 3,
};

struct TransformOperationState
{
	int type{TRANSFORM_TRANSLATION};
	int frame{TRANSFORM_FRAME_SCENE};
	glm::vec3 values{0.0f};
	glm::mat4 cachedModel{1.0f};
};

struct MeshObjectState
{
	int kind{0};
	int serialId{0};
	glm::mat4 baseModel{1.0f};
	std::vector<TransformOperationState> operations;
};

std::vector<MeshObjectState> meshObjectStates;
static int g_nextObjectSerialId{1};
static int g_selectedIndex{-1};
/** Engine-owned multi-operand CSG selection, stored as serial ids so indices can shift while reducing. */
static std::vector<int> g_boolSelectionSerials;

Shader litShader;
Shader lineShader;
Shader selectedAxisShader;
GLuint g_selectedAxisVAO{0};
GLuint g_selectedAxisVBO{0};
BaseGridRenderer g_baseGrid;
AxisHelperOverlay g_axisHelper;
NavigationCubeOverlay g_navigationCube;
OrbitCamera orbitCamera(glm::vec3(13.5f, 13.5f, 15.5f), glm::vec3(0.0f));

namespace
{
bool validMeshIndex(int idx)
{
	return idx >= 0 && idx < static_cast<int>(meshList.size());
}

bool validObjectStateIndex(int idx)
{
	return validMeshIndex(idx) && idx < static_cast<int>(meshObjectStates.size());
}

int indexBySerialId(int serialId)
{
	for (size_t i = 0; i < meshObjectStates.size(); ++i)
	{
		if (meshObjectStates[i].serialId == serialId)
			return static_cast<int>(i);
	}
	return -1;
}

std::shared_ptr<Mesh> meshAt(int index)
{
	if (!validMeshIndex(index))
		return nullptr;
	return meshList[static_cast<size_t>(index)];
}

template <typename T>
std::shared_ptr<T> objectAs(int index)
{
	auto mesh = meshAt(index);
	return mesh ? std::dynamic_pointer_cast<T>(mesh) : nullptr;
}

glm::mat4 operationMatrix(TransformOperationState const &op)
{
	if (op.type == TRANSFORM_ROTATION)
	{
		glm::mat4 R(1.0f);
		R = glm::rotate(R, glm::radians(op.values.x), glm::vec3(1.0f, 0.0f, 0.0f));
		R = glm::rotate(R, glm::radians(op.values.y), glm::vec3(0.0f, 1.0f, 0.0f));
		R = glm::rotate(R, glm::radians(op.values.z), glm::vec3(0.0f, 0.0f, 1.0f));
		return R;
	}
	return glm::translate(glm::mat4(1.0f), op.values);
}

void applyTransformCacheFrom(int objectIndex, int firstDirtyOp)
{
	if (!validObjectStateIndex(objectIndex))
		return;

	auto &state = meshObjectStates[static_cast<size_t>(objectIndex)];
	int const count = static_cast<int>(state.operations.size());
	int const start = std::max(0, std::min(firstDirtyOp, count));
	glm::mat4 running = start > 0
													? state.operations[static_cast<size_t>(start - 1)].cachedModel
													: state.baseModel;

	for (int i = start; i < count; ++i)
	{
		auto &op = state.operations[static_cast<size_t>(i)];
		glm::mat4 const opMat = operationMatrix(op);
		running = op.frame == TRANSFORM_FRAME_OBJECT ? running * opMat : opMat * running;
		op.cachedModel = running;
	}

	if (count == 0)
		running = state.baseModel;

	*meshList[static_cast<size_t>(objectIndex)]->getModelMatrix() = running;
	meshList[static_cast<size_t>(objectIndex)]->threeBSPDone = false;
}

void registerMeshState(int kind, std::shared_ptr<Mesh> const &mesh, glm::mat4 const *baseOverride = nullptr)
{
	MeshObjectState state;
	state.kind = kind;
	state.serialId = g_nextObjectSerialId++;
	state.baseModel = baseOverride ? *baseOverride : *mesh->getModelMatrix();
	meshObjectStates.push_back(state);
}

int addTransformOperation(int objectIndex, int type, int frame = TRANSFORM_FRAME_SCENE)
{
	if (!validObjectStateIndex(objectIndex))
		return -1;
	auto &ops = meshObjectStates[static_cast<size_t>(objectIndex)].operations;
	TransformOperationState op;
	op.type = type == TRANSFORM_ROTATION ? TRANSFORM_ROTATION : TRANSFORM_TRANSLATION;
	op.frame = frame == TRANSFORM_FRAME_OBJECT ? TRANSFORM_FRAME_OBJECT : TRANSFORM_FRAME_SCENE;
	ops.push_back(op);
	int const opIndex = static_cast<int>(ops.size()) - 1;
	applyTransformCacheFrom(objectIndex, opIndex);
	return opIndex;
}

void setTransformOperationFullValue(int objectIndex, int opIndex, int type, int frame, float x, float y, float z)
{
	if (!validObjectStateIndex(objectIndex))
		return;
	auto &ops = meshObjectStates[static_cast<size_t>(objectIndex)].operations;
	if (opIndex < 0 || opIndex >= static_cast<int>(ops.size()))
		return;
	auto &op = ops[static_cast<size_t>(opIndex)];
	op.type = type == TRANSFORM_ROTATION ? TRANSFORM_ROTATION : TRANSFORM_TRANSLATION;
	op.frame = frame == TRANSFORM_FRAME_OBJECT ? TRANSFORM_FRAME_OBJECT : TRANSFORM_FRAME_SCENE;
	op.values = glm::vec3(x, y, z);
	applyTransformCacheFrom(objectIndex, opIndex);
}

void pruneBooleanSelection()
{
	std::vector<int> pruned;
	pruned.reserve(g_boolSelectionSerials.size());
	for (int serial : g_boolSelectionSerials)
	{
		if (indexBySerialId(serial) >= 0 && std::find(pruned.begin(), pruned.end(), serial) == pruned.end())
			pruned.push_back(serial);
	}
	g_boolSelectionSerials.swap(pruned);
}

void removeTransformOperation(int objectIndex, int opIndex)
{
	if (!validObjectStateIndex(objectIndex))
		return;
	auto &ops = meshObjectStates[static_cast<size_t>(objectIndex)].operations;
	if (opIndex < 0 || opIndex >= static_cast<int>(ops.size()))
		return;
	ops.erase(ops.begin() + opIndex);
	applyTransformCacheFrom(objectIndex, opIndex);
}
} // namespace
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

#ifdef __EMSCRIPTEN__
static const GLchar *vSelectedAxis =
		"#version 300 es                                          \n"
		"uniform mat4 mvp;                                      \n"
		"layout(location = 0) in vec3 pos;                      \n"
		"layout(location = 1) in vec3 color;                    \n"
		"out vec3 vColor;                                       \n"
		"void main() {                                          \n"
		"  vColor = color;                                      \n"
		"  gl_Position = mvp * vec4(pos, 1.0);                  \n"
		"}                                                      \n";

static const GLchar *fSelectedAxis =
		"#version 300 es                                          \n"
		"precision mediump float;                               \n"
		"in vec3 vColor;                                        \n"
		"out vec4 fragColor;                                    \n"
		"void main() { fragColor = vec4(vColor, 1.0); }        \n";
#else
static const GLchar *vSelectedAxis =
		"#version 330 core                                        \n"
		"uniform mat4 mvp;                                      \n"
		"layout(location = 0) in vec3 pos;                      \n"
		"layout(location = 1) in vec3 color;                    \n"
		"out vec3 vColor;                                       \n"
		"void main() {                                          \n"
		"  vColor = color;                                      \n"
		"  gl_Position = mvp * vec4(pos, 1.0);                  \n"
		"}                                                      \n";

static const GLchar *fSelectedAxis =
		"#version 330 core                                        \n"
		"in vec3 vColor;                                        \n"
		"out vec4 fragColor;                                    \n"
		"void main() { fragColor = vec4(vColor, 1.0); }        \n";
#endif

static glm::vec3 g_lightDirWorld;

void CreateShaders()
{
	litShader.CreateFromString(vLit, fLit);
	lineShader.CreateFromString(vLine, fLine);
	selectedAxisShader.CreateFromString(vSelectedAxis, fSelectedAxis);
	glGenVertexArrays(1, &g_selectedAxisVAO);
	glGenBuffers(1, &g_selectedAxisVBO);
	glBindVertexArray(g_selectedAxisVAO);
	glBindBuffer(GL_ARRAY_BUFFER, g_selectedAxisVBO);
	glBufferData(GL_ARRAY_BUFFER, 6 * 6 * sizeof(GLfloat), nullptr, GL_DYNAMIC_DRAW);
	GLsizei const stride = 6 * static_cast<GLsizei>(sizeof(GLfloat));
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
	/** Mostly downward / -Z-ish lighting so Z-up solids read clearly. */
	g_lightDirWorld = glm::normalize(glm::vec3(0.28f, 0.22f, 0.92f));
}

void emcmainloop(void *mainLoopArg);
void mainloop();

int main()
{
	mainWindow = Window(800, 600);
	mainWindow.Initialise();

	CreateShaders();
	g_baseGrid.initShaders();
	g_axisHelper.init();
	g_navigationCube.init();

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

void renderSelectedObjectAxes(glm::mat4 const &view)
{
	if (!validMeshIndex(g_selectedIndex) || g_selectedIndex >= static_cast<int>(meshObjectStates.size()))
		return;
	if (g_selectedAxisVAO == 0 || g_selectedAxisVBO == 0)
		return;

	auto const &mesh = meshList[static_cast<size_t>(g_selectedIndex)];
	glm::vec3 localMin(0.0f);
	glm::vec3 localMax(0.0f);
	if (!mesh->getLocalBounds(localMin, localMax))
		return;

	glm::vec3 const extent = localMax - localMin;
	float const largestExtent = std::max(extent.x, std::max(extent.y, extent.z));
	float const axisLen = std::max(0.5f, largestExtent * 1.15f);

	GLfloat const interleaved[] = {
			0.0f, 0.0f, 0.0f, 1.0f, 0.08f, 0.06f, axisLen, 0.0f, 0.0f, 1.0f, 0.08f, 0.06f,
			0.0f, 0.0f, 0.0f, 0.16f, 0.95f, 0.32f, 0.0f, axisLen, 0.0f, 0.16f, 0.95f, 0.32f,
			0.0f, 0.0f, 0.0f, 0.16f, 0.44f, 1.0f, 0.0f, 0.0f, axisLen, 0.16f, 0.44f, 1.0f,
	};

	GLboolean depthWasEnabled;
	glGetBooleanv(GL_DEPTH_TEST, &depthWasEnabled);
	glDisable(GL_DEPTH_TEST);

	glm::mat4 const mvp = g_projection * view * (*mesh->getModelMatrix());
	selectedAxisShader.UseShader();
	GLint locMvp = selectedAxisShader.GetMVPLocation();
	if (locMvp < 0)
		locMvp = glGetUniformLocation(selectedAxisShader.GetProgramId(), "mvp");
	if (locMvp >= 0)
		glUniformMatrix4fv(locMvp, 1, GL_FALSE, glm::value_ptr(mvp));

	glBindVertexArray(g_selectedAxisVAO);
	glBindBuffer(GL_ARRAY_BUFFER, g_selectedAxisVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(interleaved), interleaved);
	glLineWidth(3.0f);
	glDrawArrays(GL_LINES, 0, 6);
	glLineWidth(1.0f);
	glBindVertexArray(0);
	glUseProgram(0);

	if (depthWasEnabled)
		glEnable(GL_DEPTH_TEST);
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

	mainWindow.syncFramebufferSize();

	GLfloat const canvasW = static_cast<GLfloat>(std::max(1, mainWindow.getBufferWidth()));
	GLfloat const canvasH = static_cast<GLfloat>(std::max(1, mainWindow.getBufferHeight()));
	GLfloat const xDelta = mainWindow.getXChange();
	GLfloat const yDelta = mainWindow.getYChange();

	bool const leftDown = mainWindow.isLeftButtonPressed();
	bool const rightDown = mainWindow.isRightButtonPressed();

	static bool s_prevLeftDown = false;
	static bool s_suppressOrbitLeftDrag = false;
	GLfloat cursorFbX{};
	GLfloat cursorFbY{};
	NavigationCubeOverlay::cursorFramebufferPixels(mainWindow.getGLFWWindow(), cursorFbX, cursorFbY);
	g_navigationCube.syncHover(orbitCamera, cursorFbX, cursorFbY, canvasW, canvasH);
	if (!leftDown)
		s_suppressOrbitLeftDrag = false;
	if (leftDown && !s_prevLeftDown)
	{
		float navTheta{};
		float navPhi{};
		if (g_navigationCube.pick(orbitCamera, cursorFbX, cursorFbY, canvasW, canvasH, &navTheta, &navPhi))
		{
			orbitCamera.snapOrbitToAngles(navTheta, navPhi, true);
			s_suppressOrbitLeftDrag = true;
		}
	}
	s_prevLeftDown = leftDown;

	orbitCamera.setDragging((leftDown || rightDown) && !s_suppressOrbitLeftDrag);

	orbitCamera.keyControl(mainWindow.getKeys(), deltaTime);
	if (leftDown && !s_suppressOrbitLeftDrag)
		orbitCamera.applyRotatePixels(xDelta, yDelta, canvasW, canvasH);
	else if (rightDown)
		orbitCamera.applyPanPixels(xDelta, yDelta, canvasW, canvasH);
	orbitCamera.applyWheel(mainWindow.getScrollY());
	orbitCamera.setFovDegrees(g_fovDegrees);
	orbitCamera.update(deltaTime);

	refreshProjection();

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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
	renderSelectedObjectAxes(view);
	g_axisHelper.render(orbitCamera);
	g_navigationCube.render(orbitCamera);

	mainWindow.swapBuffers();
}

extern "C" {

unsigned int addCube(float width, float height, float depth)
{
	GLfloat const w = std::max(1e-4f, width);
	GLfloat const h = std::max(1e-4f, height);
	GLfloat const d = std::max(1e-4f, depth);
	auto cube = createBox(BoxDimensions{w, h, d});
	cube->setSolidColor(glm::vec3(0.92f, 0.48f, 0.18f));
	meshList.push_back(cube);
	registerMeshState(KIND_BOX, cube);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
	return static_cast<unsigned int>(meshObjectStates.back().serialId);
}

unsigned int addSphere(float radius, int widthSeg, int heightSeg)
{
	int const ws = std::max(3, widthSeg);
	int const hs = std::max(2, heightSeg);
	auto sphere = createSphere(
			SphereDimensions{std::max(0.01f, radius)},
			SphereParameters{ws, hs, 0.0f, 2.0f * static_cast<float>(M_PI), 0.0f, static_cast<float>(M_PI)});
	sphere->setSolidColor(glm::vec3(0.22f, 0.52f, 0.95f));
	meshList.push_back(sphere);
	registerMeshState(KIND_SPHERE, sphere);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
	return static_cast<unsigned int>(meshObjectStates.back().serialId);
}

unsigned int addPyramid(float side, float height)
{
	GLfloat const s = std::max(1e-4f, side);
	GLfloat const h = std::max(1e-4f, height);
	auto pyr = createPyramid(PyramidDimensions{s, h});
	pyr->setSolidColor(glm::vec3(0.75f, 0.55f, 0.22f));
	meshList.push_back(pyr);
	registerMeshState(KIND_PYRAMID, pyr);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
	return static_cast<unsigned int>(meshObjectStates.back().serialId);
}

unsigned int addCylinder(float radiusBottom, float radiusTop, float height, int radialSeg, int heightSeg)
{
	int const rseg = std::max(3, radialSeg);
	int const hseg = std::max(1, heightSeg);
	auto cyl = createCylinder(
			CylinderDimensions{std::max(1e-4f, radiusBottom), std::max(1e-4f, radiusTop), std::max(1e-4f, height)},
			CylinderParameters{rseg, hseg});
	cyl->setSolidColor(glm::vec3(0.24f, 0.78f, 0.45f));
	meshList.push_back(cyl);
	registerMeshState(KIND_CYLINDER, cyl);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
	return static_cast<unsigned int>(meshObjectStates.back().serialId);
}

unsigned int addCone(float radius, float height, int radialSeg, int heightSeg)
{
	int const rseg = std::max(3, radialSeg);
	int const hseg = std::max(1, heightSeg);
	auto cone = createCylinder(
			CylinderDimensions{std::max(1e-4f, radius), 0.0f, std::max(1e-4f, height)},
			CylinderParameters{rseg, hseg});
	cone->setSolidColor(glm::vec3(0.95f, 0.42f, 0.28f));
	meshList.push_back(cone);
	registerMeshState(KIND_CONE, cone);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
	return static_cast<unsigned int>(meshObjectStates.back().serialId);
}

unsigned int addTorus(float majorRadius, float minorRadius, int radialSeg, int tubularSeg)
{
	int const rs = std::max(3, radialSeg);
	int const ts = std::max(3, tubularSeg);
	auto t = createTorus(
			TorusDimensions{std::max(1e-4f, majorRadius), std::max(1e-4f, minorRadius)},
			TorusParameters{rs, ts});
	t->setSolidColor(glm::vec3(0.72f, 0.38f, 0.88f));
	meshList.push_back(t);
	registerMeshState(KIND_TORUS, t);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
	return static_cast<unsigned int>(meshObjectStates.back().serialId);
}

unsigned int addDefaultObject(int kind)
{
	switch (kind)
	{
	case KIND_BOX:
		return addCube(1.0f, 1.0f, 1.0f);
	case KIND_SPHERE:
		return addSphere(0.5f, 18, 18);
	case KIND_PYRAMID:
		return addPyramid(1.0f, 1.0f);
	case KIND_CYLINDER:
		return addCylinder(0.5f, 0.5f, 1.0f, 24, 1);
	case KIND_TORUS:
		return addTorus(0.35f, 0.15f, 24, 32);
	case KIND_CONE:
		return addCone(0.5f, 1.0f, 24, 1);
	default:
		return 0;
	}
}

int getSceneObjectCount(void)
{
	return static_cast<int>(meshList.size());
}

void setSelectedObjectId(unsigned int serialId)
{
	if (serialId == 0 || meshList.empty())
	{
		g_selectedIndex = -1;
		return;
	}
	int const idx = indexBySerialId(static_cast<int>(serialId));
	g_selectedIndex = idx >= 0 ? idx : -1;
}

unsigned int getSelectedObjectId(void)
{
	if (!validMeshIndex(g_selectedIndex) || g_selectedIndex >= static_cast<int>(meshObjectStates.size()))
		return 0;
	return static_cast<unsigned int>(meshObjectStates[static_cast<size_t>(g_selectedIndex)].serialId);
}

int getObjectKind(int index)
{
	if (index < 0 || index >= static_cast<int>(meshObjectStates.size()))
		return 0;
	return meshObjectStates[static_cast<size_t>(index)].kind;
}

int getObjectKindById(unsigned int serialId)
{
	int const idx = indexBySerialId(static_cast<int>(serialId));
	if (idx < 0)
		return 0;
	return meshObjectStates[static_cast<size_t>(idx)].kind;
}

int getObjectSerialId(int index)
{
	if (index < 0 || index >= static_cast<int>(meshObjectStates.size()))
		return 0;
	return meshObjectStates[static_cast<size_t>(index)].serialId;
}

unsigned int getSceneObjectId(int index)
{
	if (index < 0 || index >= static_cast<int>(meshObjectStates.size()))
		return 0;
	return static_cast<unsigned int>(meshObjectStates[static_cast<size_t>(index)].serialId);
}

int removeSceneObject(unsigned int serialId)
{
	if (serialId == 0)
		return 0;
	int const idx = indexBySerialId(static_cast<int>(serialId));
	if (idx < 0)
		return 0;
	meshList.erase(meshList.begin() + idx);
	meshObjectStates.erase(meshObjectStates.begin() + idx);
	g_boolSelectionSerials.erase(
			std::remove(g_boolSelectionSerials.begin(), g_boolSelectionSerials.end(), static_cast<int>(serialId)),
			g_boolSelectionSerials.end());
	if (g_selectedIndex == idx)
		g_selectedIndex = -1;
	else if (g_selectedIndex > idx)
		g_selectedIndex--;
	pruneBooleanSelection();
	return 1;
}

float getObjectFloatParameter(int objectIndex, int parameterIndex)
{
	if (!validObjectStateIndex(objectIndex))
		return 0.0f;
	int const kind = meshObjectStates[static_cast<size_t>(objectIndex)].kind;
	switch (kind)
	{
	case KIND_BOX:
	{
		auto b = objectAs<Box>(objectIndex);
		if (!b)
			return 0.0f;
		auto const d = b->getDimensions();
		if (parameterIndex == 0)
			return d.width;
		if (parameterIndex == 1)
			return d.height;
		if (parameterIndex == 2)
			return d.depth;
		break;
	}
	case KIND_SPHERE:
	{
		auto s = objectAs<Sphere>(objectIndex);
		return (s && parameterIndex == 0) ? s->getDimensions().radius : 0.0f;
	}
	case KIND_PYRAMID:
	{
		auto p = objectAs<Pyramid>(objectIndex);
		if (!p)
			return 0.0f;
		auto const d = p->getDimensions();
		if (parameterIndex == 0)
			return d.side;
		if (parameterIndex == 1)
			return d.height;
		break;
	}
	case KIND_CYLINDER:
	case KIND_CONE:
	{
		auto c = objectAs<Cylinder>(objectIndex);
		if (!c)
			return 0.0f;
		auto const d = c->getDimensions();
		if (parameterIndex == 0)
			return d.radiusBottom;
		if (parameterIndex == 1)
			return kind == KIND_CONE ? 0.0f : d.radiusTop;
		if (parameterIndex == 2)
			return d.height;
		break;
	}
	case KIND_TORUS:
	{
		auto t = objectAs<Torus>(objectIndex);
		if (!t)
			return 0.0f;
		auto const d = t->getDimensions();
		if (parameterIndex == 0)
			return d.majorRadius;
		if (parameterIndex == 1)
			return d.minorRadius;
		break;
	}
	default:
		break;
	}
	return 0.0f;
}

int getObjectIntParameter(int objectIndex, int parameterIndex)
{
	if (!validObjectStateIndex(objectIndex))
		return 0;
	int const kind = meshObjectStates[static_cast<size_t>(objectIndex)].kind;
	switch (kind)
	{
	case KIND_SPHERE:
	{
		auto s = objectAs<Sphere>(objectIndex);
		if (!s)
			return 0;
		auto const p = s->getParameters();
		return parameterIndex == 0 ? p.widthSegments : (parameterIndex == 1 ? p.heightSegments : 0);
	}
	case KIND_CYLINDER:
	case KIND_CONE:
	{
		auto c = objectAs<Cylinder>(objectIndex);
		if (!c)
			return 0;
		auto const p = c->getParameters();
		return parameterIndex == 0 ? p.radialSegments : (parameterIndex == 1 ? p.heightSegments : 0);
	}
	case KIND_TORUS:
	{
		auto t = objectAs<Torus>(objectIndex);
		if (!t)
			return 0;
		auto const p = t->getParameters();
		return parameterIndex == 0 ? p.radialSegments : (parameterIndex == 1 ? p.tubularSegments : 0);
	}
	default:
		return 0;
	}
}

int setObjectParameters(int objectIndex, float p0, float p1, float p2, int i0, int i1)
{
	if (!validObjectStateIndex(objectIndex))
		return 0;
	int const kind = meshObjectStates[static_cast<size_t>(objectIndex)].kind;
	switch (kind)
	{
	case KIND_BOX:
	{
		auto b = objectAs<Box>(objectIndex);
		if (!b || !(p0 > 0.0f && p1 > 0.0f && p2 > 0.0f))
			return 0;
		b->setDimensions(BoxDimensions{std::max(1e-4f, p0), std::max(1e-4f, p1), std::max(1e-4f, p2)});
		b->rebuildGeometry();
		return 1;
	}
	case KIND_SPHERE:
	{
		auto s = objectAs<Sphere>(objectIndex);
		if (!s || !(p0 > 0.0f))
			return 0;
		s->setDimensions(SphereDimensions{std::max(0.01f, p0)});
		SphereParameters sp = s->getParameters();
		sp.widthSegments = std::max(3, i0);
		sp.heightSegments = std::max(2, i1);
		s->setParameters(sp);
		s->rebuildGeometry();
		return 1;
	}
	case KIND_PYRAMID:
	{
		auto p = objectAs<Pyramid>(objectIndex);
		if (!p || !(p0 > 0.0f && p1 > 0.0f))
			return 0;
		p->setDimensions(PyramidDimensions{std::max(1e-4f, p0), std::max(1e-4f, p1)});
		p->rebuildGeometry();
		return 1;
	}
	case KIND_CYLINDER:
	case KIND_CONE:
	{
		auto c = objectAs<Cylinder>(objectIndex);
		float const topRadius = kind == KIND_CONE ? 0.0f : p1;
		if (!c || !(p0 > 0.0f && topRadius >= 0.0f && p2 > 0.0f))
			return 0;
		c->setDimensions(CylinderDimensions{std::max(1e-4f, p0), std::max(0.0f, topRadius), std::max(1e-4f, p2)});
		CylinderParameters cp = c->getParameters();
		cp.radialSegments = std::max(3, i0);
		cp.heightSegments = std::max(1, i1);
		c->setParameters(cp);
		c->rebuildGeometry();
		return 1;
	}
	case KIND_TORUS:
	{
		auto t = objectAs<Torus>(objectIndex);
		if (!t || !(p0 > 0.0f && p1 > 0.0f))
			return 0;
		t->setDimensions(TorusDimensions{std::max(1e-4f, p0), std::max(1e-4f, p1)});
		TorusParameters tp = t->getParameters();
		tp.radialSegments = std::max(3, i0);
		tp.tubularSegments = std::max(3, i1);
		t->setParameters(tp);
		t->rebuildGeometry();
		return 1;
	}
	default:
		return 0;
	}
}

int getObjectTransformOperationCount(int objectIndex)
{
	if (!validObjectStateIndex(objectIndex))
		return 0;
	return static_cast<int>(meshObjectStates[static_cast<size_t>(objectIndex)].operations.size());
}

int getObjectTransformOperationType(int objectIndex, int opIndex)
{
	if (!validObjectStateIndex(objectIndex))
		return 0;
	auto const &ops = meshObjectStates[static_cast<size_t>(objectIndex)].operations;
	if (opIndex < 0 || opIndex >= static_cast<int>(ops.size()))
		return 0;
	return ops[static_cast<size_t>(opIndex)].type;
}

int getObjectTransformOperationFrame(int objectIndex, int opIndex)
{
	if (!validObjectStateIndex(objectIndex))
		return 0;
	auto const &ops = meshObjectStates[static_cast<size_t>(objectIndex)].operations;
	if (opIndex < 0 || opIndex >= static_cast<int>(ops.size()))
		return 0;
	return ops[static_cast<size_t>(opIndex)].frame;
}

float getObjectTransformOperationValue(int objectIndex, int opIndex, int axis)
{
	if (!validObjectStateIndex(objectIndex))
		return 0.0f;
	auto const &ops = meshObjectStates[static_cast<size_t>(objectIndex)].operations;
	if (opIndex < 0 || opIndex >= static_cast<int>(ops.size()))
		return 0.0f;
	auto const &v = ops[static_cast<size_t>(opIndex)].values;
	if (axis == 0)
		return v.x;
	if (axis == 1)
		return v.y;
	if (axis == 2)
		return v.z;
	return 0.0f;
}

int addObjectTransformOperation(int objectIndex, int type, int frame)
{
	return addTransformOperation(objectIndex, type, frame);
}

void setObjectTransformOperationFull(int objectIndex, int opIndex, int type, int frame, float x, float y, float z)
{
	setTransformOperationFullValue(objectIndex, opIndex, type, frame, x, y, z);
}

void removeObjectTransformOperation(int objectIndex, int opIndex)
{
	removeTransformOperation(objectIndex, opIndex);
}

void clearBooleanSelection(void)
{
	g_boolSelectionSerials.clear();
}

int addBooleanSelectionObject(int objectIndex)
{
	if (!validObjectStateIndex(objectIndex))
		return 0;
	int const serial = meshObjectStates[static_cast<size_t>(objectIndex)].serialId;
	if (std::find(g_boolSelectionSerials.begin(), g_boolSelectionSerials.end(), serial) == g_boolSelectionSerials.end())
		g_boolSelectionSerials.push_back(serial);
	return 1;
}

int getBooleanSelectionCount(void)
{
	pruneBooleanSelection();
	return static_cast<int>(g_boolSelectionSerials.size());
}

int getBooleanSelectionObject(int selectionIndex)
{
	pruneBooleanSelection();
	if (selectionIndex < 0 || selectionIndex >= static_cast<int>(g_boolSelectionSerials.size()))
		return -1;
	return indexBySerialId(g_boolSelectionSerials[static_cast<size_t>(selectionIndex)]);
}

static int applyBooleanResult(std::shared_ptr<Mesh> res, int indexA, int indexB)
{
	if (!res)
		return -1;
	int const lo = std::min(indexA, indexB);
	int const hi = std::max(indexA, indexB);
	meshList.erase(meshList.begin() + hi);
	meshObjectStates.erase(meshObjectStates.begin() + hi);
	meshList.erase(meshList.begin() + lo);
	meshObjectStates.erase(meshObjectStates.begin() + lo);
	res->setSolidColor(glm::vec3(0.30f, 0.72f, 0.48f));
	glm::mat4 const resultBaseModel = *res->getModelMatrix();
	meshList.push_back(res);
	registerMeshState(KIND_CSG, res, &resultBaseModel);
	g_selectedIndex = static_cast<int>(meshList.size()) - 1;
	g_boolSelectionSerials.clear();
	g_boolSelectionSerials.push_back(meshObjectStates.back().serialId);
	return g_selectedIndex;
}

static int performBooleanPair(int operation, int a, int b)
{
	int const n = static_cast<int>(meshList.size());
	if (a < 0 || b < 0 || a == b || a >= n || b >= n)
		return -1;

	auto ma = meshList[static_cast<size_t>(a)];
	auto mb = meshList[static_cast<size_t>(b)];
	if (operation == BOOLEAN_UNION)
		return applyBooleanResult(ma->add(mb), a, b);
	if (operation == BOOLEAN_DIFFERENCE)
		return applyBooleanResult(ma->subtract(mb), a, b);
	if (operation == BOOLEAN_INTERSECTION)
		return applyBooleanResult(ma->intersect(mb), a, b);
	return -1;
}

int performBooleanOperation(int operation)
{
	pruneBooleanSelection();
	if (g_boolSelectionSerials.size() < 2)
		return -1;

	std::vector<int> operands = g_boolSelectionSerials;
	int currentSerial = operands.front();
	for (size_t i = 1; i < operands.size(); ++i)
	{
		int const a = indexBySerialId(currentSerial);
		int const b = indexBySerialId(operands[i]);
		int const resultIndex = performBooleanPair(operation, a, b);
		if (resultIndex < 0)
			return -1;
		currentSerial = meshObjectStates[static_cast<size_t>(resultIndex)].serialId;
	}

	g_boolSelectionSerials.clear();
	g_boolSelectionSerials.push_back(currentSerial);
	g_selectedIndex = indexBySerialId(currentSerial);
	return g_selectedIndex;
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

void setAxisHelperVisible(int visible)
{
	g_axisHelper.setVisible(visible != 0);
}

}
