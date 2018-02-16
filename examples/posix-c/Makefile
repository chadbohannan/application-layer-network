elp-serialization : main.o link.o packet.o
	cc -o elp-serialization main.o link.o

main.o : main.c
	cc -c main.c

link.o : ../../src/c99/link.c ../../src/c99/link.h
	cc -c ../../src/c99/link.c ../../src/c99/link.h

packet.o : ../../src/c99/packet.c ../../src/c99/packet.h
	cc -c ../../src/c99/packet.c ../../src/c99/packet.h
