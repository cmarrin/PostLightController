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
for a custom language, Clover. See https://github.com/cmarrin/Clover. Clover uses the concepts of commands for calling functions to perform the various lighting effects. Because of the serial interface, commands for the post lights are a single character. You write a function for init and loop (in Arduino terms), indicate how many params are to be passed and associate it with a single character command string. The params passed to the serial interface must match the number of params specified in the command. The Interpreter init() function is called, passing the command and params. Those params are also available to the loop() function. The loop() function returns an int which is the number of milliseconds to delay before calling loop again. This allows accurate timing for the lighting effects.
	
Clover has a mechanism for adding modules, custom native functions for specific functionality. The post lights use a circular NeoPixel array with 8 addressable RGB LEDs (https://www.ebay.com/itm/392044823820). So the PostLightController adds a NativeColor module. This adds the concept of 4 color "registers" which can be manipulated by the functions in the module. There are functions to load a color (3 byte HSV) from the Params into one of the registers, loading and storing one color component (HSV) in one of the registers, and setting one light or all lights from a color register.

## Uploading
	
Commands are uploaded to the Aduino from the serial port in 64 byte binary chunks. The chunks are actually 66 bytes: 64 data bytes preceeded by a 2 byte offset of where to put the bytes in EEPROM. The Clover source is compiled on Mac into a series of 64 byte chunks saved to disk. A Node Red project (https://github.com/cmarrin/PondController-node-red-mac and https://github.com/cmarrin/PondController-node-red-mac) is used to upload. The Mac version is for testing but the system is intended to be run on a Raspberry Pi connected through its hardware serial port. You can connect to a PostLightController board from a USB to Serial board connected to the Mac and use the Node Red project to upload using the Send Executable button. See below for how to set up Node Red on Mac and RPi. The Mac compiler is a command line tool. You give it the Clover source file with the '-s' option to output a sequence of files with the same name as the input file minus the '.clvr' suffix, with a 2 digit sequence number and '.arlx'. These file are each 66 bytes long except for the last one, which is as long as needed.
	
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
