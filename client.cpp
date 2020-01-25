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
#define PAYLOAD_OFFSET 2

bool isLogin = false;

struct response {
    std::string type;
    std::string payload;
    bool ERROR;
    bool OK;
};

std::string trim(const std::string &s) {
    std::string::const_iterator it = s.begin();
    while (it != s.end() && isspace(*it))
        it++;

    std::string::const_reverse_iterator rit = s.rbegin();
    while (rit.base() != it && isspace(*rit))
        rit++;

    return std::string(it, rit.base());
}

std::string buildPayload(const std::vector<std::string>& req) {
    std::string payload = trim(req[PAYLOAD_OFFSET]);
    for (int i = PAYLOAD_OFFSET + 1; i < req.size(); ++i) {
        payload += ' ' + trim(req[i]);
    }
    return trim(payload);
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

struct response parseResponse(std::vector<std::string>& object) {
    struct response res;
    res.type = object[0];
    res.payload = buildPayload(object);
    if (object[1].compare("OK") == 0) {
        res.OK = true;
        res.ERROR = false;
    } else {
        res.OK = false;
        res.ERROR = true;
    }
    return res;
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

void doReceive(int clientSocket) {
    while (true) {
        char buffer[BUFFER_SIZE] = {0};
        int read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        buffer[read] = '\0';

        if (read == 0) {
            std::cerr << "[Error] Connection failed" << std::endl;
            exit(EXIT_FAILURE);
        }

        std::vector<std::string> tokens = split(std::string(buffer), ' ');
        struct response res = parseResponse(tokens);

        // handle response
        if (res.type.compare("CHAT") == 0) {
            std::cout << "[>] " << buildPayload(tokens) << std::endl;
        }
        if (res.type.compare("JOIN") == 0) {
            if (res.ERROR) {
                std::cout << "[ChunNet] Error: " << res.payload << std::endl;
                authenication(clientSocket);
            }
            if (res.OK) {
                isLogin = true;
            }
        }
        if (res.type.compare("LEAVE") == 0) {
            if (res.OK) {
                std::cout << "[ChunNet] Program exited, Good Bye :)" << std::endl;
                exit(0);
            }
        }
        if (res.type.compare("LIST") == 0) {
            if (res.OK) {
                std::cout << "[ChunNet] " << res.payload << std::endl;
            }
        }
        if (res.type.compare("SEND") == 0) {
            if (res.ERROR) {
                std::cout << "[ChunNet] Error: " << res.payload << std::endl;
            }
        }
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

    std::thread thReceive(doReceive, clientSocket);

    std::cout << "[ChunNet] Connection has been established!" << std::endl;
    displayWelcomeMessage();
    authenication(clientSocket);

    // sending command
    while (1) {
        std::string command; getline(std::cin, command);
        std::vector<std::string> tokens = split(command, ' ');

        if (tokens.size() == 0) continue;

        if (tokens[0].compare("send") == 0 && isLogin) {
            // send a message to the chat
            if (tokens.size() < 2) {
                std::cout << "[ChunNet] Usage: send <message>" << std::endl;
                continue;
            }
            std::string message = "";
            for (int i = 1; i < tokens.size(); ++i) message += tokens[i] + ' ';

            sendRequest(clientSocket, "SEND", message);
        } else if (tokens[0].compare("list") == 0 && isLogin) {
            // display online users
            sendRequest(clientSocket, "LIST", "");
        } else if (tokens[0].compare("shutdown") == 0 && isLogin) {
            // shutdown the server
            sendRequest(clientSocket, "SHUTDOWN", "");
        } else if (tokens[0].compare("exit") == 0 && isLogin) {
            // exit the program
            sendRequest(clientSocket, "LEAVE", "");
        } else if (isLogin) {
            std::cout << "[ChunNet] Unknown command " << tokens[0] << std::endl;
        }
    }

    return 0;
}