# makefile

all: shell

Jobs.o: Jobs.h Jobs.cpp
	g++ -g -w -std=c++14 -c Jobs.cpp

Command.o: Command.h Command.cpp
	g++ -g -w -std=c++14 -c Command.cpp

shell: shell.cpp Command.o Jobs.o
	g++ -g -w -std=c++14 -o shell shell.cpp Command.o Jobs.o -lpthread -lrt

clean:
	rm -rf *.o shell
