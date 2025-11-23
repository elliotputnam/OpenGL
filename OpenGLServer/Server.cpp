#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#pragma comment(lib, "ws2_32.lib")

#pragma pack(push,1)
struct HelicopterStatePOD {
    int id;
    float px, py, pz;
    float rx, ry, rz;
};

struct SnapshotPacket {
    int count;
    HelicopterStatePOD states[32];
};

struct HeartbeatPacket {
    int id;
    char type;
};

// Packet sent to client with their saved data
struct SavedDataPacket {
    char type; // 'S' for saved data
    int id;
    float px, py, pz;
    float rx, ry, rz;
};
#pragma pack(pop)

struct ClientInfo {
    sockaddr_in addr;
    HelicopterStatePOD state;
    std::chrono::steady_clock::time_point lastSeen;
};

// save data
const char* DATA_FILE = "client_data.json";
void saveToJson(const std::unordered_map<int, HelicopterStatePOD>& data) {
    std::ofstream file(DATA_FILE);
    if (!file.is_open()) {
        std::cerr << "[SERVER] Failed to open file for writing\n";
        return;
    }

    file << "{\n";
    bool first = true;
    for (const auto& kv : data) {
        if (!first) file << ",\n";
        first = false;
        const auto& s = kv.second;
        file << "  \"" << kv.first << "\": {"
            << "\"id\":" << s.id << ","
            << "\"px\":" << std::fixed << std::setprecision(4) << s.px << ","
            << "\"py\":" << s.py << ","
            << "\"pz\":" << s.pz << ","
            << "\"rx\":" << s.rx << ","
            << "\"ry\":" << s.ry << ","
            << "\"rz\":" << s.rz << "}";
    }
    file << "\n}\n";
    file.close();
    std::cout << "[SERVER] Saved " << data.size() << " client states to " << DATA_FILE << "\n";
}

std::unordered_map<int, HelicopterStatePOD> loadFromJson() {
    std::unordered_map<int, HelicopterStatePOD> data;
    std::ifstream file(DATA_FILE);
    if (!file.is_open()) {
        std::cout << "[SERVER] No existing data file found, starting fresh\n";
        return data;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // JSON parsing
    size_t pos = 0;
    while ((pos = content.find("\"id\":", pos)) != std::string::npos) {
        HelicopterStatePOD s{};
        sscanf(content.c_str() + pos,
            "\"id\":%d,\"px\":%f,\"py\":%f,\"pz\":%f,\"rx\":%f,\"ry\":%f,\"rz\":%f",
            &s.id, &s.px, &s.py, &s.pz, &s.rx, &s.ry, &s.rz);
        data[s.id] = s;
        pos++;
    }

    std::cout << "[SERVER] Loaded " << data.size() << " client states from " << DATA_FILE << "\n";
    return data;
}

int main() {
    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8008);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));

    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    std::cout << "UDP server running on port 8008 (with persistence)\n";

    // Load saved client data from JSON
    std::unordered_map<int, HelicopterStatePOD> savedData = loadFromJson();

    std::unordered_map<int, ClientInfo> clients;
    const int TIMEOUT_MS = 5000;
    const int BROADCAST_INTERVAL_MS = 50;
    const int SAVE_INTERVAL_MS = 10000; // Save to file every 10 seconds

    auto lastCheck = std::chrono::steady_clock::now();
    auto lastBroadcast = std::chrono::steady_clock::now();
    auto lastSave = std::chrono::steady_clock::now();

    while (true) {
        auto now = std::chrono::steady_clock::now();

        // 1) RECEIVE PACKETS
        char buffer[1024];
        sockaddr_in clientAddr{};
        int addrLen = sizeof(clientAddr);

        int recvd = recvfrom(sock, buffer, sizeof(buffer), 0,
            (sockaddr*)&clientAddr, &addrLen);

        if (recvd > 0) {
            if (recvd == sizeof(HeartbeatPacket) && buffer[sizeof(int)] == 'H') {
                HeartbeatPacket* hb = (HeartbeatPacket*)buffer;
                if (clients.find(hb->id) != clients.end()) {
                    clients[hb->id].lastSeen = now;
                }
            }
            else if (recvd == sizeof(HelicopterStatePOD)) {
                HelicopterStatePOD* incoming = (HelicopterStatePOD*)buffer;
                bool isNew = (clients.find(incoming->id) == clients.end());

                if (isNew) {
                    char ipStr[32]{};
                    inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
                    std::cout << "[SERVER] New client connected! ID=" << incoming->id
                        << "  IP=" << ipStr
                        << "  Port=" << ntohs(clientAddr.sin_port) << "\n";
                    std::cout << "Current clients: " << (clients.size() + 1) << "\n";

                    // Check if we have saved data for this client
                    if (savedData.find(incoming->id) != savedData.end()) {
                        std::cout << "[SERVER] Sending saved data to client ID=" << incoming->id << "\n";
                        const auto& s = savedData[incoming->id];
                        SavedDataPacket savedPkt;
                        savedPkt.type = 'S';
                        savedPkt.id = s.id;
                        savedPkt.px = s.px; savedPkt.py = s.py; savedPkt.pz = s.pz;
                        savedPkt.rx = s.rx; savedPkt.ry = s.ry; savedPkt.rz = s.rz;
                        sendto(sock, (char*)&savedPkt, sizeof(savedPkt), 0,
                            (sockaddr*)&clientAddr, sizeof(clientAddr));
                    }
                }

                clients[incoming->id].addr = clientAddr;
                clients[incoming->id].state = *incoming;
                clients[incoming->id].lastSeen = now;

                // Update saved data
                savedData[incoming->id] = *incoming;
            }
        }

        // 2) PERIODIC SNAPSHOT BROADCAST
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastBroadcast).count() >= BROADCAST_INTERVAL_MS) {
            lastBroadcast = now;
            if (!clients.empty()) {
                SnapshotPacket snapshot{};
                snapshot.count = (int)clients.size();
                int i = 0;
                for (auto& kv : clients) {
                    snapshot.states[i++] = kv.second.state;
                }
                for (auto& kv : clients) {
                    sendto(sock, (char*)&snapshot,
                        sizeof(int) + snapshot.count * sizeof(HelicopterStatePOD),
                        0, (sockaddr*)&kv.second.addr, sizeof(kv.second.addr));
                }
            }
        }

        // 3) PERIODIC SAVE TO JSON
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSave).count() >= SAVE_INTERVAL_MS) {
            lastSave = now;
            if (!savedData.empty()) {
                saveToJson(savedData);
            }
        }

        // 4) CHECK FOR TIMEOUTS
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCheck).count() > 1000) {
            lastCheck = now;
            std::vector<int> toRemove;
            for (auto& kv : clients) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - kv.second.lastSeen).count();
                if (elapsed > TIMEOUT_MS) {
                    toRemove.push_back(kv.first);
                }
            }
            for (int id : toRemove) {
                char ipStr[32]{};
                inet_ntop(AF_INET, &clients[id].addr.sin_addr, ipStr, sizeof(ipStr));
                std::cout << "[SERVER] Client disconnected (timeout)! ID=" << id
                    << "  IP=" << ipStr << "\n";
                std::cout << "Current clients: " << (clients.size() - 1) << "\n";
                clients.erase(id);
                // keep savedData even after disconnect
            }
        }

        Sleep(1);
    }

    // Save before exit
    saveToJson(savedData);
    closesocket(sock);
    WSACleanup();
    return 0;
}