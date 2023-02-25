# serial

I wrote this in order to interact with some IC modules over serial.
Surprisingly I couldn't find any free terminal software that wouldn't try to
emulate vt100 or ANSI. This did the trick. Only tested on Mac (Ventura 13.2.1),
although it should work on Linux.

Requires:
- gcc-equivalent toolchain
- libserial (on mac: brew install libserial)

Compiling:
- make

Running:
- ./serial                        # will list available serial ports
- ./serial /dev/cu.usbserial-0001 # or your port name, then type away
