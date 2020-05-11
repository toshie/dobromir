#!/bin/bash
dudepath=/mnt/e/tools/avrdude
make clean && make && $dudepath/avrdude.exe -p m8 -c usbasp -P usb -U flash:w:dobromir.hex:a
