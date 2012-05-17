CC=gcc
CFLAGS= -g -Wall -pedantic -std=gnu99 -O2 -march=athlon64
make: 
	${CC} cash.c -o cash ${CFLAGS}
clean:
	rm cash
