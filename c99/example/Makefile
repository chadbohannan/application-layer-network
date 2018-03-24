elp-serialization : main.o packet.o parser.o types.o
	cc -o elp-serialization main.o packet.o parser.o types.o

main.o : main.c
	cc -c main.c

packet.o : ../src/packet.c ../src/packet.h
	cc -c ../src/packet.c ../src/packet.h

parser.o : ../src/parser.c ../src/parser.h
		cc -c ../src/parser.c ../src/parser.h

types.o : ../src/types.c ../src/types.h
		cc -c ../src/types.c ../src/types.h

clean :
	rm  -f *.o packetstream.* ../src/*.gch
