#pragma once

#include <fcntl.h>
#include <sys/select.h>

// Windows
#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#undef TEXT
#include <winsock2.h>
#include <ws2tcpip.h>

// Linux
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#ifndef SOCKET
typedef int SOCKET;
#endif
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#include <iostream>
#include <string>
#include "../Logger.hpp"

using namespace std;

struct TCPSocket {
    SOCKET socket_num;
    uint32_t timeout_ms = 5000;

    TCPSocket(uint32_t timeout_ms = 5000) {
        this->timeout_ms = timeout_ms;

        // Create a socket
        this->socket_num = socket(AF_INET, SOCK_STREAM, 0);
        if (this->socket_num == INVALID_SOCKET) {
            throw runtime_error("Failed trying to create socket");
        }

        // set timeout
        #ifdef _WIN32
        setsockopt(this->socket_num, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
        #else
        // POSIX-specific socket setup (Linux, macOS)
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(this->socket_num, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        #endif
    }

    ~TCPSocket() {
        #ifdef _WIN32
        closesocket(this->socket_num);
        #else
        close(this->socket_num);
        #endif
    }

    void server_init(int port) {

        // Prepare the sockaddr_in structure
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(port);
        
        // Bind
        // Retry binding until it succeeds
        while (true) {
            Logger::info("Port " + std::to_string(port) + " in use or being cleaned up. Retrying...");
            if (::bind(this->socket_num, (struct sockaddr *)&server, sizeof(server)) == 0) {
                break; // Bind succeeded
            }
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait for 1 second before retrying
        }

        // Listen to incoming connections
        if (::listen(this->socket_num, SOMAXCONN) < 0) {
            throw std::runtime_error("Listen failed");
        }

        // Set socket to non-blocking
        int flags = fcntl(this->socket_num, F_GETFL, 0);
        fcntl(this->socket_num, F_SETFL, flags | O_NONBLOCK);
    }

    SOCKET acceptConnection(sockaddr_in &client_addr) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(this->socket_num, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 5;  // 5 seconds timeout
        timeout.tv_usec = 0;

        int activity = select(this->socket_num + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0 && errno != EINTR) return -1;

        if (activity == 0) return -1;

        socklen_t client_len = sizeof(client_addr);
        SOCKET client_socket = -1;
        if (FD_ISSET(this->socket_num, &readfds)) {
            client_socket = accept(this->socket_num, (struct sockaddr *)&client_addr, &client_len);
        }

        if (client_socket == INVALID_SOCKET) {
            throw std::runtime_error("Failed to accept client connection");
        }

        return client_socket;
    }

    void connect(const char* serverIP, int port) {
        

        // set ip address
        #ifdef _WIN32
        struct sockaddr_in server;
        int slen2 = sizeof(server);
        ZeroMemory(&server, slen2);
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        #else
        struct sockaddr_in server;
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        #endif
        
        if (inet_pton(AF_INET, serverIP, &server.sin_addr) <= 0) {
            throw runtime_error("Invalid address/ Address not supported.");
        }

        // Connect to remote server
        if (::connect(this->socket_num, (struct sockaddr *)&server, sizeof(server)) != 0) {
            throw runtime_error("Connect error");
        }

    }

    int send(SOCKET client_socket, const char *message, int message_length) {
        #ifdef _WIN32
        u_long mode = 0; // 0 for blocking mode
        ioctlsocket(client_socket, FIONBIO, &mode);
        #else
        int flags = fcntl(client_socket, F_GETFL, 0);
        if (flags & O_NONBLOCK) {
            fcntl(client_socket, F_SETFL, flags & ~O_NONBLOCK);
        }
        #endif
        
        int bytes_sent = ::send(client_socket, message, message_length, 0);
        if (bytes_sent == SOCKET_ERROR) {
            throw runtime_error("Failed to send message");
        }
        return bytes_sent;
    }

    int receive(SOCKET client_socket, char *buffer, int buffer_length) {
        // Ensure the socket is in blocking mode
        #if _WIN32
        u_long mode = 0; // 0 for blocking mode
        if (ioctlsocket(client_socket, FIONBIO, &mode) != 0) {
            throw std::runtime_error("Failed to set socket to blocking mode");
        }
        #else
        int flags = fcntl(client_socket, F_GETFL, 0);
        if (fcntl(client_socket, F_SETFL, flags & ~O_NONBLOCK) != 0) {
            throw std::runtime_error("Failed to set socket to blocking mode");
        }
        #endif

        // Set the receive timeout
        struct timeval timeout;
        timeout.tv_sec = this->timeout_ms / 1000.0;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

        
        int bytes_received = ::recv(client_socket, buffer, buffer_length, 0);
        return bytes_received;
    }

    void close_connection(SOCKET client_socket) {
        #ifdef _WIN32
        closesocket(client_socket);
        #else
        close(client_socket);
        #endif
    }
};
