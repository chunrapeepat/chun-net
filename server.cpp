#include <iostream>
#include <vector>
#include <thread>
#include <sstream>
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
    bool isOnline;
    std::string username;
    struct sockaddr_in clientAddr;
};

int clientCount = 0;

struct client clients[BUFFER_SIZE];
std::thread threads[BUFFER_SIZE];

std::string join(const std::vector<std::string>& vec, const char* delim) {
    std::stringstream res;
    copy(vec.begin(), vec.end(), std::ostream_iterator<std::string>(res, delim));
    return res.str().substr(0, res.str().size() - 2);
}

void doReceiveCommand(int index) {
    struct client* Client = &clients[index];
    int clientSocket = Client->socketId;

    std::cout << "[Log] New connection from client; id = " << index+1 << std::endl;

    while (true) {
        char buffer[BUFFER_SIZE] = {0};
        int read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        buffer[read] = '\0';

        if (strcmp(buffer, "JOIN") == 0) {
            // store username and broadcast message
            read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            buffer[read] = '\0';

            clients[index].username = std::string(buffer);
            clients[index].isOnline = true;

            std::string message = std::string(buffer);
            message += " has joined the chat";

            for (int i = 0; i < clientCount; ++i) {
                if (clients[i].isOnline) {
                    send(clients[i].socketId, message.c_str(), BUFFER_SIZE, 0);
                }
            }

            std::cout << "[Log] " << message << std::endl;
        }

        if (strcmp(buffer, "LEAVE") == 0) {
            clients[index].isOnline = false;

            std::string message = clients[index].username;
            message += " has leaved the chat";
            send(clients[index].socketId, message.c_str(), BUFFER_SIZE, 0);

            for (int i = 0; i < clientCount; ++i) {
                if (clients[i].isOnline) {
                    send(clients[i].socketId, message.c_str(), BUFFER_SIZE, 0);
                }
            }

            close(clients[index].socketId);
            std::cout << "[Log] " << message << std::endl;
        }

        if (strcmp(buffer, "LIST") == 0) {
            std::vector<std::string> onlines;
            for (int i = 0; i < clientCount; ++i) {
                if (clients[i].isOnline)
                    onlines.push_back(clients[i].username);
            }
            std::string message = "Online ";
            message += "(" + std::to_string(onlines.size()) + "): " + join(onlines, ", ");
            send(clients[index].socketId, message.c_str(), BUFFER_SIZE, 0);
        }

        if (strcmp(buffer, "SEND") == 0) {
            // receive a message and broadcast to others
            read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            buffer[read] = '\0';

            std::string message = clients[index].username + ": " + std::string(buffer);

            for (int i = 0; i < clientCount; ++i) {
                if (index != i && clients[i].isOnline) {
                    send(clients[i].socketId, message.c_str(), BUFFER_SIZE, 0);
                }
            }
        }

        if (strcmp(buffer, "SHUTDOWN") == 0) {
            std::cout << "[Log] Server is shutting down..." << std::endl;
            for (int i = 0; i < clientCount; ++i) {
                send(clients[i].socketId, "Server is shutting down...", BUFFER_SIZE, 0);
                close(clients[i].socketId);
            }
            exit(0);
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
        std::cerr << "[Error] setsockopt reuseaddr" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt))) {
        std::cerr << "[Error] setsockopt nosigpipe" << std::endl;
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