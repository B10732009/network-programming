.PHONY: all run clean

CC = g++
CFLAGS = -O3 -Wall -std=c++17

# TYPE_SRC = $(wildcard types/*.cpp)
# TYPE_OBJ = $(patsubst %.cpp, %.o, $(TYPE_SRC))
# CMD_SRC = $(wildcard command/*.cpp)
# CMD_OBJ = $(patsubst %.cpp, %.o, $(CMD_SRC))
NPSHELL_SRC = npshell.cpp
NPSHELL_OBJ = npshell.o

TARGET = npshell
# CMD_TARGET = $(patsubst %.o, %, $(CMD_OBJ))

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f *.o types/*.o $(TARGET)

$(TARGET): $(NPSHELL_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# $(CMD_TARGET): $(CMD_OBJ)
# 	$(CC) $(CFLAGS) -o 

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
