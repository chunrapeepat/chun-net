#include <iostream>
#include <vector>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 8888
#define PROTOCOL 0
#define BUFFER_SIZE 1024
#define MAX_REQUEST_AT_TIME 10

struct client {
    int index;
    int socketId;
    int addrLen;
    struct sockaddr_in clientAddr;
};

int isExit = 0;
int clientCount = 0;

struct client clients[BUFFER_SIZE];
std::thread threads[BUFFER_SIZE];
std::thread thShutdown;

void doHandleShutdown() {
    while (!isExit);
    for (int i = 0; i < clientCount; ++i) {
        threads[i].join();
    }
    thShutdown.join();
    std::cout << "[Log] Server is shutting down..." << std::endl;
    exit(-1);
}

void doReceiveCommand(int index) {
    struct client* Client = &clients[index];
    int clientSocket = Client->socketId;

    std::cout << "[Log] New connection from client id = " << index+1 << std::endl;

    while (true) {
        char buffer[BUFFER_SIZE] = {0};
        int read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        buffer[read] = '\0';

        if (strcmp(buffer, "SEND") == 0) {
            // receive a message and broadcast to others
            read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            buffer[read] = '\0';

            for (int i = 0; i < clientCount; ++i) {
                if (i != index)
                    send(clients[i].socketId, buffer, BUFFER_SIZE, 0);
            }
        }
    }
}

int main() {
    std::cout << "[Log] The ChunNet server is running on port " << PORT << std::endl;

    const int sockfd = socket(AF_INET, SOCK_STREAM, PROTOCOL);
    if (sockfd == 0) {
        std::cerr << "[Error] Socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    const int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "[Error] setsockopt" << std::endl;
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*) &address, sizeof(address))) {
        std::cerr << "[Error] Error binding socket" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (listen(sockfd, MAX_REQUEST_AT_TIME) < 0) {
        std::cerr << "[Error] Error listening" << std::endl;
        exit(EXIT_FAILURE);
    }

    thShutdown = std::thread(doHandleShutdown);

    while (true) {
        clients[clientCount].socketId = accept(sockfd, (struct sockaddr*) &clients[clientCount].clientAddr, (socklen_t*) &clients[clientCount].addrLen);
        clients[clientCount].index = clientCount;

        if (clients[clientCount].socketId < 0) {
            std::cerr << "[Error] Error accepting request from client" << std::endl;
            exit(EXIT_FAILURE);
        }

        threads[clientCount] = std::thread(doReceiveCommand, clientCount);
        ++clientCount;
    }

    return 0;
}