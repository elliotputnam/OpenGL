#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <iostream>

const GLint WIDTH = 800, HEIGHT = 600;

int main()
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

	GLFWwindow* mainWindow = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL", NULL, NULL);

	// Get buffer size info
	int bufferWidth, bufferHeight;
	glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight);

	// Set context for GLEW to use
	glfwMakeContextCurrent(mainWindow);
	
	// Allow modern extensions
	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		printf("GLEW Init failed.");
		glfwDestroyWindow(mainWindow);
		glfwTerminate();
		return 1;
	}

	// Setup viewport size
	glViewport(0, 0, bufferWidth, bufferHeight);

	// loop until window closes
	while (!glfwWindowShouldClose(mainWindow))
	{
		// Get / Handle input events
		glfwPollEvents();

		// Clear window
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Swap drawn and drawing buffers
		glfwSwapBuffers(mainWindow);
	}

	return 0;
}
