#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 8888
#define PROTOCOL 0

int main() {
    std::cout << "[Log] The server is running on port " << 8888 << std::endl;

    int sockfd, new_socket, valread;
    int opt = 1;
    char buffer[1024] = {0};
    char *hello = "Hello from server";

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, PROTOCOL)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8888
    if (bind(sockfd, (struct sockaddr*) &address, sizeof(address))) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(sockfd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(sockfd, (struct sockaddr*) &address, (socklen_t*) &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    valread = read(new_socket, buffer, 1024);
    printf("%s\n",buffer);
    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent\n");

    return 0;
}