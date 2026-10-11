// =============================================================================
// NetworkReceiver.cpp - Winsock UDP receiver for fingertip coordinates.
// See NetworkReceiver.hpp for the protocol description.
// =============================================================================

// These macros must be defined BEFORE <winsock2.h> (which pulls in
// <windows.h>) to trim what Windows declares:
//   WIN32_LEAN_AND_MEAN - skip rarely used Windows APIs (faster compile).
//   NOGDI               - skip the GDI graphics API.
//   NOUSER              - skip the USER (windowing) API.
// NOGDI/NOUSER matter here because Windows declares names such as Rectangle,
// CloseWindow, ShowCursor and DrawText, which collide with raylib's functions.
// This is also why only this .cpp file includes Winsock - never main.cpp.
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include "NetworkReceiver.hpp"
#include <winsock2.h>   // sockets API (socket, bind, recv, ...)
#include <ws2tcpip.h>   // extra TCP/IP helpers
#include <iostream>     // std::cerr for error messages
#include <sstream>      // std::istringstream to parse the text message

// MSVC-specific way to link ws2_32.lib from source code.
// NOTE: CMakeLists.txt already links ws2_32, so this line is redundant.
#pragma comment(lib, "ws2_32.lib")

// Start with an invalid socket. initialize() creates the real one.
NetworkReceiver::NetworkReceiver(int port)
    : m_port(port), m_socket(static_cast<uintptr_t>(INVALID_SOCKET)) {}

NetworkReceiver::~NetworkReceiver() {
    // Release resources in the REVERSE order of acquisition:
    // the socket first, then the Winsock library itself.
    SOCKET sock = static_cast<SOCKET>(m_socket);
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
    if (m_initialized) {
        WSACleanup();
    }
}

bool NetworkReceiver::initialize() {
    // ---- Step 1: start the Winsock library (version 2.2) ----
    // On Windows every program must call WSAStartup before using sockets,
    // and WSACleanup when done (handled by the destructor).
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }
    m_initialized = true;

    // ---- Step 2: create a UDP socket ----
    // AF_INET = IPv4, SOCK_DGRAM + IPPROTO_UDP = UDP datagrams.
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return false;
    }
    m_socket = static_cast<uintptr_t>(sock);

    // ---- Step 3: make it non-blocking ----
    // Normally recv() waits ("blocks") until a packet arrives, which would
    // freeze the game loop. In non-blocking mode recv() returns immediately
    // with an error (WSAEWOULDBLOCK) when there is nothing to read.
    // FIONBIO = "File I/O Non-Blocking I/O"; mode 1 turns it on.
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    // ---- Step 4: bind to the port so we receive packets sent to it ----
    // `sockaddr_in serverAddr{};` - the empty braces zero-initialize every
    // field, which these C structs require.
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    // htons = "host to network short": converts the port number to the
    // big-endian byte order used on the network.
    serverAddr.sin_port = htons(static_cast<u_short>(m_port));
    // INADDR_ANY = accept packets sent to any of this machine's addresses
    // (including 127.0.0.1, which the Python server uses).
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // The C sockets API takes a generic `sockaddr*`, so we have to
    // reinterpret our IPv4-specific struct as one. reinterpret_cast is the
    // most dangerous C++ cast; it is acceptable here because this is exactly
    // how the API is designed to be used.
    if (bind(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        // Most common cause: another program (or another copy of the game)
        // is already using the port.
        std::cerr << "Socket bind failed\n";
        return false;
    }

    return true;
}

std::optional<Point2D> NetworkReceiver::receiveLatestPoint() {
    SOCKET sock = static_cast<SOCKET>(m_socket);
    if (!m_initialized || sock == INVALID_SOCKET) return std::nullopt;

    // A fixed-size buffer on the stack. The messages are tiny
    // (e.g. "0.5123 0.4410" is 13 bytes), so 128 bytes is plenty.
    char buffer[128];
    std::optional<Point2D> latestPoint = std::nullopt;

    // The camera may send several packets between two game frames.
    // Keep reading until the socket is empty, overwriting latestPoint each
    // time, so only the NEWEST position is returned and old ones are dropped.
    while (true) {
        // Read at most sizeof(buffer) - 1 bytes, leaving room for the '\0'.
        // Returns the number of bytes read, or SOCKET_ERROR (-1) when there is
        // nothing left (WSAEWOULDBLOCK) or on a real error.
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            break;
        }

        // Turn the raw bytes into a C string, then parse two floats from it.
        buffer[bytesReceived] = '\0';
        std::istringstream iss(buffer);
        float normX = 0.0f, normY = 0.0f;
        // `iss >> a >> b` evaluates to true only if BOTH numbers were parsed,
        // so malformed packets are silently ignored.
        // (Modern alternative: std::from_chars + std::string_view, which is
        // faster and does not allocate memory.)
        if (iss >> normX >> normY) {
            latestPoint = Point2D{normX, normY};
        }
    }

    return latestPoint;
}
