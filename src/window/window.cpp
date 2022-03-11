#include "window.h"
#include <iostream>

Window::Window() : width{800},
									 height{600},
									 mouseFirstMoved{true},
									 leftButtonPressed{false},
									 rightButtonPressed{false}
{
	pKeys = std::make_shared<std::array<bool, 1024>>();
	pKeys->fill(false);
}

Window::Window(GLint windowWidth, GLint windowHeight) : width{windowWidth},
																												height{windowHeight},
																												mouseFirstMoved{true},
																												leftButtonPressed{false},
																												rightButtonPressed{false}
{
	pKeys = std::make_shared<std::array<bool, 1024>>();
	pKeys->fill(false);
}

int Window::Initialise()
{
	if (!glfwInit())
	{
		printf("Error Initialising GLFW");
		glfwTerminate();
		return 1;
	}

	// Setup GLFW Windows Properties
	// OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Core Profile
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Allow forward compatiblity
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// Create the window
	mainWindow = glfwCreateWindow(width, height, "Test Window", NULL, NULL);
	if (!mainWindow)
	{
		printf("Error creating GLFW window!");
		glfwTerminate();
		return 1;
	}

	// Get buffer size information
	glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight);

	// Set the current context
	glfwMakeContextCurrent(mainWindow);

	// Handle key presses + mouse input
	createCallbacks();
	// glfwSetInputMode(mainWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Disable cursor

	// Allow modern extension access
	glewExperimental = GL_TRUE;

	GLenum error = glewInit();
	if (error != GLEW_OK)
	{
		printf("Error: %s", glewGetErrorString(error));
		glfwDestroyWindow(mainWindow);
		glfwTerminate();
		return 1;
	}

	glEnable(GL_DEPTH_TEST);

	// Create Viewport
	glViewport(0, 0, bufferWidth, bufferHeight);
	glfwSetWindowUserPointer(mainWindow, this);
	return 0;
}

Window::~Window()
{
	glfwDestroyWindow(mainWindow);
	glfwTerminate();
}

void Window::handleMouseButton(GLFWwindow *window, int button, int action, int mods)
{
	Window *theWindow = static_cast<Window *>(glfwGetWindowUserPointer(window));
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		theWindow->leftButtonPressed = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		theWindow->leftButtonPressed = false;
		theWindow->mouseFirstMoved = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		theWindow->rightButtonPressed = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
	{
		theWindow->rightButtonPressed = false;
		theWindow->mouseFirstMoved = true;
	}
}

void Window::handleKeys(GLFWwindow *window, int key, int code, int action, int mode)
{
	Window *theWindow = static_cast<Window *>(glfwGetWindowUserPointer(window));
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < theWindow->pKeys->size())
	{
		if (action == GLFW_PRESS)
		{
			theWindow->pKeys->at(key) = true;
			std::cout << "Key Pressed: " << key << std::endl;
		}
		else if (action == GLFW_RELEASE)
		{
			theWindow->pKeys->at(key) = false;
			std::cout << "Key Released: " << key << std::endl;
		}
	}
}

void Window::handleMouseMovement(GLFWwindow *window, double xPos, double yPos)
{
	Window *theWindow = static_cast<Window *>(glfwGetWindowUserPointer(window));
	if (!theWindow->leftButtonPressed)
		return;
	if (theWindow->mouseFirstMoved)
	{
		theWindow->lastX = xPos;
		theWindow->lastY = yPos;
		theWindow->mouseFirstMoved = false;
	}

	theWindow->xChange = xPos - theWindow->lastX;
	theWindow->yChange = theWindow->lastY - yPos;

	theWindow->lastX = xPos;
	theWindow->lastY = yPos;

	std::cout << "Mouse X: " << xPos << " Mouse Y: " << yPos << std::endl;
	std::cout << "X Change: " << theWindow->xChange << " Y Change: " << theWindow->yChange << std::endl;
}

void Window::createCallbacks()
{
	glfwSetKeyCallback(mainWindow, handleKeys);
	glfwSetCursorPosCallback(mainWindow, handleMouseMovement);
	glfwSetMouseButtonCallback(mainWindow, handleMouseButton);
}

GLfloat Window::getXChange()
{
	GLfloat aux = xChange;
	xChange = 0;
	return aux;
}

GLfloat Window::getYChange()
{
	GLfloat aux = xChange;
	yChange = 0;
	return aux;
}