# Partyhat 2 software

Partyhat 2 is programmable a neopixel driver based on LPC824 micro controller. It can run custom animations created by the user. The animations are small Basic programs that can be coded with an internal editor over serial port.

The hardware for the project is in partyhat_hw repo.

## Building and flashing

1. Run: make all
You might need to install arm-none-aebi-gcc and arm-none-aebi-newlib packages.

2. Connect the board to serial port. Keep both buttons down when connecting the power to enter the ISP bootloader.

3. Run: make program 
Change the port from the make file if needed.

or

Run: make bin && python lpcprog.py -p /dev/ttyUSB0 ./build/flash.bin

## Editor

Connect the board to serial port and open termial with 115200 / 8n1. Press "e" to open the editor. You can also use numbers 1 - 8 to change the current animation or keys "zxcv" to change the brightness. 
