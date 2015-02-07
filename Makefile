CC=g++-4.8 -std=c++0x
CFLAGS=-Wall -c

.PHONY: clean-leaves clean-all

all: driver clean-leaves

driver: driver.cpp
	$(CC) driver.cpp -o driver.out

driver.cpp: bplus.o

bplus.o: bplus.cpp
	$(CC) $(CFLAGS) bplus.cpp

clean-all: clean-leaves
	rm *.o driver.out

clean-leaves:
	rm leaves/*
	touch leaves/DUMMY
