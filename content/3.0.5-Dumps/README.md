# Serial Communication Dumps

This folder includes dumps of serial communication in hex format from the laser to the controller and back (RX from laser, TX from controller)

Each file includes a 45 second capture of data

## Legend

Packet decoding is still a work in progress but here is a dump of what I know so far.

Bytes 1-4 = The same on every packet 49 4C 6D 70  - I L m p.

Byte 5 = Data length which starts at Byte 8

Byte 6 = The index of the packet. This is tracked separately on RX and TX and loops at 255

Byte 7 =  This appears to be a control character 00, 01, 07 - NUL SOH BEL. Null, Start of Header, and Alarm identifiers

Bytes 8-8+Byte5 = WIP Data. e.g. Firmware name, and possibly some multi byte Big5 encoded chinese characters in this data

Last Byte -2, -1 = WIP Unknown, possibly IR, Force, Temp values.

Last Byte = 0A \n End of packet




