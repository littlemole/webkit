

all:
	g++ -g -std=c++17 $(shell pkg-config --cflags gtk+-3.0) test.cpp $(shell pkg-config --libs gtk+-3.0)

clean:
	-rm a.out
	