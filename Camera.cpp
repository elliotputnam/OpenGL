#include "Camera.h"

Camera::Camera(glm::vec3 startPosition, glm::vec3 startUp, GLfloat startYaw, GLfloat startPitch, GLfloat startMoveSpeed, GLfloat startTurnSpeed)
{
	position = startPosition;
	up = startUp;
	yaw = startYaw;
	pitch = startPitch;
	front = glm::vec3(0.0f, 0.0f, -1.0f);

	moveSpeed = startMoveSpeed;
	turnSpeed = startTurnSpeed;

	Update();
}

void Camera::KeyControl(bool* keys)
{
	if (keys[GLFW_KEY_W]) { position += front * moveSpeed; }
	if (keys[GLFW_KEY_A]) { position -= right * moveSpeed; }
	if (keys[GLFW_KEY_S]) { position -= front * moveSpeed; }
	if (keys[GLFW_KEY_D]) { position += right * moveSpeed; }
}

void Camera::Update()
{
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(front);

	// calc right using front and world so it's level to ground
	right = glm::normalize(glm::cross(front, worldUp));
	//calc up using right and front
	up = glm::normalize(glm::cross(right, front));
}

Camera::~Camera()
{

}
