#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define PORT 8888
#define PROTOCOL 0
#define BUFFER_SIZE 1024

void doReceive(int clientSocket) {
    while (true) {
        char buffer[1024] = {0};
        int read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        buffer[read] = '\0';

        std::cout << "[>] " << buffer << std::endl;
    }
}

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, PROTOCOL);
    if (clientSocket < 0) {
        std::cerr << "[Error] Socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "[Error] Invalid address" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (connect(clientSocket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "[Error] Connection failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "[ChunNet] Connection established...." << std::endl;
    std::thread thReceive(doReceive, clientSocket);

    // sending command
    while (1) {
        char buffer[1024] = {0};
        scanf("%s", buffer); getchar();

        if (strcmp(buffer, "SEND") == 0) {
            send(clientSocket, buffer, BUFFER_SIZE, 0);

            std::string message; getline(std::cin, message);
            send(clientSocket, message.c_str(), BUFFER_SIZE, 0);
        } else {
            std::cout << "[ChunNet] Unknown command " << buffer << std::endl;
        }
    }

    return 0;
}