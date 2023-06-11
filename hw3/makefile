CXX = g++
CXXFLAGS = -std=c++14 -Wall -pedantic -pthread -lboost_system
CXX_INCLUDE_DIRS = /usr/local/include
CXX_INCLUDE_PARAMS = $(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS = /usr/local/lib
CXX_LIB_PARAMS = $(addprefix -L , $(CXX_LIB_DIRS))

SRC1 = http_server.cpp
SRC2 = console.cpp
SRC3 = cgi_server.cpp

TARGET1 = http_server
TARGET2 = console.cgi
TARGET3 = cgi_server.exe

all: $(TARGET1) $(TARGET2) $(TARGET3)

part1:
	$(CXX) $(CXXFLAGS) $(SRC1) -o $(TARGET1) $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)
	$(CXX) $(CXXFLAGS) $(SRC2) -o $(TARGET2) $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

part2:
	$(CXX) $(CXXFLAGS) $(SRC3) -o $(TARGET3) -lws2_32 -lwsock32