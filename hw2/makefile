.PHONY: all run1 run2 run3 clean

CC = g++
CFLAGS = -O3 -Wall -std=c++17

SRC1 = np_simple.cpp
UTIL = npshell.cpp npsocket.cpp

TARGET1 = np_simple

DEFAULT_PORT = 12000

all: $(TARGET1)

run1: $(TARGET1)
	./$(TARGET1) $(DEFAULT_PORT)
	
$(TARGET1): $(SRC1) $(UTIL)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<







# all:
# 	g++ -c -o include/npshell.o include/npshell.cpp
# 	g++ -c -o include/npshell3.o include/npshell3.cpp
# 	g++ -c -o include/user.o include/user.cpp
# 	g++ -c -o include/user3.o include/user3.cpp

# 	g++ -c -o np_simple.o np_simple.cpp
# 	g++ -c -o np_single_proc.o np_single_proc.cpp
# 	g++ -c -o np_multi_proc.o np_multi_proc.cpp

# 	g++ -o np_simple np_simple.o include/npshell.o
# 	g++ -o np_single_proc np_single_proc.o include/npshell.o include/user.o
# 	g++ -o np_multi_proc np_multi_proc.o include/npshell3.o include/user3.o