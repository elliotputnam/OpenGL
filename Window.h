#pragma once
#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>


class Window
{
public:
	Window();

	Window(GLint windowWidth, GLint windowHeight);

	int Initialize();

	GLfloat getBufferWidth(){ return bufferWidth; }
	GLfloat getBufferHeight(){ return bufferHeight; }
	bool getWindowShouldClose(){ return glfwWindowShouldClose(mainWindow); }
	bool* getsKeys() { return keys; }
	GLfloat getXChange();
	GLfloat getYChange();
	void swapBuffers() { glfwSwapBuffers(mainWindow); }


	~Window();

private:

	GLFWwindow* mainWindow;
	GLint width, height;
	GLint bufferWidth, bufferHeight;

	bool keys[1024];

	GLfloat lastX;
	GLfloat lastY;
	GLfloat xChange;
	GLfloat yChange;
	bool mouseFirstMoved;

	// handles inputs
	void createCallbacks();

	// requires static for glfw callback
	static void handleKeys(GLFWwindow* window, int key, int code, int action, int mode);
	static void handleMous(GLFWwindow* window, double xPos, double yPos);
};

