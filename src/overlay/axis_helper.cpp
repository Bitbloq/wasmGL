#include "overlay/axis_helper.h"
#include "camera/orbit_camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <algorithm>
#include <cmath>

#ifdef __EMSCRIPTEN__
static char const *AXIS_VS =
	"#version 300 es                                          \n"
	"uniform mat4 mvp;                                      \n"
	"layout(location = 0) in vec3 pos;                      \n"
	"layout(location = 1) in vec3 color;                    \n"
	"out vec3 vColor;                                       \n"
	"void main() {                                          \n"
	"  vColor = color;                                      \n"
	"  gl_Position = mvp * vec4(pos, 1.0);                  \n"
	"}                                                      \n";

static char const *AXIS_FS =
	"#version 300 es                                          \n"
	"precision mediump float;                               \n"
	"in vec3 vColor;                                        \n"
	"out vec4 fragColor;                                    \n"
	"void main() { fragColor = vec4(vColor, 1.0); }        \n";
#else
static char const *AXIS_VS =
	"#version 330 core                                        \n"
	"uniform mat4 mvp;                                      \n"
	"layout(location = 0) in vec3 pos;                      \n"
	"layout(location = 1) in vec3 color;                    \n"
	"out vec3 vColor;                                       \n"
	"void main() {                                          \n"
	"  vColor = color;                                      \n"
	"  gl_Position = mvp * vec4(pos, 1.0);                  \n"
	"}                                                      \n";

static char const *AXIS_FS =
	"#version 330 core                                        \n"
	"in vec3 vColor;                                        \n"
	"out vec4 fragColor;                                    \n"
	"void main() { fragColor = vec4(vColor, 1.0); }          \n";
#endif

namespace
{
constexpr GLint kVpW = 150;
constexpr GLint kVpH = 150;
constexpr GLint kMargin = 16;
/** World-axis arm length; ortho fits this tetrahedron each frame so pixel size stays fixed vs zoom. */
constexpr float kAxisLen = 1.25f;
constexpr float kOrthoPad = 0.2f;

/**
 * Small ortho in **origin-centered** eye space (after subtracting world origin in eye coords).
 * Bounds stay ~O(axisLen) at any camera distance — avoids float error / clipping when zoomed far out.
 */
inline glm::mat4 orthoForCenteredAxes(float axisLen, float viewportAspect)
{
	float const half = axisLen + kOrthoPad;
	float hx = half;
	float hy = half;
	if (viewportAspect > 1e-6f)
	{
		if (hx / hy > viewportAspect)
			hy = hx / viewportAspect;
		else
			hx = hy * viewportAspect;
	}
	float const hz = half + 2.5f;
	return glm::ortho(-hx, hx, -hy, hy, -hz, hz);
}
} // namespace

void AxisHelperOverlay::init()
{
	shutdown();
	shader_.CreateFromString(AXIS_VS, AXIS_FS);

	float const L = kAxisLen;
	// pos(3) + color(3) per vertex; GL_LINES: X red, Y green, Z blue (world axes, Z-up).
	float interleaved[] = {
			0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, L, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, L, 0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, L, 0.0f, 0.0f, 1.0f,
	};

	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vbo_);
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(interleaved), interleaved, GL_STATIC_DRAW);
	GLsizei const stride = 6 * static_cast<GLsizei>(sizeof(float));
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	gpuReady_ = true;
}

void AxisHelperOverlay::shutdown()
{
	if (vbo_)
		glDeleteBuffers(1, &vbo_);
	if (vao_)
		glDeleteVertexArrays(1, &vao_);
	vbo_ = vao_ = 0;
	shader_.ClearShader();
	gpuReady_ = false;
}

void AxisHelperOverlay::render(OrbitCamera const &cam)
{
	GLint viewportBackup[4];
	glGetIntegerv(GL_VIEWPORT, viewportBackup);
	GLint const framebufferWidth = viewportBackup[2];
	GLint const framebufferHeight = viewportBackup[3];

	if (!visible_ || !gpuReady_ || framebufferWidth < 1 || framebufferHeight < 1)
		return;

	GLboolean depthWasEnabled;
	glGetBooleanv(GL_DEPTH_TEST, &depthWasEnabled);

	GLint const vpW = std::min(kVpW, framebufferWidth - kMargin * 2);
	GLint const vpH = std::min(kVpH, framebufferHeight - kMargin * 2);
	if (vpW < 8 || vpH < 8)
		return;

	GLint const vx0 = framebufferWidth - vpW - kMargin;
	GLint const vy0 = kMargin;

	glViewport(vx0, vy0, vpW, vpH);
	glDisable(GL_DEPTH_TEST);

	glm::mat4 const view = cam.calculateViewMatrix();
	glm::vec3 const originEye = glm::vec3(view * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	glm::mat4 const viewCentered = glm::translate(glm::mat4(1.0f), -originEye) * view;

	float const vpAspect = static_cast<float>(vpW) / static_cast<float>(vpH);
	glm::mat4 const projOrtho = orthoForCenteredAxes(kAxisLen, vpAspect);
	glm::mat4 const mvp = projOrtho * viewCentered;

	shader_.UseShader();
	GLint locMvp = shader_.GetMVPLocation();
	if (locMvp < 0)
		locMvp = glGetUniformLocation(shader_.GetProgramId(), "mvp");
	if (locMvp >= 0)
		glUniformMatrix4fv(locMvp, 1, GL_FALSE, glm::value_ptr(mvp));

	glBindVertexArray(vao_);
	glDrawArrays(GL_LINES, 0, 6);
	glBindVertexArray(0);
	glUseProgram(0);

	glViewport(viewportBackup[0], viewportBackup[1], viewportBackup[2], viewportBackup[3]);
	if (depthWasEnabled)
		glEnable(GL_DEPTH_TEST);
}
