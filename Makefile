CC=gcc
CFLAGS= -g -Wall -pedantic -std=gnu99 -O2 -march=athlon64
make: 
	${CC} ${CFLAGS} cash.c -o cash
clean:
	rm cash
