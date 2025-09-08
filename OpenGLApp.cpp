#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glm/glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

const GLint WIDTH = 800, HEIGHT = 600;
const float toRadians = 3.14159265f / 180.0f;

GLuint VAO, VBO, shader, uniformModel;

bool direction = true;
float triOffset = 0.0f;
float triMaxOffset = 0.7f;
float triIncrement = 0.01f;
float currAngle = 0.0f;

bool sizeDirection = true;
float curSize = 0.4f;
float maxSize = 0.8f;
float minSize = 0.1f;

// Vertex Shader
static const char* vShader = "											\n\
#version 330															\n\
																		\n\
layout (location = 0) in vec3 pos;										\n\
																		\n\
																		\n\
uniform mat4 model;														\n\
																		\n\
																		\n\
void main()																\n\
{																		\n\
	gl_Position = model * vec4(pos, 1.0);								\n\
}";

// Fragment shader
static const char* fShader = "						\n\
#version 330										\n\
													\n\
out vec4 color;										\n\
													\n\
void main()											\n\
{													\n\
	color = vec4(0.0, 0.0, 1.0, 1.0);				\n\
}";

void CreateTriangle()
{
	GLfloat vertices[] =
	{
		// X, Y, Z
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		0.0f, 1.0f, 0.0f
	};

	// Binding VAO to VertexArray
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			// GL_STATIC_DRAW: Not changing array points
			// GL_DYNAIC_DRAW: Changing array points

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	// Unbinding from Array
	glBindVertexArray(0);

}

void AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType)
{
	// Creates empty shader that takes shader type
	GLuint theShader = glCreateShader(shaderType);

	// pointer to shader code
	const GLchar* theCode[1];
	theCode[0] = shaderCode;

	GLint codeLength[1];
	codeLength[0] = strlen(shaderCode);

	// grabs shader code and compiles it
	glShaderSource(theShader, 1, theCode, codeLength);
	glCompileShader(theShader);

	// Error logging
	GLint result = 0;
	GLchar eLog[1024] = { 0 };

	// Stores compile status into result of shader
	glGetShaderiv(theShader, GL_COMPILE_STATUS, &result);

	if (!result)
	{
		// Create log
		glGetShaderInfoLog(theShader, 1024, NULL, eLog);
		printf("Error compiling %d shader: %s.\n",shaderType, eLog);
		return;
	}
	// attaches shader to program
	glAttachShader(theProgram, theShader);
}

void CompileShaders()
{
	// create shader
	shader = glCreateProgram();

	// validate
	if (!shader)
	{
		printf("Shader failed to create.");
		return;
	}
	AddShader(shader, vShader, GL_VERTEX_SHADER);
	AddShader(shader, fShader, GL_FRAGMENT_SHADER);
	 
	// Error logging
	GLint result = 0;
	GLchar eLog[1024] = { 0 };

	// Creates executables on the GPU
	glLinkProgram(shader);
	// Stores link status into result of shader
	glGetProgramiv(shader, GL_LINK_STATUS, &result);

	if (!result)
	{
		// Create log
		glGetProgramInfoLog(shader, sizeof(eLog), NULL, eLog);
		printf("Error linking program: %s.\n", eLog);
		return;
	}

	glValidateProgram(shader);

	// Stores validation status into result of shader
	glGetProgramiv(shader, GL_VALIDATE_STATUS, &result);

	if (!result)
	{
		// Create log
		glGetProgramInfoLog(shader, sizeof(eLog), NULL, eLog);
		printf("Error validating program: %s.\n", eLog);
		return;
	}

	uniformModel = glGetUniformLocation(shader, "model");

}

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

	CreateTriangle();
	CompileShaders();

	// loop until window closes
	while (!glfwWindowShouldClose(mainWindow))
	{
		// Get / Handle input events
		glfwPollEvents();

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
		glClear(GL_COLOR_BUFFER_BIT);

		// assigns shader
		glUseProgram(shader);

		// Mat4 = Matrix 4x4
		glm::mat4 model(1.0f);

		//// translate(OBJECT, OFFSET)
		//model = glm::translate(model, glm::vec3(triOffset, 0.0f, 0.0f));
		//// rotate(OBJECT, ROTATION (in Rad), AXIS(x, y, z))
		//model = glm::rotate(model, currAngle * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(curSize, curSize, 1.0f));


		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));

		// assigns array
		glBindVertexArray(VAO);


		glDrawArrays(GL_TRIANGLES, 0, 3);

		// unassigns array
		// unassigns shader
		glBindVertexArray(0);
		glUseProgram(0);

		// Swap drawn and drawing buffers
		glfwSwapBuffers(mainWindow);
	}

	return 0;
}
