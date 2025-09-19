#define STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>

#include <GL\glew.h>
#include <GLFW\glfw3.h>

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include "Window.h"
#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"

const float toRadians = 3.14159265f / 180.0f;

Window mainWindow;
std::vector<Mesh*> meshList;
std::vector<Shader> shaderList;
Camera camera;

Texture brickTexture;
Texture dirtTexture;

GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;

bool direction = true;
float triOffset = 0.0f;
float triMaxOffset = 0.7f;
float triIncrement = 0.001f;
float currAngle = 0.0f;

bool sizeDirection = true;
float curSize = 0.4f;
float maxSize = 0.8f;
float minSize = 0.1f;

// Vertex Shader
// clamp() keeps input values within specified range (0 - 1), removing negatives in position.
static const char* vShader = "Shaders/shader.vert";

// Fragment shader
static const char* fShader = "Shaders/shader.frag";

void CreateObjects()
{
	unsigned int indices[] =
	{
		0, 3, 1, // side 
		1, 3, 2, // side
		2, 3, 0, // front
		0, 1, 2 // base
	};

	GLfloat vertices[] =
	{
		// X, Y, Z
		-1.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 0.0f,
		0.0f, 1.0f, 0.0f
	};
	Mesh *obj1 = new Mesh();
	obj1->CreateMesh(vertices, indices, 12, 12);
	meshList.push_back(obj1);

	Mesh *obj2 = new Mesh();
	obj2->CreateMesh(vertices, indices, 12, 12);
	meshList.push_back(obj2);
}

void CreateShaders()
{
	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(vShader, fShader);
	shaderList.push_back(*shader1);
}

int main()
{
	mainWindow = Window(800, 600);
	mainWindow.Initialize();

	CreateObjects();
	CreateShaders();

	// init camera
	camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 3.0f, 0.05f);

	brickTexture = Texture("Textures/brick.png");
	brickTexture.LoadTexture();
	dirtTexture = Texture("Textures/brick.png");
	dirtTexture.LoadTexture();

	GLuint uniformProjection = 0, uniformModel = 0, uniformView = 0;

	// setup projection
	glm::mat4 projection = glm::perspective(45.0f, mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 100.0f);

	// loop until window closes
	while (!mainWindow.getWindowShouldClose())
	{
		GLfloat now = glfwGetTime(); // SDL_GetPerformanceCounter(); milliseconds
		deltaTime = now - lastTime; // (now - lastTime)*1000 / SDL_GetPerformanceCounter() seconds
		lastTime = now;

		// Get / Handle input events
		glfwPollEvents();
		// Passes inputs into camera
		camera.KeyControl(mainWindow.getsKeys(), deltaTime);
		camera.MouseControl(mainWindow.getXChange(), mainWindow.getYChange());

		if (direction)
		{
			triOffset += triIncrement;
		}
		else
		{
			triOffset -= triIncrement;
		}

		if (abs(triOffset) >= triMaxOffset)
		{
			direction = !direction;
		}

		// Rotation variable
		currAngle += 0.5f;
		if (currAngle >= 360)
		{
			currAngle = 0.0f;
		}

		if (sizeDirection)
		{
			curSize += 0.01f;
		}
		else
		{
			curSize -= 0.01f;
		}

		if (curSize >= maxSize || curSize <= minSize)
		{
			sizeDirection = !sizeDirection;
		}



		// Clear window
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// assigns shader
		shaderList[0].UseShader();
		uniformModel = shaderList[0].GetModelLocation();
		uniformProjection = shaderList[0].GetProjectionLocation();
		uniformView = shaderList[0].GetViewLocation();

		// Mat4 = Matrix 4x4
		glm::mat4 model(1.0f);

		//// translate(OBJECT, OFFSET)
		//// rotate(OBJECT, ROTATION (in Rad), AXIS(x, y, z))
		//// sacle(OBJECT, vec3)
		model = glm::translate(model, glm::vec3(triOffset, 0.0f, -2.5f));
		//model = glm::rotate(model, currAngle * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 1.0f));

		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));

		meshList[0]->RenderMesh();

		model = glm::mat4(1.0f);

		model = glm::translate(model, glm::vec3(-triOffset, 1.0f, -2.5f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 1.0f)); 
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

		meshList[1]->RenderMesh();
		

		glUseProgram(0);

		// Swap drawn and drawing buffers
		mainWindow.swapBuffers();
	}

	return 0;
}
