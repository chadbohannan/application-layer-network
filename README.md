# expanding-link-protocol
Source, documentation and examples of ELP packet handlers in multiple programming languages.

# Why?
Packet frames are a useful means of moving data through a network. This library attempts to offer a packet composer/parser compatable with itself across several languages without the development overhead of related solutions such as (Protocol Buffers)[https://developers.google.com/protocol-buffers/].

This library offers a working packetizer & parser 'off the shelf' useful for applications build across heterogenous runtimes such as occur when linking sensor networks to cloud technologies.

# Packet Structure

<table>
  <tr>
    <td>Frame Leader*</td>
    <td>Control Flags*</td>
    <td>Source</td>
    <td>Destination</td>
    <td>Seq Num</td>
    <td>Ack Block</td>
    <td>Data Length</td>
    <td>Data</td>
    <td>CRC</td>
  </tr>
  <tr>
    <td>4</td>
    <td>2</td>
    <td>2</td>
    <td>2</td>
    <td>2</td>
    <td>4</td>
    <td>2</td>
    <td>Variable</td>
    <td>4</td>
  </tr>

  <tr>
    <td>"<<<<"</td>
    <td colspan="7">CRC Content</td>
    <td>CRC</td>
  </tr>
  <tr>
    <td></td>
    <td colspan="8" align="center">ELP Packet</td>
  </tr>
  <tr>
    <td colspan="9" align="center">ELP Frame</td>
  </tr>
</table>


Frames are delimited by four 0x3C ('<') characters. If contain data that would appear to indicate a frame delimiter then there will be a 0xC3 byte inserted ('byte stuffed') into the byte sequence after three '<' occur in sequence a packet.

A Frame contains a single Packet which starts with a two byte sequence of Control Flag bits. Of these 16 bits, 4 are used for forward error correction, 1 is static to prevent the frame delimeter byte ever being the first control byte, and the remaining 11 flag bits used to indicate which fields are present in the packet header.

If the Data flag is set the Data Length field will be followed by the number of bytes indicated.

If the CRC flag is set the last for bytes of the packet will be a CRC32 result of the prior bytes (unframed packet).

To be valid an ELP fram must contain a frame delimiter and 2 control flag bytes, it need not contain content, thus the minimum length of an ELP packet is 6 bytes; 4 delimeter bytes and 2 control bytes that contain only zeros. The largest packet supported by this implementation is composed by setting all 7 packet fields and maximizing data capacity is 65556 bytes. Such packets are expected when reliable mesh-networking protocols utilize all fields support content fragmentation and resequencing.


# Goals
This repository is young and ambitious. It is useful with only Packets and Parsers defined, but the ultimate goal is to provide a cross-language set of tools useful in developing self-organizing software applications across a broad range of application domians. The lofty goal of this repository is to provide intercompatable libraries with few dependencies so that new projects can be prototyped quickly, even if multiple languges and code bases are need to achieve nessessary functionality.
ELP intends to make as small a footprint in your application as possible, allowing for other technologies to replace it as your project matures and evolves.

# Planning
Here's the current state of intention:
 * Define Packet
   * 4 byte frame leader
   * 2 control bytes with 11 usable bits
   * 7 packet fields (4 unused control bits)
   * CRC32
 * Parse Packets
   * Buffers streaming input
   * Validated packets are accepted
 * Encapuslate Serial Links (single hop)
   * Assume full-duplix links, for now
   * Only protocol is framed packet transmission
 * Ad Hoc Networking (packet routing)
   * Develop protocol based on  [AODV](https://en.wikipedia.org/wiki/Ad_hoc_On-Demand_Distance_Vector_Routing) for multihop mesh networking
   * Encapsulate protocol handler into local mesh gateway
 * Transmission Control (end-to-end or 'socket' reliability)
   * Synchronize sequence numbers
   * Retransmit lost packets from rotating buffer
   * Ought to work both with a single-hop link or multi-hop socket


# Repository Status

Here's how far the intent has manifested:

## [C99](./c99/README.md)
 * Packet implemented but could be more efficient
 * Parser done.

## [C#](./csharp/README.md)
 * Packet done.
 * Parser done.

## [Go](./go/README.md)
 * Not started

## [Java](./java/README.md)
 * Started

 ## [Python 2.7](./python/README.md)
 * Packet done.
 * Parser done.

 