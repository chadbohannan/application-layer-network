elp-example : main.o link.o
	cc -o elp-example main.o

main.o : main.c
	cc -c main.c

link.o : ../../src/c99/link.c ../../src/c99/link.h
	cc -c ../../src/c99/link.c ../../src/c99/link.h
