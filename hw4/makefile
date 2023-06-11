CXX = g++
CXXFLAGS = -std=c++14 -Wall -pedantic -pthread -lboost_system
CXX_INCLUDE_DIRS = /usr/local/include
CXX_INCLUDE_PARAMS = $(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS = /usr/local/lib
CXX_LIB_PARAMS = $(addprefix -L , $(CXX_LIB_DIRS))

SRC1 = socks_server.cpp
SRC2 = console.cpp

TARGET1 = socks_server
TARGET2 = hw4.cgi

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(SRC1)
	$(CXX) $(SRC1) -o $(TARGET1) $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS)

$(TARGET2): $(SRC2)
	$(CXX) $(SRC2) -o $(TARGET2) $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS)