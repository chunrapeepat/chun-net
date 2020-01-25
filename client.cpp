#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8888
#define PROTOCOL 0
#define BUFFER_SIZE 1024

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// format: <type> [payload]
// type: JOIN <username>, LEAVE, SEND <message>, LIST, SHUTDOWN
void sendRequest(int clientSocket, std::string type, std::string payload) {
    std::string object = type;
    if (payload.size() > 0) {
        object += ' ' + payload;
    }
    send(clientSocket, object.c_str(), BUFFER_SIZE, 0);
}

void doReceive(int clientSocket) {
    while (true) {
        char buffer[BUFFER_SIZE] = {0};
        int read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        buffer[read] = '\0';

        if (read == 0) {
            std::cerr << "[Error] Connection failed" << std::endl;
            exit(EXIT_FAILURE);
        }

        std::cout << "[>] " << buffer << std::endl;
    }
}

void displayWelcomeMessage() {
    std::cout << "==================================================" << std::endl;
    std::cout << "    Welcome to ChunNet - The Chat Application" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "[ChunNet] Commands:" << std::endl;
    std::cout << "[ChunNet]     · send <message> - Send a message" << std::endl;
    std::cout << "[ChunNet]     · list - View currently online person" << std::endl;
    std::cout << "[ChunNet]     · shutdown - Shutdown the server" << std::endl;
    std::cout << "[ChunNet]     · exit - Exit the program" << std::endl;
    std::cout << std::endl;
}

void authenication(int clientSocket) {
    std::cout << "Enter your name: ";
    std::string username; getline(std::cin, username);

    sendRequest(clientSocket, "JOIN", username);
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

    std::thread thReceive(doReceive, clientSocket);

    std::cout << "[ChunNet] Connection has been established!" << std::endl;
    displayWelcomeMessage();
    authenication(clientSocket);

    // sending command
    while (1) {
        std::string command; getline(std::cin, command);
        std::vector<std::string> tokens = split(command, ' ');

        if (tokens.size() == 0) continue;

        if (tokens[0].compare("send") == 0) {
            // send a message to the chat
            if (tokens.size() < 2) {
                std::cout << "[ChunNet] Usage: send <message>" << std::endl;
                continue;
            }
            std::string message = "";
            for (int i = 1; i < tokens.size(); ++i) message += tokens[i] + ' ';

            sendRequest(clientSocket, "SEND", message);
        } else if (tokens[0].compare("list") == 0) {
            // display online users
            sendRequest(clientSocket, "LIST", "");
        } else if (tokens[0].compare("shutdown") == 0) {
            // shutdown the server
            sendRequest(clientSocket, "SHUTDOWN", "");
        } else if (tokens[0].compare("exit") == 0) {
            // exit the program
            sendRequest(clientSocket, "LEAVE", "");
            std::cout << "[ChunNet] Exiting program..., Good Bye :)" << std::endl;
            exit(0);
        } else {
            std::cout << "[ChunNet] Unknown command " << tokens[0] << std::endl;
        }
    }

    return 0;
}