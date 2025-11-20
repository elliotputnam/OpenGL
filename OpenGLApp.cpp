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

#include "CommonValues.h"

#include "Window.h"
#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"
#include "Light.h"
#include "Material.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Model.h"
#include "NetworkClient.h"

#include <assimp/BaseImporter.h>
#include <assimp/Importer.hpp>

NetworkClient net;
std::unordered_map<int, HelicopterState> otherHelis;
GLfloat lastNetworkSend = 0.0f;

const float toRadians = 3.14159265f / 180.0f;

GLuint uniformProjection = 0, uniformModel = 0, uniformView = 0, uniformEyePosition = 0,
uniformSpecularIntensity = 0, uniformShininess = 0, uniformDirectionalLightTransform = 0;

Window mainWindow;
std::vector<Mesh*> meshList;
std::vector<Shader> shaderList;
Shader directionalShadowShader;

Camera camera;

// TEXTURES
Texture brickTexture;
Texture dirtTexture;
Texture plainTexture;
Texture heliTexture;

// MATERIALS
Material shinyMaterial;
Material dullMaterial;

Model heli;
Model penguin;

// LIGHTS
DirectionalLight mainLight;
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight spotLights[MAX_SPOT_LIGHTS];

unsigned int pointLightCount = 0;
unsigned int spotLightCount = 0;

// TIMING
GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;

// movement testing variables
bool direction = true;
float triOffset = 0.0f;
float triMaxOffset = 0.4f;
float triIncrement = 0.001f;

// rotation testing variables
float currAngle = 0.0f;

// Size testing variables
bool sizeDirection = true;
float curSize = 0.3f;
float maxSize = 0.4f;
float minSize = 0.2f;

// Heli Variables
glm::vec3 helicopterPos(0.0f, 0.0f, 0.0f);
glm::vec3 helicopterRot(0.0f, 180.0f, 0.0f);

// Movement & rotation speeds (units per second / degrees per second)
float moveSpeed = 1.5f;   // tune this for your scale
float rotSpeed = 90.0f;   // degrees per second
float smoothForward = 0.0f;
float smoothTurn = 0.0f;


// Vertex Shader
// clamp() keeps input values within specified range (0 - 1), removing negatives in position.
static const char* vShader = "Shaders/shader.vert";

// Fragment shader
static const char* fShader = "Shaders/shader.frag";

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
		//X     Y     Z			U     V			nx	  ny	nz
		-1.0f, -1.0f, -0.5f,	0.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 1.0f,		0.5f, 0.0f,		0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, -0.5f,		1.0f, 0.0f,		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,		0.5f, 1.0f,		0.0f, 0.0f, 0.0f
	};

	unsigned int floorIndices[] =
	{
		0, 2, 1,
		1, 2, 3
	};

	GLfloat floorVertices[] =
	{
		//X     Y     Z			U     V			nx	  ny	nz
		-10.f, 0.0f, -10.0f,	0.0f, 0.0f,		0.0f, -1.0f, 0.0f,
		10.0f, 0.0f, -10.0f,	10.0f, 0.0f,	0.0f, -1.0f, 0.0f,
		-10.0f, 0.0f, 10.0f,	0.0f, 10.0f,	0.0f, -1.0f, 0.0f,
		10.0f, 0.0f, 10.0f,		10.0f, 10.0f,	0.0f, -1.0f, 0.0f
	};

	unsigned int wallIndices[] =
	{
		0, 2, 1,
		1, 2, 3
	};

	GLfloat wallVertices[] =
	{
		//X     Y     Z			U     V			nx	  ny	nz
		-10.0f, 0.0f, -10.0f,	0.0f, 0.0f,		0.0f, -1.0f, 0.0f,
		10.0f, 0.0f, -10.0f,	10.0f, 0.0f,	0.0f, -1.0f, 0.0f,
		-10.0f, 5.0f, -10.0f,	0.0f, 10.0f,	0.0f, -1.0f, 0.0f,
		10.0f, 5.0f, -10.0f,	10.0f, 10.0f,	0.0f, -1.0f, 0.0f
	};

	calcAverageNormals(indices, 12, vertices, 32, 8, 5);

	Mesh* pyramidOne = new Mesh();
	pyramidOne->CreateMesh(vertices, indices, 32, 12);
	meshList.push_back(pyramidOne);

	Mesh* pyramidTwo = new Mesh();
	pyramidTwo->CreateMesh(vertices, indices, 32, 12);
	meshList.push_back(pyramidTwo);

	Mesh* pyramidThree = new Mesh();
	pyramidThree->CreateMesh(vertices, indices, 32, 12);
	meshList.push_back(pyramidThree);

	Mesh* pyramidFour = new Mesh();
	pyramidFour->CreateMesh(vertices, indices, 32, 12);
	meshList.push_back(pyramidFour);

	Mesh* floor = new Mesh();
	floor->CreateMesh(floorVertices, floorIndices, 32, 6);
	meshList.push_back(floor);

	Mesh* nWall = new Mesh();
	nWall->CreateMesh(wallVertices, wallIndices, 32, 6);
	meshList.push_back(nWall);

	Mesh* wWall = new Mesh();
	wWall->CreateMesh(wallVertices, wallIndices, 32, 6);
	meshList.push_back(wWall);

	Mesh* sWall = new Mesh();
	sWall->CreateMesh(wallVertices, wallIndices, 32, 6);
	meshList.push_back(sWall);

	Mesh* eWall = new Mesh();
	eWall->CreateMesh(wallVertices, wallIndices, 32, 6);
	meshList.push_back(eWall);
}

void CreateShaders()
{
	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(vShader, fShader);
	shaderList.push_back(*shader1);

	directionalShadowShader.CreateFromFiles("Shaders/directional_shadow_map.vert", "Shaders/directional_shadow_map.frag");
}

void RenderScene()
{
	// MODEL 1 (rotating)
	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(3.0f, 0.0f, -2.5f));
	model = glm::rotate(model, currAngle * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	brickTexture.UseTexture();
	shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[0]->RenderMesh();

	// MODEL 2 (moving)
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-triOffset + 1.0f, 0.0f, -2.5f));
	model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	dirtTexture.UseTexture();
	meshList[1]->RenderMesh();

	// MODEL 3 (growing/shrinking)
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -2.5f));
	model = glm::scale(model, glm::vec3(curSize, curSize, curSize));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	meshList[2]->RenderMesh();

	// MODEL 4 (static)
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-3.0f, 0.0f, -2.5f));
	model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	brickTexture.UseTexture();
	meshList[3]->RenderMesh();

	// Floor
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	dirtTexture.UseTexture();
	meshList[4]->RenderMesh();

	// North Wall
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	brickTexture.UseTexture();
	meshList[5]->RenderMesh();

	// West Wall
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::rotate(model, 90 * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	brickTexture.UseTexture();
	meshList[6]->RenderMesh();

	// South Wall
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::rotate(model, 180 * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	brickTexture.UseTexture();
	meshList[7]->RenderMesh();

	// East Wall
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	model = glm::rotate(model, -90 * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	brickTexture.UseTexture();
	meshList[8]->RenderMesh();

	// Helicopter Model (local) — apply yaw, pitch, roll
	{
		glm::mat4 model(1.0f);
		model = glm::translate(model, glm::vec3(helicopterPos.x, helicopterPos.y, helicopterPos.z));

		// Apply rotations: yaw (Y), pitch (X), roll (Z)
		model = glm::rotate(model, helicopterRot.y * toRadians, glm::vec3(0.0f, 1.0f, 0.0f)); // yaw
		model = glm::rotate(model, helicopterRot.x * toRadians, glm::vec3(1.0f, 0.0f, 0.0f)); // pitch
		model = glm::rotate(model, helicopterRot.z * toRadians, glm::vec3(0.0f, 0.0f, 1.0f)); // roll

		model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		heliTexture.UseTexture();
		heli.RenderModel();
	}


	// Remote helicopters (apply yaw, pitch, roll)
	for (auto& kv : otherHelis) {
		const HelicopterState& h = kv.second;

		glm::mat4 model(1.0f);
		model = glm::translate(model, h.pos);

		model = glm::rotate(model, h.rot.y * toRadians, glm::vec3(0.0f, 1.0f, 0.0f)); // yaw
		model = glm::rotate(model, h.rot.x * toRadians, glm::vec3(1.0f, 0.0f, 0.0f)); // pitch
		model = glm::rotate(model, h.rot.z * toRadians, glm::vec3(0.0f, 0.0f, 1.0f)); // roll

		model = glm::scale(model, glm::vec3(0.01f));

		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		heliTexture.UseTexture();
		heli.RenderModel();
	}



}

void DirectionalShadowMapPass(DirectionalLight* light)
{
	directionalShadowShader.UseShader();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();
	glClear(GL_DEPTH_BUFFER_BIT);

	uniformModel = directionalShadowShader.GetModelLocation();
	glm::mat4 dirLightTransform = light->CalculateLightTransform();
	directionalShadowShader.SetDirectionalLightTransform(&dirLightTransform);

	RenderScene();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPass(glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
	shaderList[0].UseShader();

	// assign uniforms
	uniformModel = shaderList[0].GetModelLocation();
	uniformProjection = shaderList[0].GetProjectionLocation();
	uniformView = shaderList[0].GetViewLocation();
	uniformEyePosition = shaderList[0].GetEyePositionLocation();
	uniformSpecularIntensity = shaderList[0].GetSpecularIntensityLocation();
	uniformShininess = shaderList[0].GetShininessLocation();

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	// Clear window
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniform3f(uniformEyePosition, camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

	// Assign lighting to shader
	shaderList[0].SetDirectionalLight(&mainLight);
	shaderList[0].SetPointLights(pointLights, pointLightCount);
	shaderList[0].SetSpotLights(spotLights, spotLightCount);
	glm::mat4 mainlightTransform = mainLight.CalculateLightTransform();
	shaderList[0].SetDirectionalLightTransform(&mainlightTransform);

	mainLight.GetShadowMap()->Read(GL_TEXTURE1);
	shaderList[0].SetTexture(0);
	shaderList[0].SetDirectionalShadowMap(1);

	glm::vec3 flashlightLocation = camera.getCameraPosition();
	flashlightLocation.y -= 0.3f;
	//spotLights[0].SetFlash(flashlightLocation, camera.getCameraDirection());

	RenderScene();
}

int main()
{
	mainWindow = Window(WINDOW_WIDTH, WINDOW_HEIGHT);
	mainWindow.Initialize();

	//int myClientId = 0;
	// generate a non-zero client id (random)
	std::srand((unsigned)time(nullptr));
	int myClientId = (std::rand() % 100000) + 1; // 1..100000

	printf("Connecting to server with clientId=%d...\n", myClientId);

	// use the global `net` (do NOT redeclare locally)
	if (!net.init("127.0.0.1", SERVER_PORT, myClientId)) {
		printf("Failed to init network client\n");
		// you might still want to continue in local-only mode
	}
	else {
		printf("Network initialized\n");
	}
	HelicopterState myState{};
	myState.id = myClientId;
	myState.pos = glm::vec3(0, 0, 0);
	myState.rot = glm::vec3(0, 0, 0);

	std::unordered_map<int, HelicopterState> others;

	// other players
	otherHelis.clear();

	CreateObjects();
	CreateShaders();

	// init camera
	//camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 3.0f, 0.05f);
	// Initialize camera BEHIND the helicopter for third-person view
	camera = Camera(
		glm::vec3(0.0f, 2.0f, 5.0f),   // Position: behind and above helicopter
		glm::vec3(0.0f, 1.0f, 0.0f),    // World up direction
		90.0f,                          // Yaw: facing forward
		-15.0f,                          // Pitch: angled down slightly to see helicopter
		3.0f,                            // Move speed (for free cam mode)
		0.05f                            // Turn speed (for free cam mode)
	);
	// load textures
	brickTexture = Texture("Textures/brick.png");
	brickTexture.LoadTextureA();
	dirtTexture = Texture("Textures/dirt.png");
	dirtTexture.LoadTextureA();
	plainTexture = Texture("Textures/plain.png");
	plainTexture.LoadTextureA();
	heliTexture = Texture("Textures/TEX_SBMP.jpg");
	heliTexture.LoadTexture();

	// Specular lighting
	shinyMaterial = Material(1.0f, 32);
	dullMaterial = Material(0.3f, 4);

	// create Heli Model
	heli = Model();
	heli.LoadModel("Models/Seahawk.obj");

	// load lighting
	mainLight = DirectionalLight
	(
		2048, 2048,				// default size
		1.0f, 1.0f, 1.0f,		// R G B
		0.01f, 0.3f,			// ambient + diffuse intensity
		0.0f, -15.0f, -10.0f	// direction
	);

	pointLights[0] = PointLight(0.0f, 0.0f, 1.0f,	// R G B
		0.0f, 0.5f,			// ambient + diffuse intensity
		-3.5f, 0.0f, 3.5f,	// position
		0.3f, 0.2f, 0.1f);	// constant, linear, exponent
	pointLightCount++;
	pointLights[1] = PointLight(0.0f, 1.0f, 0.0f,
		0.0f, 0.5f,
		3.5f, 0.0f, 3.5f,
		0.3f, 0.1f, 0.1f);
	pointLightCount++;
	pointLights[2] = PointLight(1.0f, 0.0f, 0.0f,
		0.0f, 0.5f,
		-3.5f, 0.0f, -3.5f,
		0.3f, 0.1f, 0.1f);
	pointLightCount++;
	pointLights[3] = PointLight(1.0f, 1.0f, 1.0f,
		0.0f, 0.5f,
		3.5f, 0.0f, -3.5f,
		0.3f, 0.1f, 0.1f);
	pointLightCount++;

	spotLights[0] = SpotLight
	(
		1.0f, 1.0f, 1.0f,		// R G B
		0.0f, 2.0f,				// ambient + diffuse intensity
		0.0f, 0.0f, 0.0f,		// position
		0.0f, -1.0f, 0.0f,		// direction
		1.0f, 0.01f, 0.01f,		// constant, linear, exponent
		20.0f					// edge (angle)
	);
	spotLightCount++;

	spotLights[1] = SpotLight
	(
		1.0f, 1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f, 0.0f,
		-100.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		20.0f
	);
	spotLightCount++;

	// setup projection
	glm::mat4 projection = glm::perspective(45.0f, mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 100.0f);

	Assimp::Importer importer;

	// sends init
	net.sendState(myState);



	// loop until window closes
	while (!mainWindow.getWindowShouldClose())
	{
		GLfloat now = glfwGetTime(); // SDL_GetPerformanceCounter(); milliseconds
		deltaTime = now - lastTime; // (now - lastTime)*1000 / SDL_GetPerformanceCounter() seconds
		lastTime = now;

		// Get / Handle input events
		glfwPollEvents();
		// Passes inputs into camera
		//camera.KeyControl(mainWindow.getsKeys(), deltaTime);
		//camera.MouseControl(mainWindow.getXChange(), mainWindow.getYChange());

		camera.FollowTarget(helicopterPos, helicopterRot, deltaTime);

		// *********** MODEL MOVEMENT ************* // 

		// TRANSLATION (demo triangles)
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


		// HELI ROTATION and MOVEMENT
		bool* keys = mainWindow.getsKeys();
    
    glm::vec3 tempPosition = helicopterPos;
    glm::vec3 tempRotation = helicopterRot;

    // Tilt amounts
    float maxPitch = 20.0f;
    float maxRoll = 20.0f;
    float tiltSpeed = 20.0f;

    float turnInput = 0.0f;
    if (keys[GLFW_KEY_LEFT])  turnInput = 1.0f;
    if (keys[GLFW_KEY_RIGHT]) turnInput = -1.0f;

    helicopterRot.y += turnInput * rotSpeed * deltaTime;

    float forwardInput = 0.0f;
    if (keys[GLFW_KEY_UP])   forwardInput = -1.0f;
    if (keys[GLFW_KEY_DOWN]) forwardInput = 0.5f;

    float yawRad = glm::radians(helicopterRot.y);
    glm::vec3 forward(
        -sin(yawRad),
        0.0f,
        -cos(yawRad)
    );

    helicopterPos += forward * (forwardInput * moveSpeed * deltaTime);

    float targetPitch = -forwardInput * maxPitch;
    float targetRoll = -turnInput * maxRoll;

    helicopterRot.x = glm::mix(helicopterRot.x, targetPitch, tiltSpeed * deltaTime);
    helicopterRot.z = glm::mix(helicopterRot.z, targetRoll, tiltSpeed * deltaTime);

    // SEND STATE AT FIXED RATE (not every frame)
    bool hasChanged = (tempPosition != helicopterPos || tempRotation != helicopterRot);
    bool shouldSend = (now - lastNetworkSend) >= NETWORK_SEND_INTERVAL;
    
    if (hasChanged && shouldSend) {
        myState.pos = helicopterPos;
        myState.rot = helicopterRot;
        myState.id = myClientId;
        
        net.sendState(myState);
        lastNetworkSend = now;
    }

    // Always call update for heartbeat logic
    net.update();

    // RECEIVE ALL SNAPSHOTS (drain queue)
    std::vector<HelicopterState> snapshot;
    if (net.receiveSnapshot(snapshot)) {
        // Update remote helicopters
        for (auto& s : snapshot) {
            if (s.id == myClientId) continue; // skip self
            otherHelis[s.id] = s;
        }
        // Removed debug printf - only log in debug builds
        #ifdef _DEBUG
        // printf("Received snapshot with %d helis\n", (int)snapshot.size());
        #endif
    }

		// render scene to buffer
		DirectionalShadowMapPass(&mainLight);
		RenderPass(projection, camera.calculateViewMatrix());

		// Swap drawn and drawing buffers
		mainWindow.swapBuffers();
	}
	return 0;
}
