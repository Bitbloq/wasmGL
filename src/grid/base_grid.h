#ifndef BASE_GRID_H
#define BASE_GRID_H

#include "wasmgl_gl.h"
#include "glm/glm.hpp"
#include <vector>

#include "../shaders/shader.h"

/** Matches Bitbloq BaseGrid / THREE.GridHelper-style layout: lines in the XY plane (Z = 0). */
struct BaseGridConfig
{
	float size{20.0f};

	bool smallEnabled{true};
	float smallStep{0.25f};
	unsigned smallColor{0xbbbbbb};
	float smallLineWidth{1.0f}; // ignored on WebGL (always ~1)

	bool bigEnabled{true};
	float bigStep{1.0f};
	unsigned bigColor{0x888888};
	float bigLineWidth{1.0f};

	bool centerEnabled{true};
	unsigned centerColor{0xe74c3c};
	float centerLineWidth{1.0f};

	bool planeEnabled{true};
	unsigned planeColor{0xaabbcc};
};

/** Builds GPU buffers once; draws after clear, before scene meshes. */
class BaseGridRenderer
{
public:
	explicit BaseGridRenderer(BaseGridConfig config = {});

	void setConfig(BaseGridConfig const &cfg);
	void setVisible(bool visible) { visible_ = visible; }
	bool isVisible() const { return visible_; }

	void initShaders();
	void rebuildGeometry();
	void shutdown();

	void render(glm::mat4 const &projection, glm::mat4 const &view);

private:
	void appendLine(std::vector<float> &interleaved, glm::vec3 a, glm::vec3 b, glm::vec3 rgb);
	void appendGridLines(float halfSize, float step, float z, glm::vec3 const &rgb, std::vector<float> &buf);
	static glm::vec3 hexRgb(unsigned hex);

	BaseGridConfig config_;
	bool visible_{true};
	bool gpuReady_{false};

	std::vector<float> lineVertices_; // pos(3)+color(3) per vertex, GL_LINES pairs

	Shader gridLineShader_;
	Shader gridPlaneShader_;
	GLuint lineVAO_{0};
	GLuint lineVBO_{0};
	GLsizei lineVertexCount_{0};

	GLuint planeVAO_{0};
	GLuint planeVBO_{0};
	GLint uniformPlaneMvp_{-1};
	GLint uniformPlaneColor_{-1};
};

#endif
