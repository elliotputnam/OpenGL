// server_udp.cpp
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>
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

// Heartbeat packet (small packet sent periodically)
struct HeartbeatPacket {
    int id;
    char type; // 'H' for heartbeat
};
#pragma pack(pop)

struct ClientInfo {
    sockaddr_in addr;
    HelicopterStatePOD state;
    std::chrono::steady_clock::time_point lastSeen;
};

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

    std::cout << "UDP server running on port 8008 (with heartbeat)\n";

    std::unordered_map<int, ClientInfo> clients;
    const int TIMEOUT_MS = 5000; // 5 seconds timeout
    const int BROADCAST_INTERVAL_MS = 50; // Broadcast snapshots every 50ms (20 Hz)

    auto lastCheck = std::chrono::steady_clock::now();
    auto lastBroadcast = std::chrono::steady_clock::now();

    while (true) {
        auto now = std::chrono::steady_clock::now();

        // 1) RECEIVE PACKETS (STATE or HEARTBEAT)
        char buffer[1024];
        sockaddr_in clientAddr{};
        int addrLen = sizeof(clientAddr);

        int recvd = recvfrom(
            sock,
            buffer,
            sizeof(buffer),
            0,
            (sockaddr*)&clientAddr,
            &addrLen
        );

        if (recvd > 0) {
            // Check packet type
            if (recvd == sizeof(HeartbeatPacket) && buffer[sizeof(int)] == 'H') {
                // HEARTBEAT PACKET
                HeartbeatPacket* hb = (HeartbeatPacket*)buffer;
                if (clients.find(hb->id) != clients.end()) {
                    clients[hb->id].lastSeen = now;
                }
            }
            else if (recvd == sizeof(HelicopterStatePOD)) {
                // STATE PACKET
                HelicopterStatePOD* incoming = (HelicopterStatePOD*)buffer;

                bool isNew = (clients.find(incoming->id) == clients.end());
                if (isNew) {
                    char ipStr[32]{};
                    inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
                    std::cout << "[SERVER] New client connected! ID="
                        << incoming->id
                        << "  IP=" << ipStr
                        << "  Port=" << ntohs(clientAddr.sin_port)
                        << "\n";
                    std::cout << "Current clients: " << (clients.size() + 1) << "\n";
                }

                clients[incoming->id].addr = clientAddr;
                clients[incoming->id].state = *incoming;
                clients[incoming->id].lastSeen = now;
            }
        }

        // 2) PERIODIC SNAPSHOT BROADCAST
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastBroadcast).count() >= BROADCAST_INTERVAL_MS) {
            lastBroadcast = now;

            if (!clients.empty()) {
                // BUILD SNAPSHOT
                SnapshotPacket snapshot{};
                snapshot.count = (int)clients.size();
                int i = 0;
                for (auto& kv : clients) {
                    snapshot.states[i++] = kv.second.state;
                }

                // BROADCAST SNAPSHOT TO ALL CLIENTS
                for (auto& kv : clients) {
                    sendto(
                        sock,
                        (char*)&snapshot,
                        sizeof(int) + snapshot.count * sizeof(HelicopterStatePOD),
                        0,
                        (sockaddr*)&kv.second.addr,
                        sizeof(kv.second.addr)
                    );
                }
            }
        }

        // 3) CHECK FOR TIMEOUTS (every 1 second)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCheck).count() > 1000) {
            lastCheck = now;

            std::vector<int> toRemove;
            for (auto& kv : clients) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - kv.second.lastSeen
                ).count();

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
            }
        }

        Sleep(1); // Small sleep to prevent 100% CPU usage
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}