CC=g++-4.8 -std=c++0x
CFLAGS=-Wall -c

.PHONY: clean

all: driver

driver: driver.cpp
	$(CC) driver.cpp -o driver.out

driver.cpp: bplus.o

bplus.o: bplus.cpp
	$(CC) $(CFLAGS) bplus.cpp

clean:
	rm *.o driver.out leaves/*
