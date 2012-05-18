CC=gcc
CFLAGS= -g -Wall -pedantic -O2 -march=athlon64 -lreadline
make: 
	gcc ${CFLAGS} cash.c built_ins.c -o cash
clean:
	rm cash
