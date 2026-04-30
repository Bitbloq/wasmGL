#include "overlay/navigation_cube.h"
#include "camera/orbit_camera.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#ifdef __EMSCRIPTEN__
static char const *NAV_VS =
	"#version 300 es                                          \n"
	"precision highp float;                                 \n"
	"uniform mat4 mvp;                                      \n"
	"layout(location = 0) in vec3 pos;                      \n"
	"layout(location = 1) in vec3 normal;                   \n"
	"layout(location = 2) in vec2 uv;                       \n"
	"out vec3 vNormal;                                      \n"
	"out vec2 vUv;                                         \n"
	"void main() {                                          \n"
	"  vNormal = normal;                                    \n"
	"  vUv = uv;                                            \n"
	"  gl_Position = mvp * vec4(pos, 1.0);                 \n"
	"}                                                      \n";

static char const *NAV_FS =
	"#version 300 es                                          \n"
	"precision highp float;                                 \n"
	"precision highp sampler2D;                             \n"
	"in vec3 vNormal;                                       \n"
	"in vec2 vUv;                                           \n"
	"uniform sampler2D uTex;                               \n"
	"out vec4 fragColor;                                    \n"
	"uniform float uBoost;                                  \n"
	"void main() {                                          \n"
	"  vec3 base = texture(uTex, vUv).rgb;                  \n"
	"  vec3 L = normalize(vec3(0.30, -0.42, 0.86));        \n"
	"  float ndl = max(dot(normalize(vNormal), L), 0.0);   \n"
	"  vec3 col = base * (0.26 + 0.74 * ndl);              \n"
	"  col *= uBoost;                                       \n"
	"  col = min(col, vec3(1.0));                           \n"
	"  fragColor = vec4(col, 1.0);                         \n"
	"}                                                      \n";
#else
static char const *NAV_VS =
	"#version 330 core                                        \n"
	"uniform mat4 mvp;                                      \n"
	"layout(location = 0) in vec3 pos;                      \n"
	"layout(location = 1) in vec3 normal;                     \n"
	"layout(location = 2) in vec2 uv;                     \n"
	"out vec3 vNormal;                                      \n"
	"out vec2 vUv;                                         \n"
	"void main() {                                          \n"
	"  vNormal = normal;                                    \n"
	"  vUv = uv;                                            \n"
	"  gl_Position = mvp * vec4(pos, 1.0);                 \n"
	"}                                                      \n";

static char const *NAV_FS =
	"#version 330 core                                        \n"
	"in vec3 vNormal;                                       \n"
	"in vec2 vUv;                                           \n"
	"uniform sampler2D uTex;                               \n"
	"uniform float uBoost;                                  \n"
	"out vec4 fragColor;                                    \n"
	"void main() {                                          \n"
	"  vec3 base = texture(uTex, vUv).rgb;                  \n"
	"  vec3 L = normalize(vec3(0.30, -0.42, 0.86));        \n"
	"  float ndl = max(dot(normalize(vNormal), L), 0.0);   \n"
	"  vec3 col = base * (0.26 + 0.74 * ndl);              \n"
	"  col *= uBoost;                                       \n"
	"  col = min(col, vec3(1.0));                           \n"
	"  fragColor = vec4(col, 1.0);                         \n"
	"}                                                      \n";
#endif

namespace
{
constexpr int kTexSize = 160;
constexpr float kHalfPi = static_cast<float>(M_PI) * 0.5f;
constexpr float kQuarterPi = static_cast<float>(M_PI) * 0.25f;

struct NavClickBox
{
	glm::vec3 center;
	glm::vec3 half;
	float theta;
	float phi;
};

/** Same ordering / angles as the TS NavigationBox clickBoxes. */
static NavClickBox const kClickBoxes[] = {
		{{0.0f, 0.0f, 1.0f}, {0.75f, 0.75f, 0.0005f}, -kHalfPi, 0.0f},
		{{0.0f, 0.0f, -1.0f}, {0.75f, 0.75f, 0.0005f}, -kHalfPi, static_cast<float>(M_PI)},
		{{0.0f, -1.0f, 0.0f}, {0.75f, 0.0005f, 0.75f}, -kHalfPi, kHalfPi},
		{{0.0f, 1.0f, 0.0f}, {0.75f, 0.0005f, 0.75f}, kHalfPi, kHalfPi},
		{{1.0f, 0.0f, 0.0f}, {0.0005f, 0.75f, 0.75f}, 0.0f, kHalfPi},
		{{-1.0f, 0.0f, 0.0f}, {0.0005f, 0.75f, 0.75f}, -static_cast<float>(M_PI), kHalfPi},
		{{0.0f, -0.876f, 0.876f}, {0.75f, 0.125f, 0.125f}, -kHalfPi, kQuarterPi},
		{{-0.876f, 0.0f, 0.876f}, {0.125f, 0.75f, 0.125f}, -static_cast<float>(M_PI), kQuarterPi},
		{{0.0f, 0.876f, 0.876f}, {0.75f, 0.125f, 0.125f}, kHalfPi, kQuarterPi},
		{{0.876f, 0.0f, 0.876f}, {0.125f, 0.75f, 0.125f}, 0.0f, kQuarterPi},
		{{0.0f, -0.876f, -0.876f}, {0.75f, 0.125f, 0.125f}, -kHalfPi, 3.0f * kQuarterPi},
		{{-0.876f, 0.0f, -0.876f}, {0.125f, 0.75f, 0.125f}, -static_cast<float>(M_PI), 3.0f * kQuarterPi},
		{{0.0f, 0.876f, -0.876f}, {0.75f, 0.125f, 0.125f}, kHalfPi, 3.0f * kQuarterPi},
		{{0.876f, 0.0f, -0.876f}, {0.125f, 0.75f, 0.125f}, 0.0f, 3.0f * kQuarterPi},
		{{-0.876f, -0.876f, 0.0f}, {0.125f, 0.125f, 0.75f}, -3.0f * kQuarterPi, kHalfPi},
		{{0.876f, -0.876f, 0.0f}, {0.125f, 0.125f, 0.75f}, -kQuarterPi, kHalfPi},
		{{-0.876f, 0.876f, 0.0f}, {0.125f, 0.125f, 0.75f}, 3.0f * kQuarterPi, kHalfPi},
		{{0.876f, 0.876f, 0.0f}, {0.125f, 0.125f, 0.75f}, kQuarterPi, kHalfPi},
		{{-0.876f, -0.876f, 0.876f}, {0.125f, 0.125f, 0.125f}, -3.0f * kQuarterPi, kQuarterPi},
		{{0.876f, -0.876f, 0.876f}, {0.125f, 0.125f, 0.125f}, -kQuarterPi, kQuarterPi},
		{{-0.876f, 0.876f, 0.876f}, {0.125f, 0.125f, 0.125f}, 3.0f * kQuarterPi, kQuarterPi},
		{{0.876f, 0.876f, 0.876f}, {0.125f, 0.125f, 0.125f}, kQuarterPi, kQuarterPi},
		{{-0.876f, -0.876f, -0.876f}, {0.125f, 0.125f, 0.125f}, -3.0f * kQuarterPi, 3.0f * kQuarterPi},
		{{0.876f, -0.876f, -0.876f}, {0.125f, 0.125f, 0.125f}, -kQuarterPi, 3.0f * kQuarterPi},
		{{-0.876f, 0.876f, -0.876f}, {0.125f, 0.125f, 0.125f}, 3.0f * kQuarterPi, 3.0f * kQuarterPi},
		{{0.876f, 0.876f, -0.876f}, {0.125f, 0.125f, 0.125f}, kQuarterPi, 3.0f * kQuarterPi},
};

static void glyphRows(unsigned char ch, uint8_t rows[7])
{
	for (int i = 0; i < 7; ++i)
		rows[i] = 0;
	switch (ch)
	{
	case 'A':
	{
		static const uint8_t d[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'B':
	{
		static const uint8_t d[7] = {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'C':
	{
		static const uint8_t d[7] = {0x1E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x1E};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'D':
	{
		static const uint8_t d[7] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'E':
	{
		static const uint8_t d[7] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'F':
	{
		static const uint8_t d[7] = {0x1F, 0x10, 0x1E, 0x10, 0x10, 0x10, 0x10};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'G':
	{
		static const uint8_t d[7] = {0x1E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x1E};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'H':
	{
		static const uint8_t d[7] = {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'I':
	{
		static const uint8_t d[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'K':
	{
		static const uint8_t d[7] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'L':
	{
		static const uint8_t d[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'M':
	{
		static const uint8_t d[7] = {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'N':
	{
		static const uint8_t d[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'O':
	{
		static const uint8_t d[7] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'P':
	{
		static const uint8_t d[7] = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'R':
	{
		static const uint8_t d[7] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'T':
	{
		static const uint8_t d[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
		std::memcpy(rows, d, 7);
		break;
	}
	case 'W':
	{
		static const uint8_t d[7] = {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11};
		std::memcpy(rows, d, 7);
		break;
	}
	default:
		break;
	}
}

static void setPixel(std::vector<uint8_t> &rgba, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
	if (x < 0 || y < 0 || x >= kTexSize || y >= kTexSize)
		return;
	size_t const o = (static_cast<size_t>(y) * static_cast<size_t>(kTexSize) + static_cast<size_t>(x)) * 4;
	rgba[o + 0] = r;
	rgba[o + 1] = g;
	rgba[o + 2] = b;
	rgba[o + 3] = 255;
}

static void rotAroundCenter(float px, float py, float cx, float cy, float rad, float &ox, float &oy)
{
	float const c = std::cos(rad);
	float const s = std::sin(rad);
	float const x = px - cx;
	float const y = py - cy;
	ox = c * x - s * y + cx;
	oy = s * x + c * y + cy;
}

static void drawGlyph(std::vector<uint8_t> &rgba, int ox, int oy, unsigned char ch, float rotRad)
{
	uint8_t gr[7];
	glyphRows(ch, gr);
	float const cx = static_cast<float>(kTexSize) * 0.5f;
	float const cy = static_cast<float>(kTexSize) * 0.5f;
	for (int row = 0; row < 7; ++row)
	{
		for (int col = 0; col < 5; ++col)
		{
			if (((gr[row] >> (4 - col)) & 1) == 0)
				continue;
			float const lx = static_cast<float>(ox + col);
			float const ly = static_cast<float>(oy + row);
			float rx, ry;
			rotAroundCenter(lx, ly, cx, cy, rotRad, rx, ry);
			int const ix = static_cast<int>(std::lround(rx));
			int const iy = static_cast<int>(std::lround(ry));
			uint8_t const ink[3] = {22, 22, 28};
			for (int dy = -1; dy <= 1; ++dy)
				for (int dx = -1; dx <= 1; ++dx)
					setPixel(rgba, ix + dx, iy + dy, ink[0], ink[1], ink[2]);
		}
	}
}

static void fillGray(std::vector<uint8_t> &rgba)
{
	rgba.resize(static_cast<size_t>(kTexSize * kTexSize * 4));
	for (int y = 0; y < kTexSize; ++y)
	{
		for (int x = 0; x < kTexSize; ++x)
		{
			size_t const o = (static_cast<size_t>(y) * static_cast<size_t>(kTexSize) + static_cast<size_t>(x)) * 4;
			rgba[o + 0] = 244;
			rgba[o + 1] = 244;
			rgba[o + 2] = 246;
			rgba[o + 3] = 255;
		}
	}
}

static void drawLabelTexture(std::vector<uint8_t> &rgba, char const *label, float rotDeg)
{
	fillGray(rgba);
	float const rotRad = glm::radians(rotDeg);
	int const len = static_cast<int>(std::strlen(label));
	int const totalW = len * 7 - 2;
	int const startX = (kTexSize - totalW) / 2;
	int const startY = (kTexSize - 7) / 2;
	for (int i = 0; i < len; ++i)
	{
		unsigned char const c = static_cast<unsigned char>(label[i]);
		drawGlyph(rgba, startX + i * 7, startY, c, rotRad);
	}
}

static GLuint makeTextureFromRgba(std::vector<uint8_t> const &rgba)
{
	GLuint t{};
	glGenTextures(1, &t);
	glBindTexture(GL_TEXTURE_2D, t);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kTexSize, kTexSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
	glBindTexture(GL_TEXTURE_2D, 0);
	return t;
}

static void appendFace(std::vector<float> &v, glm::vec3 const &a, glm::vec3 const &b, glm::vec3 const &c,
                       glm::vec3 const &d, glm::vec3 const &n, glm::vec2 const &u0, glm::vec2 const &u1,
                       glm::vec2 const &u2, glm::vec2 const &u3)
{
	float const verts[] = {
			a.x, a.y, a.z, n.x, n.y, n.z, u0.x, u0.y,
			b.x, b.y, b.z, n.x, n.y, n.z, u1.x, u1.y,
			c.x, c.y, c.z, n.x, n.y, n.z, u2.x, u2.y,
			d.x, d.y, d.z, n.x, n.y, n.z, u3.x, u3.y,
	};
	for (float x : verts)
		v.push_back(x);
}

static void buildUnitCube(std::vector<float> &interleaved, std::vector<GLuint> &idx)
{
	interleaved.clear();
	idx.clear();
	float const s = 1.0f;
	// +X RIGHT
	appendFace(interleaved, {s, -s, -s}, {s, s, -s}, {s, s, s}, {s, -s, s}, {1, 0, 0}, {0, 0}, {1, 0}, {1, 1}, {0, 1});
	// -X LEFT
	appendFace(interleaved, {-s, s, -s}, {-s, -s, -s}, {-s, -s, s}, {-s, s, s}, {-1, 0, 0}, {0, 0}, {1, 0}, {1, 1}, {0, 1});
	// +Y BACK — CCW from +Y so outward normal is +Y (previous order pointed −Y and culled the outer face)
	appendFace(interleaved, {-s, s, -s}, {-s, s, s}, {s, s, s}, {s, s, -s}, {0, 1, 0}, {0, 0}, {1, 0}, {1, 1}, {0, 1});
	// -Y FRONT — CCW from −Y so outward normal is −Y
	appendFace(interleaved, {s, -s, -s}, {s, -s, s}, {-s, -s, s}, {-s, -s, -s}, {0, -1, 0}, {0, 0}, {1, 0}, {1, 1}, {0, 1});
	// +Z TOP
	appendFace(interleaved, {-s, -s, s}, {s, -s, s}, {s, s, s}, {-s, s, s}, {0, 0, 1}, {0, 0}, {1, 0}, {1, 1}, {0, 1});
	// -Z BOTTOM
	appendFace(interleaved, {-s, s, -s}, {s, s, -s}, {s, -s, -s}, {-s, -s, -s}, {0, 0, -1}, {0, 0}, {1, 0}, {1, 1}, {0, 1});

	for (GLuint i = 0; i < 6; ++i)
	{
		GLuint const b = i * 4;
		idx.push_back(b + 0);
		idx.push_back(b + 1);
		idx.push_back(b + 2);
		idx.push_back(b + 0);
		idx.push_back(b + 2);
		idx.push_back(b + 3);
	}
}

static GLuint compileShader(GLenum type, char const *src)
{
	GLuint s = glCreateShader(type);
	glShaderSource(s, 1, &src, nullptr);
	glCompileShader(s);
	GLint ok{};
	glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		glDeleteShader(s);
		return 0;
	}
	return s;
}

static GLuint linkProgram(GLuint vs, GLuint fs)
{
	GLuint p = glCreateProgram();
	glAttachShader(p, vs);
	glAttachShader(p, fs);
	/* layout(location=) in the vertex shader; glBindAttribLocation can break WebGL2 links. */
	glLinkProgram(p);
	GLint ok{};
	glGetProgramiv(p, GL_LINK_STATUS, &ok);
	glDetachShader(p, vs);
	glDetachShader(p, fs);
	glDeleteShader(vs);
	glDeleteShader(fs);
	if (!ok)
	{
		glDeleteProgram(p);
		return 0;
	}
	return p;
}

static void framebufferCursor(GLFWwindow *win, GLfloat &outFx, GLfloat &outFy)
{
	int winW{}, winH{};
	int fbW{}, fbH{};
	glfwGetWindowSize(win, &winW, &winH);
	glfwGetFramebufferSize(win, &fbW, &fbH);
	if (winW < 1)
		winW = 1;
	if (winH < 1)
		winH = 1;
	double cx{}, cy{};
	glfwGetCursorPos(win, &cx, &cy);
	float const sx = static_cast<float>(fbW) / static_cast<float>(winW);
	float const sy = static_cast<float>(fbH) / static_cast<float>(winH);
	outFx = static_cast<float>(cx * sx);
	outFy = static_cast<float>(fbH - 1.0f - cy * sy);
}

static glm::mat4 navViewFromMainCam(OrbitCamera const &cam)
{
	glm::vec3 const off = cam.getPosition() - cam.getTarget();
	float const len = glm::length(off);
	glm::vec3 dir = len > 1e-5f ? off / len : glm::normalize(glm::vec3(1.0f, 1.0f, 1.2f));
	/** Slight +Z bias so the preview cube is rarely edge-on (horizontal views stay readable). */
	glm::vec3 const biased = glm::normalize(glm::vec3(dir.x, dir.y, dir.z + 0.28f));
	float const dist = 6.5f;
	glm::vec3 const eye = biased * dist;
	return glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}

static void viewportNavCube(GLint framebufferW, GLint framebufferH, GLint &outVx, GLint &outVy, GLint &outVw, GLint &outVh)
{
	GLint const vpW = std::min(NavigationCubeOverlay::kVpW, framebufferW - NavigationCubeOverlay::kMargin * 2);
	GLint const vpH = std::min(NavigationCubeOverlay::kVpH, framebufferH - NavigationCubeOverlay::kMargin * 2);
	outVw = vpW;
	outVh = vpH;
	outVx = NavigationCubeOverlay::kMargin;
	outVy = std::max(0, framebufferH - NavigationCubeOverlay::kMargin - vpH);
}

static bool navCubeRayAabb(glm::vec3 const &ro, glm::vec3 const &rd, glm::vec3 const &center,
                           glm::vec3 const &halfExt, float &tHit)
{
	glm::vec3 const bmin = center - halfExt;
	glm::vec3 const bmax = center + halfExt;
	float t0 = 0.0f;
	float t1 = 1.0e30f;
	for (int a = 0; a < 3; ++a)
	{
		float denom = rd[a];
		if (std::abs(denom) < 1e-8f)
		{
			if (ro[a] < bmin[a] || ro[a] > bmax[a])
				return false;
			continue;
		}
		float const invD = 1.0f / denom;
		float tNear = (bmin[a] - ro[a]) * invD;
		float tFar = (bmax[a] - ro[a]) * invD;
		if (tNear > tFar)
			std::swap(tNear, tFar);
		t0 = std::max(t0, tNear);
		t1 = std::min(t1, tFar);
		if (t0 > t1)
			return false;
	}
	if (t1 < 0.0f)
		return false;
	if (t0 < 0.0f)
		tHit = t1;
	else
		tHit = t0;
	return true;
}

static int pickHitBoxIndex(OrbitCamera const &cam, GLfloat fbx, GLfloat fby, GLint fbw, GLint fbh)
{
	GLint vx{}, vy{}, vpW{}, vpH{};
	viewportNavCube(fbw, fbh, vx, vy, vpW, vpH);
	if (vpW < 8 || vpH < 8)
		return -1;
	if (fbx < static_cast<float>(vx) || fbx >= static_cast<float>(vx + vpW) || fby < static_cast<float>(vy) ||
	    fby >= static_cast<float>(vy + vpH))
		return -1;

	float const ndcX = (2.0f * (fbx - static_cast<float>(vx)) / static_cast<float>(vpW)) - 1.0f;
	float const ndcY = (2.0f * (fby - static_cast<float>(vy)) / static_cast<float>(vpH)) - 1.0f;

	float const aspect = static_cast<float>(vpW) / static_cast<float>(vpH);
	glm::mat4 const proj = glm::perspective(glm::radians(42.0f), aspect, 0.1f, 100.0f);
	glm::mat4 const view = navViewFromMainCam(cam);
	glm::mat4 const invClip = glm::inverse(proj * view);
	glm::vec4 const pNear = invClip * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
	glm::vec4 const pFar = invClip * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
	glm::vec3 const ro = glm::vec3(pNear) / pNear.w;
	glm::vec3 const rf = glm::vec3(pFar) / pFar.w;
	glm::vec3 const rd = glm::normalize(rf - ro);

	float bestT = 1.0e30f;
	int bestIdx = -1;
	for (size_t i = 0; i < sizeof(kClickBoxes) / sizeof(kClickBoxes[0]); ++i)
	{
		float t{};
		if (!navCubeRayAabb(ro, rd, kClickBoxes[i].center, kClickBoxes[i].half, t))
			continue;
		if (t < bestT)
		{
			bestT = t;
			bestIdx = static_cast<int>(i);
		}
	}
	return bestIdx;
}

/** kClickBoxes[0..5] map to mesh face draw index order in buildUnitCube: RIGHT,LEFT,BACK,FRONT,TOP,BOTTOM → 0..5 */
static float boostForDrawFace(int drawFaceIdx, int hoverIdx)
{
	if (hoverIdx < 0)
		return 1.0f;
	if (hoverIdx < 6)
	{
		static const int kHitToDrawFace[6] = {4, 5, 3, 2, 0, 1};
		return (kHitToDrawFace[hoverIdx] == drawFaceIdx) ? 1.34f : 1.0f;
	}
	return 1.16f;
}

} // namespace

bool NavigationCubeOverlay::rayAabb(glm::vec3 const &ro, glm::vec3 const &rd, glm::vec3 const &center,
                                    glm::vec3 const &halfExt, float &tHit)
{
	return navCubeRayAabb(ro, rd, center, halfExt, tHit);
}

void NavigationCubeOverlay::init()
{
	shutdown();

	GLuint const vs = compileShader(GL_VERTEX_SHADER, NAV_VS);
	GLuint const fs = compileShader(GL_FRAGMENT_SHADER, NAV_FS);
	if (!vs || !fs)
		return;
	program_ = linkProgram(vs, fs);
	if (!program_)
		return;
	locMvp_ = glGetUniformLocation(program_, "mvp");
	locTex_ = glGetUniformLocation(program_, "uTex");
	locBoost_ = glGetUniformLocation(program_, "uBoost");

	std::vector<float> verts;
	std::vector<GLuint> indices;
	buildUnitCube(verts, indices);

	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vbo_);
	glGenBuffers(1, &ebo_);
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(verts.size() * sizeof(float)), verts.data(), GL_STATIC_DRAW);
	GLsizei const stride = 8 * static_cast<GLsizei>(sizeof(float));
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizei>(indices.size() * sizeof(GLuint)), indices.data(),
	             GL_STATIC_DRAW);
	glBindVertexArray(0);

	struct FaceLbl
	{
		char const *text;
		float rotDeg;
	};
	FaceLbl const faces[6] = {
			{"RIGHT", 90.0f},
			{"LEFT", -90.0f},
			{"BACK", 180.0f},
			{"FRONT", 0.0f},
			{"TOP", 0.0f},
			{"BOTTOM", 180.0f},
	};
	for (int i = 0; i < 6; ++i)
	{
		std::vector<uint8_t> rgba;
		drawLabelTexture(rgba, faces[i].text, faces[i].rotDeg);
		texLabels_[i] = makeTextureFromRgba(rgba);
	}

	gpuReady_ = program_ != 0 && vao_ != 0;
}

void NavigationCubeOverlay::shutdown()
{
	if (ebo_)
		glDeleteBuffers(1, &ebo_);
	if (vbo_)
		glDeleteBuffers(1, &vbo_);
	if (vao_)
		glDeleteVertexArrays(1, &vao_);
	ebo_ = vbo_ = vao_ = 0;
	for (int i = 0; i < 6; ++i)
	{
		if (texLabels_[i])
			glDeleteTextures(1, &texLabels_[i]);
		texLabels_[i] = 0;
	}
	if (program_)
		glDeleteProgram(program_);
	program_ = 0;
	locMvp_ = locTex_ = locBoost_ = -1;
	gpuReady_ = false;
}

void NavigationCubeOverlay::render(OrbitCamera const &cam)
{
	GLint viewportBackup[4];
	glGetIntegerv(GL_VIEWPORT, viewportBackup);
	GLint const framebufferWidth = viewportBackup[2];
	GLint const framebufferHeight = viewportBackup[3];

	if (!visible_ || !gpuReady_ || framebufferWidth < 1 || framebufferHeight < 1)
		return;

	GLint vx{}, vy{}, vpW{}, vpH{};
	viewportNavCube(framebufferWidth, framebufferHeight, vx, vy, vpW, vpH);
	if (vpW < 8 || vpH < 8)
		return;

	GLboolean depthWasEnabled{};
	glGetBooleanv(GL_DEPTH_TEST, &depthWasEnabled);
	GLboolean cullWasEnabled{};
	glGetBooleanv(GL_CULL_FACE, &cullWasEnabled);
	GLboolean scissorWasEnabled{};
	glGetBooleanv(GL_SCISSOR_TEST, &scissorWasEnabled);
	GLint scissorBackup[4]{};
	glGetIntegerv(GL_SCISSOR_BOX, scissorBackup);

	/** Viewport does not clip glClear; scissor limits depth reset to this HUD rect only (no color clear — avoids white sheet over scene). */
	glEnable(GL_SCISSOR_TEST);
	glScissor(vx, vy, vpW, vpH);
	glViewport(vx, vy, vpW, vpH);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	float const aspect = static_cast<float>(vpW) / static_cast<float>(vpH);
	glm::mat4 const proj = glm::perspective(glm::radians(42.0f), aspect, 0.1f, 100.0f);
	glm::mat4 const view = navViewFromMainCam(cam);
	glm::mat4 const mvp = proj * view;

	glUseProgram(program_);
	glUniformMatrix4fv(locMvp_, 1, GL_FALSE, glm::value_ptr(mvp));

	glBindVertexArray(vao_);
	for (int f = 0; f < 6; ++f)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texLabels_[f]);
		glUniform1i(locTex_, 0);
		if (locBoost_ >= 0)
			glUniform1f(locBoost_, boostForDrawFace(f, hoverPickIndex_));
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, reinterpret_cast<void *>(static_cast<uintptr_t>(f * 6 * sizeof(GLuint))));
	}
	glBindVertexArray(0);
	glUseProgram(0);

	glViewport(viewportBackup[0], viewportBackup[1], viewportBackup[2], viewportBackup[3]);
	glScissor(scissorBackup[0], scissorBackup[1], scissorBackup[2], scissorBackup[3]);
	if (scissorWasEnabled)
		glEnable(GL_SCISSOR_TEST);
	else
		glDisable(GL_SCISSOR_TEST);
	if (!depthWasEnabled)
		glDisable(GL_DEPTH_TEST);
	else
		glEnable(GL_DEPTH_TEST);
	if (!cullWasEnabled)
		glDisable(GL_CULL_FACE);
}

void NavigationCubeOverlay::cursorFramebufferPixels(GLFWwindow *win, GLfloat &outFx, GLfloat &outFy)
{
	framebufferCursor(win, outFx, outFy);
}

void NavigationCubeOverlay::syncHover(OrbitCamera const &cam, GLfloat framebufferX, GLfloat framebufferY,
                                      GLfloat framebufferWidth, GLfloat framebufferHeight)
{
	if (!gpuReady_ || !visible_)
	{
		hoverPickIndex_ = -1;
		return;
	}
	hoverPickIndex_ = pickHitBoxIndex(cam, framebufferX, framebufferY, static_cast<GLint>(framebufferWidth),
	                                  static_cast<GLint>(framebufferHeight));
}

bool NavigationCubeOverlay::pick(OrbitCamera const &cam, GLfloat framebufferX, GLfloat framebufferY,
                                 GLfloat framebufferWidth, GLfloat framebufferHeight, float *outTheta,
                                 float *outPhi) const
{
	if (!outTheta || !outPhi || !gpuReady_ || !visible_)
		return false;
	int const idx = pickHitBoxIndex(cam, framebufferX, framebufferY, static_cast<GLint>(framebufferWidth),
	                                static_cast<GLint>(framebufferHeight));
	if (idx < 0)
		return false;
	*outTheta = kClickBoxes[static_cast<size_t>(idx)].theta;
	*outPhi = kClickBoxes[static_cast<size_t>(idx)].phi;
	return true;
}
