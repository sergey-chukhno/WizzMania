#pragma once

#include <cstdint>

// Platform Specifics for Sockets
#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET SocketType;
#define INVALID_SOCKET_VAL INVALID_SOCKET

// Helper to close socket widely
inline void close_socket_raw(SocketType s) { closesocket(s); }
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int SocketType;
#define INVALID_SOCKET_VAL -1

// Helper to close socket widely
inline void close_socket_raw(SocketType s) { close(s); }
#endif
