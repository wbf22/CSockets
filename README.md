# CSockets
A C++ sockets api with support for Windows, Mac, and LInux. Allows for both TCP and UDP connections


# Usage

These files are used to set up sockets for both TCP and UDP connections, and both client and server connections.
It might be easiest to just copy the files into your project and use them as is, as they were made to be contained to a 
single file.

To make either a client or a server you first have to initialize sockets if on windows. (weird)
So first should use the `WindowsInit.hpp` class to intialize that and then call the cleanup method when
you're done using the sockets


Here's an example using our UDP client and server

```c++
#include 'UDPSocket.hpp'
#include 'WindowsInit.hpp'
#include <string>
#include <iostream>

using namespace std;

string SHRUG = "¯\\_(ツ)_/¯";

void thread_func() {

    UDPSocket client = UDPSocket(1000, 8081);
    client.send(SHRUG, "127.0.0.1", 8080);

}


int main() {

    WindowsInit::init();


    // server
    UDPSocket server = UDPSocket(1000, 8080);

    // start client    
    thread other_thread(thread_func);

    // recieve message
    char buffer[512] = {0};
    int bytes = 0;
    sockaddr_in client = server.recieve(buffer, 512, bytes);

    // null terminate
    buffer[bytes] = '\0';

    cout << buffer << endl;
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(client.sin_family, &client.sin_addr, ipStr, sizeof(ipStr));
    cout << "IP Address: " << ipStr << endl;
    cout << "Port: " << ntohs(client.sin_port) << endl; // Convert network byte order to host byte order

    // clean up
    other_thread.join();
    WindowsInit::cleanup();

}
```


And here's an example of using our TCP client and server

```c++

#include 'TCPSocket.hpp'
#include 'WindowsInit.hpp'
#include <string>
#include <iostream>

using namespace std;

string SHRUG = "¯\\_(ツ)_/¯";

void thread_func() {

    this_thread::sleep_for(chrono::seconds(2));

    TCPSocket client = TCPSocket(10000);

    try {

        client.connect("127.0.0.1", 8080);
        
        const char* msg = SHRUG;
        client.send(client.socket_num, msg, strlen(msg));

        char buffer[1024];
        int bytesReceived = client.receive(client.socket_num, buffer, sizeof(buffer)); // Receive data
        cout << "Received: " << string(buffer, bytesReceived) << endl;
    } catch (const runtime_error& e) {
        cerr << "Exception: " << e.what() << endl;
    }

}


int main() {

    WindowsInit::init();

    // server
    TCPSocket server = TCPSocket(10000);
    server.server_int(8080);

    // start client    
    thread other_thread(thread_func);

    try {

        // listen for connections
        sockaddr_in client;
        SOCKET clientSocket = server.acceptConnection(client); // Accept a connection

        // recieve message
        char buffer[1024];
        int bytesReceived = server.receive(clientSocket, buffer, sizeof(buffer)); // Receive data
        cout << "Received: " << string(buffer, bytesReceived) << endl;
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(client.sin_family, &client.sin_addr, ipStr, sizeof(ipStr));
        cout << "IP Address: " << ipStr << endl;
        cout << "Port: " << ntohs(client.sin_port) << endl; // Convert network byte order to host byte order

        // send message
        const char* msg = SHRUG;
        server.send(clientSocket, msg, strlen(msg)); // Send data
    } catch (const runtime_error& e) {
        cerr << "Exception: " << e.what() << endl;
    }

    // clean up
    other_thread.join();
    WindowsInit::cleanup();

}

```




