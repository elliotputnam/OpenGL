#include "Camera.h"

Camera::Camera() {}

Camera::Camera(glm::vec3 startPosition, glm::vec3 startUp, GLfloat startYaw, GLfloat startPitch, GLfloat startMoveSpeed, GLfloat startTurnSpeed)
{
    position = startPosition;
    worldUp = startUp;
    yaw = startYaw;
    pitch = startPitch;
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    moveSpeed = startMoveSpeed;
    turnSpeed = startTurnSpeed;
    Update();
}

void Camera::KeyControl(bool* keys, GLfloat deltaTime)
{
    // Optional: Keep this for free-cam mode, or disable it for locked third-person
    GLfloat velocity = moveSpeed * deltaTime;
    if (keys[GLFW_KEY_W]) { position += front * velocity; }
    if (keys[GLFW_KEY_A]) { position -= right * velocity; }
    if (keys[GLFW_KEY_S]) { position -= front * velocity; }
    if (keys[GLFW_KEY_D]) { position += right * velocity; }
    if (keys[GLFW_KEY_Q]) { position -= worldUp * velocity; }
    if (keys[GLFW_KEY_E]) { position += worldUp * velocity; }
}

void Camera::MouseControl(GLfloat xChange, GLfloat yChange)
{
    // Optional: Allow player to orbit camera around helicopter
    xChange *= turnSpeed;
    yChange *= turnSpeed;
    yaw += xChange;
    pitch += yChange;

    if (pitch > 75.0f)
    {
        pitch = 75.0f;
    }
    if (pitch < -75.0f)
    {
        pitch = -75.0f;
    }
    Update();
}

// New method: Follow the helicopter in third-person
void Camera::FollowTarget(glm::vec3 targetPosition, glm::vec3 targetRotation, GLfloat deltaTime)
{
    // Camera offset from helicopter (behind and above)
    float distanceBehind = 5.0f;  // How far behind
    float heightAbove = 2.0f;     // How high above

    // Calculate camera position based on helicopter's yaw
    float yawRad = glm::radians(targetRotation.y);

    // Position camera behind the helicopter
    glm::vec3 offset(
        -sin(yawRad) * distanceBehind,  // X offset
        heightAbove,                    // Y offset (height)
        -cos(yawRad) * distanceBehind   // Z offset
    );

    // Smooth camera movement (lerp for smoothness)
    float smoothSpeed = 8.0f; // Adjust for more/less responsiveness
    glm::vec3 desiredPosition = targetPosition + offset;
    position = glm::mix(position, desiredPosition, smoothSpeed * deltaTime);

    // Make camera look at the helicopter (slightly above it)
    glm::vec3 lookAtTarget = targetPosition + glm::vec3(0.0f, 0.5f, 0.0f);
    glm::vec3 direction = glm::normalize(lookAtTarget - position);

    // Update camera's front vector to look at target
    front = direction;

    // Recalculate right and up vectors
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

glm::vec3 Camera::getCameraPosition()
{
    return position;
}

glm::vec3 Camera::getCameraDirection()
{
    return glm::normalize(front);
}

glm::mat4 Camera::calculateViewMatrix()
{
    return glm::lookAt(position, position + front, up);
}

void Camera::Update()
{
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

Camera::~Camera()
{
}