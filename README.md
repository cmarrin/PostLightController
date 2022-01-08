# PostLightController
## Code for Pond controller
Software runs on Arduino Nano. SoftSerial is used to receive packets of data fom the host (running Node-Red).

### TODO
- ☐ Get rid of new to create Effects. Create one of each and point to it as needed
- Get rid of color table
- Change packet to be variable length: start ('('), device addr ('0' = all devices), cmd, payload len (0-63), checksum, end (')')
- Make payload a binary array, colors are 2 bytes: first byte is hue (upper 4 bits 0-15) and sat (lower 4 bits 0-15), second byte val (0-255)
- Make Effect a concrete class, executing interpreted code specified by command

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
