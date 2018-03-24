# C99 reference implementation
This implementation avoids any external includes by declaring memory on the
stack to avoid use of malloc and free and does not print error messages, read
from or write to files.
This implementation is neither time nor space optimized. This code may be a
useful, however, in implementations for specific platforms with their own
byte stream I/O routines.

## types.c/h
Packet header fields are device independent integers. They are defined here
and stand-alone device independent byte array I/O routines are provided.

## packet.c/h
The core component of the protocol is the packet specification. Here is where
attribute names and sizes are defined.

## parser.c/h
Receiving packets requires storing stream parsing state variables. Here the
parser struct is defined which caches bytes until full packets are identified
and the packet_callback routine can called.
