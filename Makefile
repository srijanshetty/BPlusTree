CC=g++-4.8 -std=c++11
CFLAGS=-Wall -c
DEBUG=-g

.PHONY: clean-files clean-all

all: tree.out

fresh: tree.out clean-files

tree.out: bplus.o
	$(CC) $(DEBUG) bplus.o -o tree.out

bplus.o: bplus.cpp
	$(CC) $(CFLAGS) $(DEBUG) bplus.cpp

clean-all: clean-files
	rm *.o *.out

clean-files:
	rm -f .tree.session
	rm leaves/* objects/*
	touch leaves/DUMMY objects/DUMMY
