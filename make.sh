make clean && make && avrdude.exe -p m8 -c usbasp -P usb -U flash:w:$(pwd)/dobromir.hex:a
