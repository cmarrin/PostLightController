# PostLightController

## Introduction
This is a controller for post lights along my fence. Each post has a NeoPixels 8 pixel ring and an Arduino Nano to drive it.
It receives serial data from a Raspberry Pi with 0-5v levels and no line conditioning other than 0.01uf capacitors on the input
and output serial lines. Data comes in on pin 11 and is sent out on pin 10. Post Light Controllers are connected in serial. The
previous controller sends data to the next. Each serial packet contains an address. An address of 0 sends the packet to all
controller, so every byte is simply passed on. If the address is 1, this packet is for the receiving controller. It consumes it
and doesn't send it on. If it is greater than 1 the address is decremented and the packet is sent to the next controller. So for
instance an address of 5 is consumed by the 5th controller in the chain.

SoftSerial is used to receive and send packets. Each packet consists of a variable number of bytes, starting with a lead-in ('('),
address, command and size. This is followed by <size> bytes of payload then by a checksum byte and a lead-out (')'). The command
tells the controller what sequence to run. For instance it can set a constant color ('c') of flicker the lights to looks like a
burning candle ('f'). The payload is specific to the command. But most commands start with a color as a byte each of hue,
saturation and value (brightness). 
	
## Interpreter
	
In order to avoid having to reprogram each post light controller whenever a command is added or changed, the system has an interpreter
for a custom language, Arly. This is a simple language, almost at the machine code level. The runtime engine has 4 registers that
can hold an int32 or float and 4 color registers. Basic math functions are available for both ints and floats, as well as register
moving and load/store of values, either direct or indexed. For loops and if-then statements are available. There are also special 
instructions for setting the lights, generating a random number, etc. Read/write store of 64 ints or floats can be defined for each 
command, single int or float constants and constant tables (int or float) can be defined. Constants and the binary program to be 
interpreted are stored in EEPROM.
	
The interpreter is very simple. For instance nested for loops are not allowed and loops and if-else clauses are limited to 64 bytes
in length. Arly is written in a source format which is compiled into binary in 64 byte blocks. Each block is preceeded by a 2 byte
address where that block is loaded in EEPROM. The controllers have an 'X' command to upload the blocks to EEPROM. To avoid wearing
out the EEPROM it is only written to with the 'X' command. All interpreted instructions that store values, place them in the 64
word RAM store.

### TODO
- [x] Get rid of color table
- [x] Create a Color class which takes rgb or hsv values and can give back color as uint32_t (0x00RRGGBB) or hsv (3 floats 0-1)
- [x] Make all effects use Color class
- [x] Make payload a binary array, colors are 3 bytes: hue, sat and val, all 0-255
- [x] Make node-red generate colors in new format
- [x] Get rid of new to create Effects. Create one of each and point to it as needed
- [x] Write Arly Compiler
- [x] Write Arly test file, implementing ConstantColor and Flicker commands
- [x] Write Decompiler to see what code is getting generated
- [x] Write Interpreter for code
- [x] Write Simulator on Mac, specializing Interpreter and printing out pixel values, etc.
- [x] Add Interpreter to Arduino as a subclass of Effect
- [x] Add command(s) to upload Arly executable to Arduino
- [x] Add interpreted commands to node-red
- [x] Once everything is working remove ConstantColor and Flicker classes
- [x] Change packet to be variable length: start ('('), device addr ('0' = all devices), cmd, payload len (0-63), checksum, end (')')
- [x] Make node-red use variable length format

## Board Fab
To send PostLightController board to seeedstudio (https://www.seeedstudio.com/fusion_pcb.html):

	1) open PostLightController.kicad_pcb

	2) go to File->Plot...

	3) Make sure Use Extended X2 Format is not checked

	4) Make sure Output folder is 'gbr/'

	5) Press Plot to generate all the files

	6) Press Generate Drill Files...

	7) Press Generate Drill Files on the sheet that comes up

	8) Go to console in the PostLightController directory and run 'zip PostLightController-gbr.zip gbr/*'

	9) Upload the resulting PostLightController-gbr.zip to SeeedStudio
