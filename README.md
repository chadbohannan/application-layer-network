# expanding-link-protocol
Source, documentation and examples of ELP packet handlers in multiple programming languages.


# Packet Structure
![alt text][elp-packet.png]

Packets are delimited on the stream by 4 '<' characters.

The controlFlags field is a set of bit flags that indicate the presence of header fields. The shortest valid packet is 6 bytes: 4 '<' and 2 null (zero) bytes; the 'empty' packet can be used to keep links alive when no data is available.
