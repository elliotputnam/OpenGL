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
#include "Light.h"
#include "Material.h"

const float toRadians = 3.14159265f / 180.0f;

Window mainWindow;
std::vector<Mesh*> meshList;
std::vector<Shader> shaderList;
Camera camera;

Texture brickTexture;
Texture dirtTexture;

Material shinyMaterial;
Material dullMaterial;

Light mainLight;

GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;

// movement testing variables
bool direction = true;
float triOffset = 0.0f;
float triMaxOffset = 0.7f;
float triIncrement = 0.001f;

// rotation testing variables
float currAngle = 0.0f;

// Size testing variables
bool sizeDirection = true;
float curSize = 0.4f;
float maxSize = 0.6f;
float minSize = 0.1f;

// Vertex Shader
// clamp() keeps input values within specified range (0 - 1), removing negatives in position.
static const char* vShader = "Shaders/shader.vert";

// Fragment shader
static const char* fShader = "Shaders/shader.frag";
static const char* fShaderRb = "Shaders/shaderSpectrum.frag";

void calcAverageNormals(unsigned int* indices, unsigned int indiceCount, GLfloat* vertices, unsigned int verticeCount,
	unsigned int vLength, unsigned int normalOffset)
{
	// iterate trough each triangle
	for (size_t i = 0; i < indiceCount; i += 3)
	{
		// assign variables of each vertice
		unsigned int in0 = indices[i] * vLength;
		unsigned int in1 = indices[i + 1] * vLength;
		unsigned int in2 = indices[i + 2] * vLength;
		glm::vec3 v1(vertices[in1] - vertices[in0], vertices[in1 + 1] - vertices[in0 + 1], vertices[in1 + 2] - vertices[in0 + 2]);
		glm::vec3 v2(vertices[in2] - vertices[in0], vertices[in2 + 1] - vertices[in0 + 1], vertices[in2 + 2] - vertices[in0 + 2]);
		glm::vec3 normal = glm::cross(v1, v2);
		normal = glm::normalize(normal);

		in0 += normalOffset; in1 += normalOffset; in2 += normalOffset; // skips to normals in vertices

		// assign calculated normals into vertices
		vertices[in0] += normal.x; vertices[in0 + 1] += normal.y; vertices[in0 + 2] += normal.z;
		vertices[in1] += normal.x; vertices[in1 + 1] += normal.y; vertices[in1 + 2] += normal.z;
		vertices[in2] += normal.x; vertices[in2 + 1] += normal.y; vertices[in2 + 2] += normal.z;
	}

	for (size_t i = 0; i < verticeCount / vLength; i++)
	{
		unsigned int nOffset = i * vLength + normalOffset;
		glm::vec3 vec(vertices[nOffset], vertices[nOffset + 1], vertices[nOffset + 2]);
		vec = glm::normalize(vec);
		vertices[nOffset] = vec.x; vertices[nOffset + 1] = vec.y; vertices[nOffset + 2] = vec.z;
	}
}

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
		//X     Y     Z        U     V			nx	  ny	nz
		-1.0f, -1.0f, -0.5f,	  0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 1.0f,    0.5f, 0.0f,		0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, -0.5f,    1.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,     0.5f, 1.0f,		0.0f, 0.0f, 0.0f
	};

	calcAverageNormals(indices, 12, vertices, 32, 8, 5);

	Mesh *obj1 = new Mesh();
	obj1->CreateMesh(vertices, indices, 32, 12);
	meshList.push_back(obj1);

	Mesh *obj2 = new Mesh();
	obj2->CreateMesh(vertices, indices, 32, 12);
	meshList.push_back(obj2);
	
	Mesh *obj3 = new Mesh();
	obj3->CreateMesh(vertices, indices, 32, 12);
	meshList.push_back(obj3);
	
	Mesh *obj4 = new Mesh();
	obj4->CreateMesh(vertices, indices, 32, 12);
	meshList.push_back(obj4);
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

	// load textures
	brickTexture = Texture("Textures/brick.png");
	brickTexture.LoadTexture();
	dirtTexture = Texture("Textures/dirt.png");
	dirtTexture.LoadTexture();

	// Specular lighting
	shinyMaterial = Material(1.0f, 32);
	dullMaterial = Material(0.3f, 4);
	
	// load lighting
	mainLight = Light(1.0f, 1.0f, 1.0f, 0.2f, // ambient lighting
					2.0f, 1.0f, 2.0f, 1.0f); // direction and diffuse


	GLuint uniformProjection = 0, uniformModel = 0, uniformView = 0, uniformEyePosition = 0,
		uniformAmbientColor = 0, uniformAmbientIntensity = 0, //ambient
		uniformDirection = 0, uniformDiffuseIntensityLocation = 0,// diffuse
		uniformSpecularIntensity = 0, uniformShininess = 0; 

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



		// *********** MODEL MOVEMENT ************* // 
		// **************************************** //
		// **************************************** //

		// TRANSLATION
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

		// ROTATION
		currAngle += 0.5f;
		if (currAngle >= 360)
		{
			currAngle = 0.0f;
		}

		// SCALE
		if (sizeDirection)
		{
			curSize += 0.001f;
		}
		else
		{
			curSize -= 0.001f;
		}

		if (curSize >= maxSize || curSize <= minSize)
		{
			sizeDirection = !sizeDirection;
		}
		// **************************************** //
		// **************************************** //
		// **************************************** //


		// Clear window
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// assigns shader
		shaderList[0].UseShader();

		// assign uniforms
		uniformModel = shaderList[0].GetModelLocation();
		uniformProjection = shaderList[0].GetProjectionLocation();
		uniformView = shaderList[0].GetViewLocation();
		uniformAmbientColor = shaderList[0].GetAmbientColorLocation();
		uniformAmbientIntensity = shaderList[0].GetAmbientIntensityLocation();
		uniformDirection = shaderList[0].GetDirectionLocation();
		uniformDiffuseIntensityLocation = shaderList[0].GetDiffuseIntensityLocation();
		uniformSpecularIntensity = shaderList[0].GetSpecularIntensityLocation();
		uniformShininess = shaderList[0].GetShininessLocation();
		uniformEyePosition = shaderList[0].GetEyePositionLocation();

		// Sets lighting
		mainLight.UseLight(uniformAmbientIntensity, uniformAmbientColor, uniformDiffuseIntensityLocation, uniformDirection);

		//// translate(OBJECT, OFFSET)
		//// rotate(OBJECT, ROTATION (in Rad), AXIS(x, y, z))
		//// scale(OBJECT, vec3)

		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));
		glUniform3f(uniformEyePosition, camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

		// MODEL 1
		glm::mat4 model(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -2.5f));
		model = glm::rotate(model, currAngle * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		brickTexture.UseTexture();
		shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[0]->RenderMesh();

		// MODEL 2
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-triOffset, 1.0f, -2.5f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		dirtTexture.UseTexture();
		meshList[1]->RenderMesh();

		// MODEL 3
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 1.0f, -2.5f));
		model = glm::scale(model, glm::vec3(curSize, curSize, curSize));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		meshList[2]->RenderMesh();

		// MODEL 4
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 0.0f, -2.5f));
		model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		brickTexture.UseTexture();
		meshList[3]->RenderMesh();

		glUseProgram(0);

		// Swap drawn and drawing buffers
		mainWindow.swapBuffers();
	}
	return 0;
}
