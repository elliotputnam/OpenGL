#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <glm/glm.hpp>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

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
    bool receiveSnapshot(std::vector<HelicopterState>& out);

private:
    SOCKET sock;
    sockaddr_in serverAddr;
    int myId;
};