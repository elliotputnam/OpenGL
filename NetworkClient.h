// NetworkClient.h
#pragma once
#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <glm/glm.hpp>
#include <vector>
#include <queue>
#include <chrono>
#pragma comment(lib, "ws2_32.lib")

#pragma pack(push,1)
struct SavedDataPacket {
    char type; // 'S' for saved data
    int id;
    float px, py, pz;
    float rx, ry, rz;
};
#pragma pack(pop)

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
    void update();
    bool receiveSnapshot(std::vector<HelicopterState>& out);
    bool receiveSavedData(SavedDataPacket& out);
    int getMyId() const;

private:
    SOCKET sock;
    sockaddr_in serverAddr;
    int myId;
    std::chrono::steady_clock::time_point lastHeartbeat;
    std::chrono::steady_clock::time_point lastStateUpdate;
    std::queue<SavedDataPacket> savedDataQueue;
};

#endif