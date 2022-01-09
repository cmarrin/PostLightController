# PostLightController
## Code for Pond controller
Software runs on Arduino Nano. SoftSerial is used to receive packets of data fom the host (running Node-Red).

### TODO
- ☑︎ Get rid of color table
- ☑︎ Create a Color class which takes rgb or hsv values and can give back color as uint32_t (0x00RRGGBB) or hsv (3 floats 0-1)
- ☑︎ Make all effects use Color class
- ☑︎ Make payload a binary array, colors are 3 bytes: hue, sat and val, all 0-255
- ☑︎ Make node-red generate colors in new format
- ☐ Get rid of new to create Effects. Create one of each and point to it as needed
- ☐ Change packet to be variable length: start ('('), device addr ('0' = all devices), cmd, payload len (0-63), checksum, end (')')
- ☐ Make node-red use variable length format
- ☐ Make Effect a concrete class, executing interpreted code specified by command

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
