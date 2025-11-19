
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
#pragma pack(pop)

NetworkClient::NetworkClient() : sock(INVALID_SOCKET), myId(0) {}
NetworkClient::~NetworkClient() { if (sock != INVALID_SOCKET) closesocket(sock); WSACleanup(); }

bool NetworkClient::init(const char* serverIp, int port, int clientId) {
    myId = clientId;

    WSADATA w;
    WSAStartup(MAKEWORD(2, 2), &w);

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);

    return true;
}

void NetworkClient::sendState(const HelicopterState& s) {
    HelicopterStatePOD pod;
    pod.id = s.id;
    pod.px = s.pos.x;
    pod.py = s.pos.y;
    pod.pz = s.pos.z;
    pod.rx = s.rot.x;
    pod.ry = s.rot.y;
    pod.rz = s.rot.z;

    sendto(
        sock,
        (char*)&pod,
        sizeof(pod),
        0,
        (sockaddr*)&serverAddr,
        sizeof(serverAddr)
    );
}

bool NetworkClient::receiveSnapshot(std::vector<HelicopterState>& out) {
    out.clear();

    SnapshotPacket snap{};
    sockaddr_in from{};
    int fromLen = sizeof(from);

    int recvd = recv(
        sock,
        (char*)&snap,
        sizeof(snap),
        0
        //(sockaddr*)&from,
        //&fromLen
    );

    if (recvd < 4) return false;

    for (int i = 0; i < snap.count; i++) {
        HelicopterState hs;
        hs.id = snap.states[i].id;
        hs.pos = { snap.states[i].px, snap.states[i].py, snap.states[i].pz };
        hs.rot = { snap.states[i].rx, snap.states[i].ry, snap.states[i].rz };
        out.push_back(hs);
        printf("%d, %f, %f, %f\n", i, hs.pos.x, hs.pos.y, hs.pos.z);
        printf("%d, %f, %f, %f\n", i, hs.rot.x, hs.rot.y, hs.rot.z);
    }

    return true;
}
