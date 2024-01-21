# Serial Communication Dumps

This folder includes dumps of serial communication in hex format from the laser to the controller and back (RX from laser, TX from controller)

Each file includes a 45 second capture of data

## Legend

Packet decoding is still a work in progress but here is a dump of what I know so far.

Bytes 1-4 = The same on every packet 49 4C 6D 70  - I L m p.

Byte 5 = Data length which starts at Byte 8

Byte 6 = The index of the packet. This is tracked separately on RX and TX and loops at 255

Byte 7 =  This appears to be a control character 00, 01, 07 - NUL SOH BEL. Null, Start of Header, and Alarm identifier

Byte 8 = Byte 8 appears to be a control character on every message but heading messages (01 in byte 7). On headings this is either part of the message or an unclosed '(' at the beginning of the message. Control characters in Byte 8 are 00, 02, 80 - NUL, SOT, PAD. Null, Start of Text, and Padding

Bytes 9-9+Byte5 = Data. e.g. Firmware name, and possibly some multi byte Big5 encoded chinese characters in this data.

Last Byte -1 = Unknown, possibly IR, Force, Temp values.

Last Byte = 0A \n End of packet

| Bytes 1-4    | Byte 5 | Byte 6 |  Byte 7 | 
|--------------|--------|--------|---------|
| PREFIX       | LENGTH | INDEX  |   BEL   |
| 49 4C 6D 70  |   08   |   01   |   07    |
| I  L  m  p   |   --   |   --   |   --    |

| Byte 8                                                     |
|------------------------------------------------------------|
| ? Control character except on heading message during boot. |
| 00, 02, 80 on Alarm messages. 28 on heading message        |
| 00 = NUL, 02 = Start of Text, 80 = PAD, 28 = (             |


| Byte 9+                         | Air | Cn |
|---------------------------------|-----|----|
| Data                            |     |    |   
| *3F* **B9 47** *00 70* 27 45 9B | max | 逼 |
|  En = force / compel / close in on         |
| *37* **B8 47** *00 B0* 07 45 B2 | mid | 矮 |
|  En = low / short                          |
| *03* **B4 47** *00 00* 80 3F BD | min | 孱 |
|  En = weak / delicate                      |
| *FD* **B3 47** *00 00* 00 41 B8 | min | 蛄 |
|  En = mayfly?? / mole cricket??            |
| *00* **B4 47** *00 00* 40 40 7B | no  | 孱 |
|  En = weak / delicate                      |
| *FF* **B3 47** *00 00* 80 40 39 | no  | 蛄 |
|  En = mayfly?? / mole cricket??            |

| Last Byte -1                | Last Byte   |
|-----------------------------|-------------|
|    ?                        | New Line    |
| 01,02,03, 09(in header)     |    0A       |
| Possibly a dec/bin value    |             |
| Control character doesnt makes sense here |