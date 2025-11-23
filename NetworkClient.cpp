// NetworkClient.cpp
#include "NetworkClient.h"
#include <iostream>

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
    char type; // 'H'
};
#pragma pack(pop)

NetworkClient::NetworkClient()
    : sock(INVALID_SOCKET)
    , myId(0)
    , lastHeartbeat(std::chrono::steady_clock::now())
    , lastStateUpdate(std::chrono::steady_clock::now())
{
}

NetworkClient::~NetworkClient() {
    if (sock != INVALID_SOCKET) closesocket(sock);
    WSACleanup();
}

bool NetworkClient::init(const char* serverIp, int port, int clientId) {
    myId = clientId;

    WSADATA w;
    if (WSAStartup(MAKEWORD(2, 2), &w) != 0) {
        std::cerr << "[CLIENT] WSAStartup failed\n";
        return false;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[CLIENT] Socket creation failed\n";
        return false;
    }

    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    int recvBufSize = 1024 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&recvBufSize, sizeof(recvBufSize));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);

    std::cout << "[CLIENT] Initialized with ID " << clientId << "\n";
    return true;
}

void NetworkClient::sendState(const HelicopterState& s) {
    HelicopterStatePOD pod{};
    pod.id = s.id;
    pod.px = s.pos.x;
    pod.py = s.pos.y;
    pod.pz = s.pos.z;
    pod.rx = s.rot.x;
    pod.ry = s.rot.y;
    pod.rz = s.rot.z;

    int sent = sendto(sock, (char*)&pod, sizeof(pod), 0,
        (sockaddr*)&serverAddr, sizeof(serverAddr));

    if (sent == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            std::cerr << "[CLIENT] Send state failed: " << err << "\n";
        }
    }
    else {
        lastStateUpdate = std::chrono::steady_clock::now();
    }
}

void NetworkClient::sendHeartbeat() {
    HeartbeatPacket hb{};
    hb.id = myId;
    hb.type = 'H';

    int sent = sendto(sock, (char*)&hb, sizeof(hb), 0,
        (sockaddr*)&serverAddr, sizeof(serverAddr));

    if (sent == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            std::cerr << "[CLIENT] Send heartbeat failed: " << err << "\n";
        }
    }
    else {
        lastHeartbeat = std::chrono::steady_clock::now();
    }
}

void NetworkClient::update() {
    auto now = std::chrono::steady_clock::now();
    auto timeSinceState = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastStateUpdate).count();
    auto timeSinceHB = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastHeartbeat).count();

    if (timeSinceState > 2000 && timeSinceHB > 2000) {
        sendHeartbeat();
    }
}

bool NetworkClient::receiveSnapshot(std::vector<HelicopterState>& out) {
    out.clear();

    SnapshotPacket latestSnap{};
    bool receivedAny = false;
    int packetsProcessed = 0;

    char buffer[2048];

    while (true) {
        sockaddr_in from{};
        int fromLen = sizeof(from);

        int recvd = recvfrom(sock, buffer, sizeof(buffer), 0,
            (sockaddr*)&from, &fromLen);

        if (recvd == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) break;
            std::cerr << "[CLIENT] Receive error: " << err << "\n";
            break;
        }

        if (recvd < 1) continue;

        // Check for SavedDataPacket (starts with 'S')
        if (recvd == sizeof(SavedDataPacket) && buffer[0] == 'S') {
            SavedDataPacket* saved = reinterpret_cast<SavedDataPacket*>(buffer);
            savedDataQueue.push(*saved);
            std::cout << "[CLIENT] Queued saved data packet for ID " << saved->id << "\n";
            continue;
        }

        // Validate minimum packet size for snapshot
        if (recvd < sizeof(int)) continue;

        SnapshotPacket* snap = reinterpret_cast<SnapshotPacket*>(buffer);

        // Validate count
        if (snap->count < 0 || snap->count > 32) {
            std::cerr << "[CLIENT] Invalid snapshot count: " << snap->count << "\n";
            continue;
        }

        // Validate received size
        int expectedSize = sizeof(int) + snap->count * sizeof(HelicopterStatePOD);
        if (recvd < expectedSize) {
            std::cerr << "[CLIENT] Incomplete snapshot received\n";
            continue;
        }

        latestSnap = *snap;
        receivedAny = true;
        packetsProcessed++;
    }

    if (packetsProcessed > 1) {
        static int skipCount = 0;
        skipCount++;
        if (skipCount % 100 == 0) {
            std::cout << "[CLIENT] Processed " << packetsProcessed
                << " packets (keeping latest). Total skips: " << skipCount << "\n";
        }
    }

    if (!receivedAny) return false;

    out.reserve(latestSnap.count);
    for (int i = 0; i < latestSnap.count; i++) {
        HelicopterState hs;
        hs.id = latestSnap.states[i].id;
        hs.pos = { latestSnap.states[i].px, latestSnap.states[i].py, latestSnap.states[i].pz };
        hs.rot = { latestSnap.states[i].rx, latestSnap.states[i].ry, latestSnap.states[i].rz };
        out.push_back(hs);
    }

    return true;
}

bool NetworkClient::receiveSavedData(SavedDataPacket& out) {
    if (savedDataQueue.empty()) return false;
    out = savedDataQueue.front();
    savedDataQueue.pop();
    return true;
}

int NetworkClient::getMyId() const {
    return myId;
}