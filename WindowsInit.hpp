#pragma once

// Windows
#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#undef TEXT
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <iostream>
#include <string>


using namespace std;


struct WindowsInit
{

    static void init() {
        #ifdef _WIN32
        // INIT WINSOCK
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (result != 0) throw runtime_error("WSAStartup failed: " + to_string(result));
        #endif
    }

    static void cleanup() {
        #ifdef _WIN32
        WSACleanup();
        #endif
    }
};


