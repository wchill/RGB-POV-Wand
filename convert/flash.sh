#!/bin/bash
./avrdude -C /home/wchill/sketchbook/RGB_POV_Wand/convert/avrdude.conf -p atmega32u4 -c avr109 -P /dev/ttyACM0 -b 57600 -D -U eeprom:w:temp.bin:r
