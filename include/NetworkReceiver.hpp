// =============================================================================
// NetworkReceiver.hpp
//
// Receives finger positions from the Python vision server over UDP.
//
// Protocol: every UDP packet is a plain-text message of two numbers separated
// by a space, e.g. "0.5123 0.4410" -> normalized (x, y) of the index fingertip.
//
// UDP is used instead of TCP because we only care about the NEWEST position:
// if a packet is lost or late, there is no point in waiting for it.
//
// This class is Windows-only (it uses Winsock).
// =============================================================================
#pragma once

#include "Types.hpp"   // Point2D
#include <optional>    // std::optional (C++17)
#include <cstdint>     // uintptr_t

class NetworkReceiver {
public:
    // `explicit` prevents implicit conversions such as
    // `NetworkReceiver r = 5005;` - you must write `NetworkReceiver r(5005);`.
    explicit NetworkReceiver(int port = 5005);

    // The destructor closes the socket and shuts down Winsock.
    // This is RAII ("Resource Acquisition Is Initialization"): the object's
    // lifetime controls the resource's lifetime, so cleanup happens
    // automatically when the object goes out of scope.
    //
    // NOTE: the class does not delete its copy constructor / copy assignment.
    // If a NetworkReceiver were ever copied, both copies would close the same
    // socket in their destructors (a double-close bug). See the "rule of
    // three/five" - fixing this is a good modern C++ exercise.
    ~NetworkReceiver();

    // Start Winsock, create a non-blocking UDP socket and bind it to m_port.
    // Returns false (and prints an error) on failure.
    // Must be called once before receiveLatestPoint().
    bool initialize();

    // Read ALL packets waiting in the socket and return only the newest valid
    // point. Returns std::nullopt if no new packet arrived since the last call.
    //
    // std::optional<T> is a value that is either "a T" or "nothing". It is a
    // type-safe replacement for magic values (like -1) or out-parameters.
    std::optional<Point2D> receiveLatestPoint();

private:
    int m_port;

    // The Winsock SOCKET handle, stored as uintptr_t.
    // SOCKET is defined in <winsock2.h>. Storing it as an integer type of the
    // same size lets this header avoid including <winsock2.h>, which is heavy
    // and can conflict with raylib's names (e.g. DrawText, Rectangle).
    uintptr_t m_socket;

    // True once WSAStartup() succeeded, so the destructor knows it must call
    // WSACleanup().
    bool m_initialized{false};
};
