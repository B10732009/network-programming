CC = g++
CFLAGS = -O3 -Wall -std=c++17

SRC1 = np_simple.cpp
SRC2 = np_single_proc.cpp
UTIL = npshell.cpp npsocket.cpp user.cpp

OBJ1 = $(patsubst %.cpp, %.o, $(SRC1))
OBJ2 = $(patsubst %.cpp, %.o, $(SRC2))
OBJUTIL = $(patsubst %.cpp, %.o, $(UTIL))

TARGET1 = $(patsubst %.cpp,%, $(SRC1))
TARGET2 = $(patsubst %.cpp,%, $(SRC2))

DEFAULT_PORT = 12000

.PHONY: all run1 run2 run3 clean
.PRECIOUS: $(OBJ1) $(OBJ2) $(OBJUTIL)

all: $(TARGET1) $(TARGET2)

run1: $(TARGET1)
	./$(TARGET1) $(DEFAULT_PORT)

run2: $(TARGET2)
	./$(TARGET2) $(DEFAULT_PORT)

clean:
	rm -f *.o $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJ1) $(OBJUTIL)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET2): $(OBJ2) $(OBJUTIL)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
