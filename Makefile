# simple-chatroom Makefile for Linux (g++/clang++)
# Windows 用户建议直接用 CMake 或 VS 开发者命令行：
#   cl /std:c++17 /EHsc server.cpp ws2_32.lib /Fe:chat-server.exe
#   cl /std:c++17 /EHsc client.cpp ws2_32.lib /Fe:chat-client.exe

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
LDFLAGS  ?= -pthread

all: chat-server chat-client

chat-server: server.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

chat-client: client.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f chat-server chat-client

.PHONY: all clean