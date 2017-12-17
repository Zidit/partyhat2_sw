/*
===============================================================================
 Name        : Partyhat.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#include "nvm.h"
#include "sine.h"
#include "chip.h"
#include "serial.h"
#include "printf.h"
#include "ws2812.h"
#include "editor.h"
#include "config.h"
#include "ubasic/ubasic.h"

#define Rev2

#define PIN_LED		22
#define PIN_STRIP	10
#define PIN_PROG	12
#define PIN_BRIGHT	23

volatile unsigned int time = 0;

#ifndef elements
#define elements(x) (sizeof(x) / sizeof(x[0]))
#endif



static uint8_t icon_lut[] = {
	IOCON_PIO0, IOCON_PIO1, IOCON_PIO2, IOCON_PIO3, IOCON_PIO4, IOCON_PIO5,
	IOCON_PIO6, IOCON_PIO7, IOCON_PIO8, IOCON_PIO9, IOCON_PIO10, IOCON_PIO11, 
	IOCON_PIO12, IOCON_PIO13, IOCON_PIO14, IOCON_PIO15, IOCON_PIO16, IOCON_PIO17, 
	IOCON_PIO18, IOCON_PIO19, IOCON_PIO20, IOCON_PIO21, IOCON_PIO22, IOCON_PIO23,
	IOCON_PIO24, IOCON_PIO25, IOCON_PIO26, IOCON_PIO27, IOCON_PIO28,  
};

void SysTick_Handler(void)
{
    time++;
    if((time % 100) == 0) {
    	Chip_GPIO_SetPortToggle(LPC_GPIO_PORT, 0, 10);
    }
}

VARIABLE_TYPE peek(VARIABLE_TYPE arg)
{
	if(arg >= 0 && arg <= sizeof(strip)) {
		return *((char*)strip + arg);
	}
	return 0;
}

void poke(VARIABLE_TYPE arg, VARIABLE_TYPE value)
{
	if(arg >= 0 && arg <= sizeof(strip)) {
		value = value > 255 ? 255 : value;
		value = value < 0 ? 0 : value;
		*((uint8_t*)strip + arg) = value;
	}
}

int run_editor(int cur_prog)
{
	Chip_SWM_MovablePinAssign(SWM_U0_TXD_O, 4);
	clear_data();
	send_strip_data(sizeof(strip));
	return editor(cur_prog);
}

int main(void)
{
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
    Chip_SWM_Init();
    Chip_GPIO_Init(LPC_GPIO_PORT);
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_IOCON);
    Chip_Clock_SetIOCONCLKDIV(IOCONCLKDIV0, 255);

    //Strip output
    Chip_SWM_DisableFixedPin(SWM_FIXED_I2C0_SCL);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 0, PIN_STRIP);
    Chip_IOCON_PinSetMode(LPC_IOCON, icon_lut[PIN_STRIP],  PIN_MODE_INACTIVE);

	//Setup buttons
	#ifdef Rev2
	Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 0, PIN_BRIGHT);
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 0, PIN_PROG);
    Chip_IOCON_PinSetMode(LPC_IOCON, icon_lut[PIN_BRIGHT], PIN_MODE_INACTIVE);
	Chip_IOCON_PinSetMode(LPC_IOCON, icon_lut[PIN_PROG], PIN_MODE_INACTIVE);
	#elif
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 0, 4);
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 0, 12);
    Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO4,  PIN_MODE_INACTIVE);
	Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO12, PIN_MODE_INACTIVE);
	#endif

	//Setup ext
	#ifdef Rev2
	Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 0, 9);
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 0, 14);
    Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO9, PIN_MODE_PULLUP);
	Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO14, PIN_MODE_PULLUP);
	#elif
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 0, 6);
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 0, 7);
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 0, 14);
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, 0, 22);
    Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO6, PIN_MODE_INACTIVE);
    Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO7, PIN_MODE_INACTIVE);
    Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO14, PIN_MODE_INACTIVE);
	Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO22, PIN_MODE_INACTIVE);
	#endif

    //Setup led
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 0, PIN_LED);
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, 0, PIN_LED, false);
	Chip_IOCON_PinSetMode(LPC_IOCON, icon_lut[PIN_LED], PIN_MODE_INACTIVE);


    //Setup uart
	Chip_SWM_MovablePinAssign(SWM_U0_TXD_O, 4);
	Chip_SWM_MovablePinAssign(SWM_U0_RXD_I, 0);
	Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO0,  PIN_MODE_REPEATER);
	Chip_IOCON_PinSetMode(LPC_IOCON, IOCON_PIO4,  PIN_MODE_INACTIVE);
    serial_init();
    serial_set_baud(115200);

    //Set unused pins to output and low
	#ifdef Rev2
	static const char unused[] = {1,6,7,8,11,13,15,16,17,18,19,20,21,24,25,26,28};
	#elif
	static const char unused[] = {1,8,9,11,13,15,16,17,18,19,20,21,24,25,26,27,28};
	#endif
    int i;
    for(i = 0; i < elements(unused); i++) {
    	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, 0, unused[i]);
    	Chip_GPIO_SetPinState(LPC_GPIO_PORT, 0, unused[i], false);
    	Chip_IOCON_PinSetMode(LPC_IOCON, icon_lut[unused[i]],  PIN_MODE_INACTIVE);
    }

    __enable_irq();

	/* Enable SysTick Timer */
    SysTick_Config(SystemCoreClock / 1000);

    if(program_number > 6)
    	program_number = 5;
	if(brightness > 4)
		brightness = 4;

	brightness = 4;
	program_number = 2;

    while(1){
		printf("\033[2J");
		printf("\033[H");
		printf("Running: %i\n", program_number + 1);
		printf("Press 'e' for editor or 1-8 to change program");

		ubasic_init_peek_poke(get_file_ptr(program_number), &peek, &poke);

		clear_data();
		send_strip_data(300);

		do {
			ubasic_run();

			static bool was_down = false;
			if(!Chip_GPIO_GetPinState(LPC_GPIO_PORT, 0, PIN_PROG)){
				if(!was_down){
					was_down = true;
					program_number++;
					if(program_number > 6)
						program_number = 0;
					break;
				}
			} else {
				was_down = false;
			}

			static bool was_down2 = false;
			if(!Chip_GPIO_GetPinState(LPC_GPIO_PORT, 0, PIN_BRIGHT)){
				if(!was_down2){
					was_down2 = true;
					brightness ++;
					if(brightness > 4)
						brightness = 1;
				}
			} else {
				was_down2 = false;
			}

			if(serial_data_available())
			{
				printf("Program terminated.");
				while(!serial_data_available());

				char c = serial_get_char();
				if(c >= '1' && c <= '7'){
					program_number = c - '1';
					break;
				} else if (c == 'e') {
					while(serial_data_available())
						serial_get_char();

					program_number = run_editor(program_number);
					break;
				} else if(c == 'z'){
					brightness = 1;
				} else if(c == 'x'){
					brightness = 2;
				} else if(c == 'c'){
					brightness = 3;
				} else if(c == 'v'){
					brightness = 4;
				}

			}

		} while(!ubasic_finished());
    }
}






