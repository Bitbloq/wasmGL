#include "base_grid.h"
#include "glm/gtc/type_ptr.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>

#ifdef __EMSCRIPTEN__
static char const *GRID_LINE_VS =
		"#version 300 es                                          \n"
		"uniform mat4 mvp;                                      \n"
		"layout(location = 0) in vec3 pos;                      \n"
		"layout(location = 1) in vec3 color;                  \n"
		"out vec3 vColor;                                       \n"
		"void main() {                                          \n"
		"  vColor = color;                                      \n"
		"  gl_Position = mvp * vec4(pos, 1.0);                  \n"
		"}                                                      \n";

static char const *GRID_LINE_FS =
		"#version 300 es                                          \n"
		"precision mediump float;                               \n"
		"in vec3 vColor;                                        \n"
		"out vec4 fragColor;                                    \n"
		"void main() { fragColor = vec4(vColor, 1.0); }        \n";

static char const *GRID_PLANE_VS =
		"#version 300 es                                          \n"
		"uniform mat4 mvp;                                      \n"
		"layout(location = 0) in vec3 pos;                      \n"
		"void main() { gl_Position = mvp * vec4(pos, 1.0); }    \n";

static char const *GRID_PLANE_FS =
		"#version 300 es                                          \n"
		"precision mediump float;                               \n"
		"uniform vec4 uColor;                                   \n"
		"out vec4 fragColor;                                    \n"
		"void main() { fragColor = uColor; }                     \n";
#else
static char const *GRID_LINE_VS =
		"#version 330 core                                        \n"
		"uniform mat4 mvp;                                      \n"
		"layout(location = 0) in vec3 pos;                      \n"
		"layout(location = 1) in vec3 color;                    \n"
		"out vec3 vColor;                                       \n"
		"void main() {                                          \n"
		"  vColor = color;                                      \n"
		"  gl_Position = mvp * vec4(pos, 1.0);                  \n"
		"}                                                      \n";

static char const *GRID_LINE_FS =
		"#version 330 core                                        \n"
		"in vec3 vColor;                                        \n"
		"out vec4 fragColor;                                    \n"
		"void main() { fragColor = vec4(vColor, 1.0); }          \n";

static char const *GRID_PLANE_VS =
		"#version 330 core                                        \n"
		"uniform mat4 mvp;                                      \n"
		"layout(location = 0) in vec3 pos;                      \n"
		"void main() { gl_Position = mvp * vec4(pos, 1.0); }    \n";

static char const *GRID_PLANE_FS =
		"#version 330 core                                        \n"
		"uniform vec4 uColor;                                   \n"
		"out vec4 fragColor;                                    \n"
		"void main() { fragColor = uColor; }                     \n";
#endif

glm::vec3 BaseGridRenderer::hexRgb(unsigned hex)
{
	return glm::vec3(
			static_cast<float>((hex >> 16) & 255u) / 255.0f,
			static_cast<float>((hex >> 8) & 255u) / 255.0f,
			static_cast<float>(hex & 255u) / 255.0f);
}

void BaseGridRenderer::appendLine(std::vector<float> &b, glm::vec3 a, glm::vec3 b_, glm::vec3 rgb)
{
	b.push_back(a.x);
	b.push_back(a.y);
	b.push_back(a.z);
	b.push_back(rgb.x);
	b.push_back(rgb.y);
	b.push_back(rgb.z);
	b.push_back(b_.x);
	b.push_back(b_.y);
	b.push_back(b_.z);
	b.push_back(rgb.x);
	b.push_back(rgb.y);
	b.push_back(rgb.z);
}

void BaseGridRenderer::appendGridLines(float halfSize, float step, float z, glm::vec3 const &rgb, std::vector<float> &buf)
{
	if (step <= 0.0f)
		return;
	float const lim = halfSize + 1e-4f;
	for (float k = -halfSize; k <= lim; k += step)
	{
		float const kk = std::min(k, halfSize);
		appendLine(buf, glm::vec3(-halfSize, kk, z), glm::vec3(halfSize, kk, z), rgb);
		appendLine(buf, glm::vec3(kk, -halfSize, z), glm::vec3(kk, halfSize, z), rgb);
	}
}

BaseGridRenderer::BaseGridRenderer(BaseGridConfig config) : config_(config) {}

void BaseGridRenderer::setConfig(BaseGridConfig const &cfg)
{
	config_ = cfg;
	rebuildGeometry();
}

void BaseGridRenderer::initShaders()
{
	gridLineShader_.CreateFromString(GRID_LINE_VS, GRID_LINE_FS);
	gridPlaneShader_.CreateFromString(GRID_PLANE_VS, GRID_PLANE_FS);
	uniformPlaneMvp_ = glGetUniformLocation(gridPlaneShader_.GetProgramId(), "mvp");
	uniformPlaneColor_ = glGetUniformLocation(gridPlaneShader_.GetProgramId(), "uColor");
	rebuildGeometry();
	gpuReady_ = true;
}

void BaseGridRenderer::rebuildGeometry()
{
	lineVertices_.clear();
	float const half = config_.size * 0.5f;
	float const zLine = 0.001f; // slight offset above translucent plane

	if (config_.bigEnabled && config_.bigStep > 0.0f)
		appendGridLines(half, config_.bigStep, zLine, hexRgb(config_.bigColor), lineVertices_);
	if (config_.smallEnabled && config_.smallStep > 0.0f)
		appendGridLines(half, config_.smallStep, zLine, hexRgb(config_.smallColor), lineVertices_);
	if (config_.centerEnabled)
	{
		glm::vec3 const cc = hexRgb(config_.centerColor);
		appendLine(lineVertices_, glm::vec3(-half, 0.0f, zLine), glm::vec3(half, 0.0f, zLine), cc);
		appendLine(lineVertices_, glm::vec3(0.0f, -half, zLine), glm::vec3(0.0f, half, zLine), cc);
	}

	lineVertexCount_ = static_cast<GLsizei>(lineVertices_.size() / 6);

	if (lineVAO_ == 0)
		glGenVertexArrays(1, &lineVAO_);
	if (lineVBO_ == 0)
		glGenBuffers(1, &lineVBO_);

	glBindVertexArray(lineVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO_);
	glBufferData(GL_ARRAY_BUFFER, lineVertices_.size() * sizeof(float), lineVertices_.data(), GL_STATIC_DRAW);
	GLsizei const stride = 6 * static_cast<GLsizei>(sizeof(float));
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	// Plane quad in XY at Z=0 (matches TS PlaneHelper lying in the grid plane)
	float const zPlane = 0.0f;
	float quad[] = {
			-half, -half, zPlane,
			half, -half, zPlane,
			half, half, zPlane,
			-half, -half, zPlane,
			half, half, zPlane,
			-half, half, zPlane,
	};
	if (planeVAO_ == 0)
		glGenVertexArrays(1, &planeVAO_);
	if (planeVBO_ == 0)
		glGenBuffers(1, &planeVBO_);
	glBindVertexArray(planeVAO_);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

void BaseGridRenderer::shutdown()
{
	if (lineVBO_)
		glDeleteBuffers(1, &lineVBO_);
	if (lineVAO_)
		glDeleteVertexArrays(1, &lineVAO_);
	if (planeVBO_)
		glDeleteBuffers(1, &planeVBO_);
	if (planeVAO_)
		glDeleteVertexArrays(1, &planeVAO_);
	lineVBO_ = lineVAO_ = planeVBO_ = planeVAO_ = 0;
	gridLineShader_.ClearShader();
	gridPlaneShader_.ClearShader();
	gpuReady_ = false;
}

void BaseGridRenderer::render(glm::mat4 const &projection, glm::mat4 const &view)
{
	if (!visible_ || !gpuReady_)
		return;

	glm::mat4 const vp = projection * view;

	if (config_.planeEnabled && planeVAO_ != 0 && uniformPlaneMvp_ >= 0)
	{
		glm::vec3 const pc = hexRgb(config_.planeColor);
		glm::vec4 const planeRgba(pc, 0.14f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		gridPlaneShader_.UseShader();
		glUniformMatrix4fv(uniformPlaneMvp_, 1, GL_FALSE, glm::value_ptr(vp));
		glUniform4fv(uniformPlaneColor_, 1, glm::value_ptr(planeRgba));
		glBindVertexArray(planeVAO_);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}

	if (lineVertexCount_ > 0 && lineVAO_ != 0)
	{
		gridLineShader_.UseShader();
		glUniformMatrix4fv(gridLineShader_.GetMVPLocation(), 1, GL_FALSE, glm::value_ptr(vp));
		glBindVertexArray(lineVAO_);
		glDrawArrays(GL_LINES, 0, lineVertexCount_);
		glBindVertexArray(0);
	}

	glUseProgram(0);
}
