CC=g++-4.8 -std=c++11
CFLAGS=-Wall
DEBUG=-g

.PHONY: clean-leaves clean-all

all: tree clean-leaves

tree: bplus.cpp
	$(CC) $(DEBUG) bplus.cpp -o tree.out

clean-all: clean-leaves
	rm *.o *.out

clean-leaves:
	rm leaves/*
	touch leaves/DUMMY
