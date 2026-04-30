#ifndef NAVIGATION_CUBE_H
#define NAVIGATION_CUBE_H

#include "wasmgl_gl.h"
#include "glm/glm.hpp"

struct GLFWwindow;
class OrbitCamera;

/** Top-left perspective cube (faces / edges / corners) matching TS NavigationBox hit regions; snaps orbit angles on click. */
class NavigationCubeOverlay
{
public:
	void init();
	void shutdown();

	void setVisible(bool visible) { visible_ = visible; }
	bool isVisible() const { return visible_; }

	void render(OrbitCamera const &cam);

	/** GLFW cursor → framebuffer pixels (bottom-left), for picking / tests. */
	static void cursorFramebufferPixels(GLFWwindow *win, GLfloat &outFx, GLfloat &outFy);

	/** Update hover region from cursor (call each frame for highlight). */
	void syncHover(OrbitCamera const &cam, GLfloat framebufferX, GLfloat framebufferY, GLfloat framebufferWidth,
	               GLfloat framebufferHeight);

	/** Cursor in framebuffer pixels (origin bottom-left). Returns false if click misses cube overlay or GPU not ready. */
	bool pick(OrbitCamera const &cam, GLfloat framebufferX, GLfloat framebufferY, GLfloat framebufferWidth,
	          GLfloat framebufferHeight, float *outTheta, float *outPhi) const;

	static constexpr GLint kVpW = 104;
	static constexpr GLint kVpH = 104;
	static constexpr GLint kMargin = 20;

private:
	GLuint program_{0};
	GLuint vao_{0};
	GLuint vbo_{0};
	GLuint ebo_{0};
	GLuint texLabels_[6]{};
	GLint locMvp_{-1};
	GLint locTex_{-1};
	GLint locBoost_{-1};
	bool gpuReady_{false};
	bool visible_{true};
	int hoverPickIndex_{-1};

	static bool rayAabb(glm::vec3 const &ro, glm::vec3 const &rd, glm::vec3 const &center,
	                    glm::vec3 const &halfExt, float &tHit);
};

#endif
