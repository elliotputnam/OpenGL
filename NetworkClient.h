// NetworkClient.h
#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <glm/glm.hpp>
#include <vector>
#include <chrono>
#pragma comment(lib, "ws2_32.lib")

struct Vec3 { float x, y, z; };

struct HelicopterState {
    int id;
    glm::vec3 pos;
    glm::vec3 rot;
};

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();
    
    bool init(const char* serverIp, int port, int clientId);
    void sendState(const HelicopterState& s);
    void sendHeartbeat();
    void update(); // Call this every frame to auto-send heartbeats
    bool receiveSnapshot(std::vector<HelicopterState>& out);
    int getMyId() const;

private:
    SOCKET sock;
    sockaddr_in serverAddr;
    int myId;
    std::chrono::steady_clock::time_point lastHeartbeat;
    std::chrono::steady_clock::time_point lastStateUpdate;
};