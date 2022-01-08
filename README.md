# PostLightController
## Code for Pond controller

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
