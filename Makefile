.PHONY: default
default: help

all: client server
	@echo "* Done"

client:
	@echo "* Building Client"
	g++ client.cpp -std=c++11 -o client

server:
	@echo "* Building Server"
	g++ server.cpp -std=c++11 -o server

clean:
	@rm  client server 1>/dev/null 2>&1
	@echo "* Done"

help:
	@echo "ChunNet: A chat application is written in C++ using socket programming"
	@echo ""
	@echo "* server: Build the server"
	@echo "* client: Build the client"
	@echo "* all:    Build the server and client"
	@echo "* clean:  Remove project binaries"
	@echo ""