elp-serialization : main.o packet.o parser.o types.o
	cc -o elp-serialization main.o packet.o parser.o types.o

main.o : main.c
	cc -c main.c

packet.o : ../../src/c99/packet.c ../../src/c99/packet.h
	cc -c ../../src/c99/packet.c ../../src/c99/packet.h

parser.o : ../../src/c99/parser.c ../../src/c99/parser.h
		cc -c ../../src/c99/parser.c ../../src/c99/parser.h

types.o : ../../src/c99/types.c ../../src/c99/types.h
		cc -c ../../src/c99/types.c ../../src/c99/types.h

clean :
	rm  -f *.o packetstream.* ../../src/c99/*.h.*
