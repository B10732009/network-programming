.PHONY: all run clean

CC = g++
CFLAGS = -O3 -Wall -std=c++17

NPSHELL_SRC = npshell.cpp
NPSHELL_OBJ = npshell.o

TARGET = npshell

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f *.o types/*.o $(TARGET)

$(TARGET): $(NPSHELL_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
