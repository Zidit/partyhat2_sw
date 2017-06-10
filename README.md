# Partyhat 2 software

Partyhat 2 is programmable a neopixel driver based on LPC824 micro controller. It can run custom animations created by the user. The animations are small Basic programs that can be coded with an internal editor over serial port.

The hardware for the project is in partyhat_hw repo.

## Programing 

The firmaware can be programed with LPC's bootloader over serial port. To program use lpcprog.py included in the project. Expample: 'python lpcprog.py -p com1 Partyhat.bin' or 'python lpcprog.py -p /dev/ttyUSB0 Partyhat.bin'

You can use the precompiled binary or compile using LPCxpresso IDE. (TODO: independent makefile).

## Editor

Connect the board to serial port and open termial with 115200 / 8n1. Press "e" to open the editor. You can also use numbers 1 - 8 to change the current animation or keys "zxcv" to change the brightness. 
