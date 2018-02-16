elp-serialization : main.o packet.o parser.o
	cc -o elp-serialization main.o packet.o parser.o

main.o : main.c
	cc -c main.c

packet.o : ../../src/c99/packet.c ../../src/c99/packet.h
	cc -c ../../src/c99/packet.c ../../src/c99/packet.h

parser.o : ../../src/c99/parser.c ../../src/c99/parser.h
		cc -c ../../src/c99/parser.c ../../src/c99/parser.h

clean :
	rm  -f *.o packetstream.*
