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
#define PAYLOAD_OFFSET 1
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

std::string trim(const std::string &s) {
    std::string::const_iterator it = s.begin();
    while (it != s.end() && isspace(*it))
        it++;

    std::string::const_reverse_iterator rit = s.rbegin();
    while (rit.base() != it && isspace(*rit))
        rit++;

    return std::string(it, rit.base());
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string join(const std::vector<std::string>& vec, const char* delim) {
    std::stringstream res;
    copy(vec.begin(), vec.end(), std::ostream_iterator<std::string>(res, delim));
    return res.str().substr(0, res.str().size() - 2);
}

std::string buildPayload(const std::vector<std::string>& req) {
    std::string payload = trim(req[PAYLOAD_OFFSET]);
    for (int i = PAYLOAD_OFFSET + 1; i < req.size(); ++i) {
        payload += ' ' + trim(req[i]);
    }
    return trim(payload);
}

// format: <type> <status> [payload]
// type: JOIN, LEAVE, LIST, SHUTDOWN, SEND, CHAT
// status: ERR, OK
void sendResponse(int socketId, std::string type, std::string status, std::string payload) {
    std::string object = type + ' ' + status;
    if (payload.size() > 0) {
        object += ' ' + trim(payload);
    }
    send(socketId, object.c_str(), BUFFER_SIZE, 0);
}

void doReceiveCommand(int index) {
    struct client* Client = &clients[index];
    int clientSocket = Client->socketId;

    std::cout << "[Log] New connection from client; id = " << index+1 << std::endl;

    while (true) {
        char buffer[BUFFER_SIZE] = {0};
        int read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        buffer[read] = '\0';

        std::vector<std::string> tokens = split(std::string(buffer), ' ');
        if (tokens.size() == 0) continue;

        std::cout << "[Log] PROTOCOL; " << buffer << std::endl;

        if (tokens[0].compare("JOIN") == 0) {
            // store username and broadcast message
            if (tokens.size() < 2) {
                sendResponse(clientSocket, "JOIN", "ERR", "Username must be exist");
                continue;
            }

            std::string username = buildPayload(tokens);
            if (trim(username).size() == 0) {
                sendResponse(clientSocket, "JOIN", "ERR", "Username can't be empty");
                continue;
            }

            clients[index].username = username;
            clients[index].isOnline = true;

            std::string message = username;
            message += " has joined the chat";

            for (int i = 0; i < clientCount; ++i) {
                if (clients[i].isOnline) {
                    send(clients[i].socketId, message.c_str(), BUFFER_SIZE, 0);
                }
            }

            std::cout << "[Log] " << message << std::endl;
            sendResponse(clientSocket, "JOIN", "OK", "");
        }

        if (tokens[0].compare("LEAVE") == 0) {
            clients[index].isOnline = false;

            std::string message = clients[index].username;
            message += " has leaved the chat";
            send(clients[index].socketId, message.c_str(), BUFFER_SIZE, 0);

            for (int i = 0; i < clientCount; ++i) {
                if (clients[i].isOnline) {
                    send(clients[i].socketId, message.c_str(), BUFFER_SIZE, 0);
                }
            }

            std::cout << "[Log] " << message << std::endl;
            sendResponse(clientSocket, "LEAVE", "OK", "");
            close(clientSocket);
        }

        if (tokens[0].compare("LIST") == 0) {
            std::vector<std::string> onlines;
            for (int i = 0; i < clientCount; ++i) {
                if (clients[i].isOnline)
                    onlines.push_back(clients[i].username);
            }
            std::string message = "Online ";
            message += "(" + std::to_string(onlines.size()) + "): " + join(onlines, ", ");

            sendResponse(clientSocket, "LIST", "OK", message.c_str());
        }

        if (tokens[0].compare("SEND") == 0) {
            // receive a message and broadcast to others
            if (tokens.size() < 2) {
                sendResponse(clientSocket, "SEND", "ERR", "Message must be exist");
                continue;
            }

            std::string message = buildPayload(tokens);
            if (trim(message).size() == 0) {
                sendResponse(clientSocket, "SEND", "ERR", "Message can't be empty");
                continue;
            }

            message = clients[index].username + ": " + message;

            for (int i = 0; i < clientCount; ++i) {
                if (index != i && clients[i].isOnline) {
                    sendResponse(clients[i].socketId, "CHAT", "OK", message.c_str());
                }
            }

            sendResponse(clientSocket, "SEND", "OK", "");
        }

        if (tokens[0].compare("SHUTDOWN") == 0) {
            std::cout << "[Log] Server is shutting down..." << std::endl;
            for (int i = 0; i < clientCount; ++i) {
                sendResponse(clientSocket, "CHAT", "OK", "Server is shutting down...");
                close(clients[i].socketId);
            }
            sendResponse(clientSocket, "SHUTDOWN", "OK", "");
            // TODO: handle thread, clean up resource
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