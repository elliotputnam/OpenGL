#define STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>
#include <string>
#include <functional>

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
#include "City.h"

#include <assimp/BaseImporter.h>
#include <assimp/Importer.hpp>

NetworkClient net;
std::unordered_map<int, HelicopterState> otherHelis;
GLfloat lastNetworkSend = 0.0f;

const float toRadians = 3.14159265f / 180.0f;

GLuint uniformProjection = 0, uniformModel = 0, uniformView = 0, uniformEyePosition = 0,
uniformSpecularIntensity = 0, uniformShininess = 0, uniformDirectionalLightTransform = 0;

Window mainWindow;
std::vector<Shader> shaderList;
Shader directionalShadowShader;

Camera camera;

// TEXTURES
Texture brickTexture;
Texture dirtTexture;
Texture waterTexture;
Texture plainTexture;
Texture heliTexture;
Texture jetTexture;
Texture carrierTexture;

// MATERIALS
Material shinyMaterial;
Material dullMaterial;

Model heli;
Model jet;
Model carrier;

// NEW: City instance
City* city = nullptr;

// LIGHTS
DirectionalLight mainLight;
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight spotLights[MAX_SPOT_LIGHTS];

unsigned int pointLightCount = 0;
unsigned int spotLightCount = 0;

// TIMING
GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;

// Heli Variables
glm::vec3 helicopterPos(0.0f, 0.0f, 0.0f);
glm::vec3 helicopterRot(0.0f, 180.0f, 0.0f);

// Movement & rotation speeds
float moveSpeed = 1.5f;
float rotSpeed = 90.0f;
float smoothForward = 0.0f;
float smoothTurn = 0.0f;

// Username
std::string playerUsername;

// Vertex Shader
static const char* vShader = "Shaders/shader.vert";
static const char* fShader = "Shaders/shader.frag";

// Convert username to a consistent client ID using hash
int usernameToClientId(const std::string& username) {
	std::hash<std::string> hasher;
	size_t hash = hasher(username);
	return static_cast<int>((hash % 999999) + 1);
}

// Get username from console input
std::string getUsername() {
	std::string username;
	printf("========================================\n");
	printf("              FLY HIGH \n");
	printf("========================================\n");
	printf("Enter your username: ");
	std::getline(std::cin, username);

	while (username.empty() || username.length() > 32) {
		if (username.empty()) {
			printf("Username cannot be empty. Try again: ");
		}
		else {
			printf("Username too long (max 32 chars). Try again: ");
		}
		std::getline(std::cin, username);
	}

	return username;
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
	// Helicopter Model (local)
	// Jet Model
	{
		glm::mat4 model(1.0f);
		model = glm::translate(model, glm::vec3(helicopterPos.x, helicopterPos.y, helicopterPos.z));
		model = glm::rotate(model, helicopterRot.y * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, helicopterRot.x * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, helicopterRot.z * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
		// vertical model, rotatation for adjustments
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		model = glm::scale(model, glm::vec3(0.15f, 0.15f, 0.15f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		jetTexture.UseTexture();
		jet.RenderModel();
	}

	// Aircraft carrier model
	{
		glm::mat4 model(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		carrierTexture.UseTexture();
		carrier.RenderModel();
	}

	// Remote helicopters
	for (auto& kv : otherHelis) {
		const HelicopterState& h = kv.second;
		glm::mat4 model(1.0f);
		model = glm::translate(model, h.pos);
		model = glm::rotate(model, h.rot.y * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, h.rot.x * toRadians, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, h.rot.z * toRadians, glm::vec3(0.0f, 0.0f, 1.0f));
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

	// Render city
	if (city) {
		city->RenderCity(uniformModel, uniformSpecularIntensity, uniformShininess, deltaTime);
	}

	RenderScene();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPass(glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
	shaderList[0].UseShader();
	uniformModel = shaderList[0].GetModelLocation();
	uniformProjection = shaderList[0].GetProjectionLocation();
	uniformView = shaderList[0].GetViewLocation();
	uniformEyePosition = shaderList[0].GetEyePositionLocation();
	uniformSpecularIntensity = shaderList[0].GetSpecularIntensityLocation();
	uniformShininess = shaderList[0].GetShininessLocation();

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniform3f(uniformEyePosition, camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

	shaderList[0].SetDirectionalLight(&mainLight);
	shaderList[0].SetPointLights(pointLights, pointLightCount);
	shaderList[0].SetSpotLights(spotLights, spotLightCount);
	glm::mat4 mainlightTransform = mainLight.CalculateLightTransform();
	shaderList[0].SetDirectionalLightTransform(&mainlightTransform);

	mainLight.GetShadowMap()->Read(GL_TEXTURE1);
	shaderList[0].SetTexture(0);
	shaderList[0].SetDirectionalShadowMap(1);

	// Render city first
	if (city) {
		city->RenderCity(uniformModel, uniformSpecularIntensity, uniformShininess, deltaTime);
	}

	// Then render helicopters
	RenderScene();
}

int main()
{
	// Get username BEFORE creating window
	playerUsername = getUsername();
	int myClientId = usernameToClientId(playerUsername);

	printf("\nWelcome, %s! (ID: %d)\n", playerUsername.c_str(), myClientId);
	printf("Connecting to server...\n\n");

	mainWindow = Window(WINDOW_WIDTH, WINDOW_HEIGHT);
	mainWindow.Initialize();

	// Initialize network
	if (!net.init("70.172.165.98", SERVER_PORT, myClientId)) {
		printf("Failed to init network client\n");
	}
	else {
		printf("Network initialized successfully!\n");
	}

	HelicopterState myState{};
	myState.id = myClientId;
	myState.pos = glm::vec3(0, 0, 0);
	myState.rot = glm::vec3(0, 180, 0);

	otherHelis.clear();

	CreateShaders();

	camera = Camera(
		glm::vec3(0.0f, 2.0f, 5.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		90.0f, -15.0f, 3.0f, 0.05f
	);

	brickTexture = Texture("Textures/brick.png");
	brickTexture.LoadTextureA();
	dirtTexture = Texture("Textures/dirt.png");
	dirtTexture.LoadTextureA();
	plainTexture = Texture("Textures/plain.png");
	plainTexture.LoadTextureA();
	waterTexture = Texture("Textures/water.jpg");
	waterTexture.LoadTexture();
	heliTexture = Texture("Textures/TEX_SBMP.jpg");
	heliTexture.LoadTexture();
	

	shinyMaterial = Material(1.0f, 32);
	dullMaterial = Material(0.3f, 4);

	heli = Model();
	heli.LoadModel("Models/Seahawk.obj");
	jet = Model();
	jet.LoadModel("Models/jet.obj");
	carrier = Model();
	carrier.LoadModel("Models/essex_scb-125_generic.obj");

	// init city eventually 
	city = new City();
	city->SetTextures(&brickTexture, &dirtTexture);
	city->SetTexture(&waterTexture);
	city->SetMaterials(&shinyMaterial, &dullMaterial);
	city->CreateCity();

	mainLight = DirectionalLight(2048, 2048, 1.0f, 1.0f, 1.0f, 0.01f, 0.3f, 0.0f, -15.0f, -10.0f);

	pointLights[0] = PointLight(0.0f, 0.0f, 1.0f, 0.0f, 0.5f, -3.5f, 0.0f, 3.5f, 0.3f, 0.2f, 0.1f);
	pointLightCount++;
	pointLights[1] = PointLight(0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 3.5f, 0.0f, 3.5f, 0.3f, 0.1f, 0.1f);
	pointLightCount++;
	pointLights[2] = PointLight(1.0f, 0.0f, 0.0f, 0.0f, 0.5f, -3.5f, 0.0f, -3.5f, 0.3f, 0.1f, 0.1f);
	pointLightCount++;
	pointLights[3] = PointLight(1.0f, 1.0f, 1.0f, 0.0f, 0.5f, 3.5f, 0.0f, -3.5f, 0.3f, 0.1f, 0.1f);
	pointLightCount++;

	spotLights[0] = SpotLight(1.0f, 1.0f, 1.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.01f, 0.01f, 20.0f);
	spotLightCount++;
	spotLights[1] = SpotLight(1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -100.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 20.0f);
	spotLightCount++;

	glm::mat4 projection = glm::perspective(45.0f, mainWindow.getBufferWidth() / mainWindow.getBufferHeight(), 0.1f, 100.0f);

	Assimp::Importer importer;

	// Send initial state
	net.sendState(myState);

	bool receivedSavedData = false;

	while (!mainWindow.getWindowShouldClose())
	{
		GLfloat now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		glfwPollEvents();
		camera.FollowTarget(helicopterPos, helicopterRot, deltaTime);

		// HELI CONTROLS
		bool* keys = mainWindow.getsKeys();
		glm::vec3 tempPosition = helicopterPos;
		glm::vec3 tempRotation = helicopterRot;

		float maxPitch = 20.0f, maxRoll = 20.0f, tiltSpeed = 20.0f;

		float turnInput = 0.0f;
		if (keys[GLFW_KEY_LEFT])  turnInput = 1.0f;
		if (keys[GLFW_KEY_RIGHT]) turnInput = -1.0f;
		helicopterRot.y += turnInput * rotSpeed * deltaTime;

		float forwardInput = 0.0f;
		if (keys[GLFW_KEY_UP])   forwardInput = -5.0f;
		if (keys[GLFW_KEY_DOWN]) forwardInput = 0.5f;

		// Vertical movement
		float verticalInput = 0.0f;
		if (keys[GLFW_KEY_W]) verticalInput = 1.0f;  // up
		if (keys[GLFW_KEY_S]) verticalInput = -1.0f; // down
		helicopterPos.y += verticalInput * moveSpeed * deltaTime;

		float yawRad = glm::radians(helicopterRot.y);
		glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));
		helicopterPos += forward * (forwardInput * moveSpeed * deltaTime);

		float targetPitch = -forwardInput * maxPitch;
		float targetRoll = -turnInput * maxRoll;
		//helicopterRot.x = glm::mix(helicopterRot.x, targetPitch, tiltSpeed * deltaTime);
		helicopterRot.z = glm::mix(helicopterRot.z, targetRoll, tiltSpeed * deltaTime);

		// NETWORK SEND
		bool hasChanged = (tempPosition != helicopterPos || tempRotation != helicopterRot);
		bool shouldSend = (now - lastNetworkSend) >= NETWORK_SEND_INTERVAL;

		if (hasChanged && shouldSend) {
			myState.pos = helicopterPos;
			myState.rot = helicopterRot;
			myState.id = myClientId;
			net.sendState(myState);
			lastNetworkSend = now;
		}

		net.update();

		// RECEIVE PACKETS
		std::vector<HelicopterState> snapshot;
		if (net.receiveSnapshot(snapshot)) {
			for (auto& s : snapshot) {
				if (s.id == myClientId) continue;
				otherHelis[s.id] = s;
			}
		}

		// Check for saved data packet from server
		SavedDataPacket savedPkt;
		if (net.receiveSavedData(savedPkt)) {
			if (savedPkt.type == 'S' && savedPkt.id == myClientId) {
				printf("[CLIENT] Received saved position from server!\n");
				printf("  Position: (%.2f, %.2f, %.2f)\n", savedPkt.px, savedPkt.py, savedPkt.pz);
				printf("  Rotation: (%.2f, %.2f, %.2f)\n", savedPkt.rx, savedPkt.ry, savedPkt.rz);

				helicopterPos = glm::vec3(savedPkt.px, savedPkt.py, savedPkt.pz);
				helicopterRot = glm::vec3(savedPkt.rx, savedPkt.ry, savedPkt.rz);

				myState.pos = helicopterPos;
				myState.rot = helicopterRot;

				receivedSavedData = true;
			}
		}

		DirectionalShadowMapPass(&mainLight);
		RenderPass(projection, camera.calculateViewMatrix());
		mainWindow.swapBuffers();
	}

	// NEW: Cleanup
	if (city) {
		delete city;
		city = nullptr;
	}

	return 0;
}