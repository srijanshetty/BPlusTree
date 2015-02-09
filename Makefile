CC=g++-4.8 -std=c++11
CFLAGS=-Wall -c
DEBUG=-g

.PHONY: clean-leaves clean-all

all: tree.out

fresh: tree.out clean-leaves

tree.out: bplus.o
	$(CC) $(DEBUG) bplus.o -o tree.out

bplus.o: bplus.cpp
	$(CC) $(CFLAGS) $(DEBUG) bplus.cpp

clean-all: clean-leaves
	rm *.o *.out

clean-leaves:
	rm leaves/*
	touch leaves/DUMMY
