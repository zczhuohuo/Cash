CC=gcc
CFLAGS= -g -Wall -pedantic -O2 -march=athlon64
make: 
	gcc ${CFLAGS} cash.c -o cash
clean:
	rm cash
