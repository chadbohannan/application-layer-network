# C99 reference implementation
This implementation avoids any external includes by declaring memory on the
stack to avoid use of malloc and free and does not print error messages, read
from or write to files.
This implementation is neither time nor space optimized. This code may be a
useful, however, in implementations for specific platforms with their own
byte stream I/O routines.
