#ifndef AXIS_HELPER_H
#define AXIS_HELPER_H

#include "wasmgl_gl.h"
#include "glm/glm.hpp"
#include "shaders/shader.h"

class OrbitCamera;

/**
 * Screen-fixed region (bottom-right): world XYZ axes (RGB), same orientation as the main camera / grid.
 * Uses orthographic projection in eye space so apparent size stays constant (zoom / dolly does not scale it).
 */
class AxisHelperOverlay
{
public:
	void init();
	void shutdown();

	void setVisible(bool visible) { visible_ = visible; }
	bool isVisible() const { return visible_; }

	void render(OrbitCamera const &cam);

private:
	Shader shader_{};
	GLuint vao_{0};
	GLuint vbo_{0};
	bool gpuReady_{false};
	bool visible_{true};
};

#endif
