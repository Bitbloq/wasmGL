#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <stdio.h>
#include <array>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <memory>

class Window
{
public:
	Window();

	Window(GLint windowWidth, GLint windowHeight);

	int Initialise();

	GLint getBufferWidth() { return bufferWidth; }
	GLint getBufferHeight() { return bufferHeight; }

	bool getShouldClose() { return glfwWindowShouldClose(mainWindow); }
	inline std::shared_ptr<std::array<bool, 1024>> getKeys() const { return pKeys; }
	GLfloat getXChange();
	GLfloat getYChange();

	void swapBuffers() { glfwSwapBuffers(mainWindow); }

	~Window();

private:
	GLFWwindow *mainWindow;

	GLint width, height;
	GLint bufferWidth, bufferHeight;

	std::shared_ptr<std::array<bool, 1024>> pKeys;
	GLfloat lastX, lastY, xChange, yChange;
	bool leftButtonPressed, rightButtonPressed;
	bool mouseFirstMoved;

	void createCallbacks();
	static void handleKeys(GLFWwindow *window, int key, int code, int action, int mode);
	static void handleMouseMovement(GLFWwindow *window, double xPos, double yPos);
	static void handleMouseButton(GLFWwindow *window, int button, int action, int mods);
};

#endif