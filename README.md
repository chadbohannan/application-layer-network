# expanding-link-protocol
Source, documentation and examples of ELP packet handlers in multiple programming languages.

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
    <td>"<<<<"</td>
    <td colspan="7">CRC Content</td>
    <td>CRC</td>
  </tr>
  <tr>
    <td></td>
    <td colspan="8">ELP Packet</td>
  </tr>
  <tr>
    <td colspan="9">ELP Frame</td>
  </tr>
</table>


# Packet Structure
![alt text](elp-packet.png)

Packets are delimited on the stream by 4 '<' characters.

The controlFlags field is a set of bit flags that indicate the presence of header fields. The shortest valid packet is 6 bytes: 4 '<' and 2 null (zero) bytes; the 'empty' packet can be used to keep links alive when no data is available.
