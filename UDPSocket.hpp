#pragma once

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


using namespace std;


struct UDPSocket {

    SOCKET socket_num;
    uint32_t timeout_ms;

    
    UDPSocket(uint32_t timeout_ms, int32_t port=8080) {
        
        #ifdef _WIN32
        // SET UP SOCKET
        struct addrinfo hints, *res;

        // set up address
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET; // Use IPv4
        hints.ai_socktype = SOCK_DGRAM; // UDP socket
        hints.ai_flags = AI_PASSIVE; // Use my IP

        // Get address info
        if (getaddrinfo(NULL, to_string(port).c_str(), &hints, &res) != 0) {
            throw runtime_error("Failed calling getaddrinfo trying to setup socket");
        }

        // Create a socket
        this->socket_num = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (this->socket_num == INVALID_SOCKET) {
            throw runtime_error("Failed trying to create socket");
        }

        // Bind socket
        if (bind(this->socket_num, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
            throw runtime_error("Failed trying to bind socket");
        }

        freeaddrinfo(res);
        
        // set timeout
        setsockopt(this->socket_num, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout_ms, sizeof(timeout_ms));
        #else
        // POSIX-specific socket setup (Linux, macOS)
        this->socket_num = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (this->socket_num == INVALID_SOCKET) {
            throw runtime_error("Failed trying to create socket");
        }
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (::bind(this->socket_num, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
            throw runtime_error("Failed trying to bind socket");
        }

        // set timeout
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(this->socket_num, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        #endif
    }

    ~UDPSocket() {
        #ifdef _WIN32
        closesocket(this->socket_num);
        #else
        close(this->socket_num);
        #endif
    }


    /**
     * Blocks until it recieves a message. 
     * 
     * Returns gathered client info and stores message in buffer
     */
    sockaddr_in recieve(char *buffer, int read_size, int& bytes_read) {
            
        // make an empty client address
        struct sockaddr_in si_client;
        socklen_t slen = sizeof(si_client);

        // recieve message
        bytes_read = recvfrom(this->socket_num, buffer, read_size, 0, (struct sockaddr *) &si_client, &slen);

        return si_client;
    };
    

    /**
     * Sends a message to a host
     */
    int send(const char *message, const int length, const string& host, int port) {

        #ifdef _WIN32

        // set up address
        struct sockaddr_in si_server;
        int slen2 = sizeof(si_server);
        ZeroMemory(&si_server, slen2);
        si_server.sin_family = AF_INET;
        si_server.sin_port = htons(port);

        // set binary IP address in the address object
        inet_pton(AF_INET, host.c_str(), &si_server.sin_addr);

        // send message
        int bytes_sent = sendto(this->socket_num, message, length, 0, (struct sockaddr *) &si_server, slen2);
        if (bytes_sent == SOCKET_ERROR) throw runtime_error("Bytes failed to send");
        
        return bytes_sent;

        #else

        // set up address
        struct sockaddr_in si_server;
        socklen_t slen2 = sizeof(si_server);
        memset(&si_server, 0, slen2); // Use memset instead of ZeroMemory
        si_server.sin_family = AF_INET;
        si_server.sin_port = htons(port);

        // set binary IP address in the address object
        if (inet_pton(AF_INET, host.c_str(), &si_server.sin_addr) <= 0) {
            throw std::runtime_error("Invalid address/ Address not supported");
        }

        // send message
        int bytes_sent = sendto(this->socket_num, message, length, 0, (struct sockaddr *) &si_server, slen2);
        if (bytes_sent == -1) {
            int err = errno;
            std::cerr << "Bytes failed to send, error code: " << err << " (" << strerror(err) << ")" << std::endl;
            throw std::runtime_error("Bytes failed to send");
        }

        return bytes_sent;

        #endif
    };

};

