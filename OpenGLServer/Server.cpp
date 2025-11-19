// server_udp.cpp

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <vector>

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
#pragma pack(pop)

struct ClientInfo {
    sockaddr_in addr;
    HelicopterStatePOD state;
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

    std::cout << "UDP server running on port 8008\n";

    std::unordered_map<int, ClientInfo> clients;

    while (true) {
        // 1) RECEIVE STATE PACKET
        HelicopterStatePOD incoming{};
        sockaddr_in clientAddr{};
        int addrLen = sizeof(clientAddr);

        int recvd = recvfrom(
            sock,
            (char*)&incoming,
            sizeof(incoming),
            0,
            (sockaddr*)&clientAddr,
            &addrLen
        );

        if (recvd > 0) {

            // -------- NEW: Detect NEW client connection --------
            bool isNew = (clients.find(incoming.id) == clients.end());
            if (isNew) {
                char ipStr[32]{};
                inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));

                std::cout << "[SERVER] New client connected! ID="
                    << incoming.id
                    << "  IP=" << ipStr
                    << "  Port=" << ntohs(clientAddr.sin_port)
                    << "\n";

                std::cout << "Current clients: " << (clients.size() + 1) << "\n";
            }
            // ---------------------------------------------------

            clients[incoming.id].addr = clientAddr;
            clients[incoming.id].state = incoming;


            // 2) BUILD SNAPSHOT
            SnapshotPacket snapshot{};
            snapshot.count = (int)clients.size();

            int i = 0;
            for (auto& kv : clients) {
                snapshot.states[i++] = kv.second.state;
            }

            // 3) BROADCAST SNAPSHOT TO ALL CLIENTS
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

    closesocket(sock);
    WSACleanup();
    return 0;
}
