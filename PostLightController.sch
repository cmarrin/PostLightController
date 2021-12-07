EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L MCU_Module:Arduino_Nano_v3.x A1
U 1 1 61ACEE2D
P 5500 3400
F 0 "A1" H 5500 2311 50  0000 C CNN
F 1 "Arduino_Nano_v3.x" H 5500 2220 50  0000 C CNN
F 2 "Modules:Arduino_Nano" H 5500 3400 50  0001 C CIN
F 3 "http://www.mouser.com/pdfdocs/Gravitech_Arduino_Nano3_0.pdf" H 5500 3400 50  0001 C CNN
	1    5500 3400
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x03_Female J1
U 1 1 61AD069E
P 3750 3300
F 0 "J1" H 3850 3350 50  0000 C CNN
F 1 "To Light" H 3950 3250 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x03_Pitch2.54mm" H 3750 3300 50  0001 C CNN
F 3 "~" H 3750 3300 50  0001 C CNN
	1    3750 3300
	-1   0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x03_Male J2
U 1 1 61AD2054
P 3750 3700
F 0 "J2" H 3700 3750 50  0000 R CNN
F 1 "Data Out" H 3700 3650 50  0000 R CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x03_Pitch2.54mm" H 3750 3700 50  0001 C CNN
F 3 "~" H 3750 3700 50  0001 C CNN
	1    3750 3700
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x03_Male J3
U 1 1 61AD2B9A
P 3750 4600
F 0 "J3" H 3650 4650 50  0000 C CNN
F 1 "Data In" H 3550 4550 50  0000 C CNN
F 2 "Pin_Headers:Pin_Header_Straight_1x03_Pitch2.54mm" H 3750 4600 50  0001 C CNN
F 3 "~" H 3750 4600 50  0001 C CNN
	1    3750 4600
	1    0    0    -1  
$EndComp
Wire Wire Line
	3950 3200 4550 3200
Wire Wire Line
	4550 3200 4550 3700
Wire Wire Line
	4550 4600 5600 4600
Wire Wire Line
	5600 4400 5600 4600
Wire Wire Line
	3950 4500 4400 4500
Wire Wire Line
	4400 4500 4400 3600
Wire Wire Line
	4400 2300 5700 2300
Wire Wire Line
	3950 3300 4400 3300
Connection ~ 4400 3300
Wire Wire Line
	4400 3300 4400 2300
Wire Wire Line
	3950 3600 4400 3600
Connection ~ 4400 3600
Wire Wire Line
	4400 3600 4400 3300
Wire Wire Line
	3950 3700 4550 3700
Connection ~ 4550 3700
Wire Wire Line
	4550 3700 4550 4100
Wire Wire Line
	3950 4600 4550 4600
Connection ~ 4550 4600
Wire Wire Line
	3950 4700 4300 4700
Wire Wire Line
	4850 4700 4850 3900
Wire Wire Line
	4850 3900 5000 3900
Wire Wire Line
	3950 3400 5000 3400
Wire Wire Line
	3950 3800 4000 3800
Wire Wire Line
	5700 2300 5700 2400
$Comp
L Device:C C2
U 1 1 61AF482E
P 4300 4850
F 0 "C2" H 4415 4896 50  0000 L CNN
F 1 "C" H 4415 4805 50  0000 L CNN
F 2 "Capacitors_THT:C_Rect_L7.0mm_W2.0mm_P5.00mm" H 4338 4700 50  0001 C CNN
F 3 "~" H 4300 4850 50  0001 C CNN
	1    4300 4850
	1    0    0    -1  
$EndComp
Connection ~ 4300 4700
Wire Wire Line
	4300 4700 4850 4700
$Comp
L Device:C C1
U 1 1 61AF5FC2
P 4000 3950
F 0 "C1" H 4115 3996 50  0000 L CNN
F 1 "C" H 4115 3905 50  0000 L CNN
F 2 "Capacitors_THT:C_Rect_L7.0mm_W2.0mm_P5.00mm" H 4038 3800 50  0001 C CNN
F 3 "~" H 4000 3950 50  0001 C CNN
	1    4000 3950
	1    0    0    -1  
$EndComp
Connection ~ 4000 3800
Wire Wire Line
	4000 3800 5000 3800
Wire Wire Line
	4000 4100 4550 4100
Connection ~ 4550 4100
Wire Wire Line
	4550 4100 4550 4600
Wire Wire Line
	4300 5000 4550 5000
Wire Wire Line
	4550 5000 4550 4600
$EndSCHEMATC
