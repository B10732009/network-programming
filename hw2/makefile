all:
	g++ -c -o include/npshell.o include/npshell.cpp
	g++ -c -o include/npshell3.o include/npshell3.cpp
	g++ -c -o include/user.o include/user.cpp
	g++ -c -o include/user3.o include/user3.cpp

	g++ -c -o np_simple.o np_simple.cpp
	g++ -c -o np_single_proc.o np_single_proc.cpp
	g++ -c -o np_multi_proc.o np_multi_proc.cpp

	g++ -o np_simple np_simple.o include/npshell.o
	g++ -o np_single_proc np_single_proc.o include/npshell.o include/user.o
	g++ -o np_multi_proc np_multi_proc.o include/npshell3.o include/user3.o