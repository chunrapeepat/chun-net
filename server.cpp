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
#define MAX_REQUEST_AT_TIME 10

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

    int new_socket;
    while (1) {
        if ((new_socket = accept(sockfd, (struct sockaddr*) &address, (socklen_t*) &addrlen)) < 0) {
            std::cerr << "[Error] Error accepting request from client" << std::endl;
            exit(EXIT_FAILURE);
        }
        char buffer[1024] = {0};
        char *hello = "Hello from server";

        recv(new_socket, buffer, 1024, 0);
        printf("%s\n", buffer);
        send(new_socket, hello, strlen(hello), 0);
        printf("Hello message sent\n");
    }

    return 0;
}