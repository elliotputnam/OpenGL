#include "Window.h"


Window::Window()
{

	// window size
	width = 800;
	height = 600;

	// initializes keys to off
	for (size_t i = 0; i < 1024; i++)
	{
		keys[i] = 0;
	}

	xChange = 0.0f;
	yChange = 0.0f;
}

Window::Window(GLint windowWidth, GLint windowHeight)
{

	// window size
	width = windowWidth;
	height = windowHeight;
	
	// initializes keys to off
	for (size_t i = 0; i < 1024; i++)
	{
		keys[i] = 0;
	}

	xChange = 0.0f;
	yChange = 0.0f;
}

int Window::Initialize()
{
	// INIT GLFW
	if (!glfwInit())
	{
		printf("GLFW Init failed.");
		glfwTerminate();
		return 1;
	}

	// SETUP GLFW window props
	// OpenGL Version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	// Core profile
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// Allow forward compat
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	mainWindow = glfwCreateWindow(width, height, "OpenGL", NULL, NULL);

	// Get buffer size info
	glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight);

	// Set context for GLEW to use
	glfwMakeContextCurrent(mainWindow);

	// Handle key + mouse inputs
	createCallbacks();

	// disable cursor (locking it to screen)
	glfwSetInputMode(mainWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Allow modern extensions
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		printf("GLEW Init failed.");
		glfwDestroyWindow(mainWindow);
		glfwTerminate();
		return 1;
	}

	// enables gl stuff
	glEnable(GL_DEPTH_TEST);

	// Setup viewport size
	glViewport(0, 0, bufferWidth, bufferHeight);

	// tells who is using the window
	glfwSetWindowUserPointer(mainWindow, this);
}

void Window::createCallbacks()
{
	// When a key is pressed in mainWindow, pass in handleKeys
	glfwSetKeyCallback(mainWindow, handleKeys);
	glfwSetCursorPosCallback(mainWindow, handleMous);
}

GLfloat Window::getXChange()
{
	GLfloat theXChange = xChange;
	xChange = 0.0f;
	return theXChange;
}

GLfloat Window::getYChange()
{
	GLfloat theYChange = yChange;
	yChange = 0.0f;
	return theYChange;
}

void Window::handleKeys(GLFWwindow* window, int key, int code, int action, int mode)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			theWindow->keys[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			theWindow->keys[key] = false;
		}
	}
}

void Window::handleMous(GLFWwindow* window, double xPos, double yPos)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	// handles first move so screen doesn't jump
	if (theWindow->mouseFirstMoved)
	{
		theWindow->lastX = xPos;
		theWindow->lastY = yPos;
		theWindow->mouseFirstMoved = false;
	}

	theWindow->xChange = xPos - theWindow->lastX;
	theWindow->yChange = theWindow->lastY - yPos; // swapped to avoid inverted up and down

	// new coordinates
	theWindow->lastX = xPos;
	theWindow->lastY = yPos;
}

Window::~Window()
{
	glfwDestroyWindow(mainWindow);
	glfwTerminate();
}