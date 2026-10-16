/*
 * main.c
 * 
 * FreeExpression firmware, main program
 *
 * This source original developed by  https://github.com/Arlet/Freecut
 *
 * This file is part of FreeExpression.
 *
 * https://github.com/thetazzbot/FreeExpression
 *
 * FreeExpression is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2.
 *
 * FreeExpression is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FreeExpression. If not, see http://www.gnu.org/licenses/.
 *
 */

// new Machine Selection header
#include "machine.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <inttypes.h>
#include <stdio.h>

#include "usb.h"
#include "keypad.h"
#include "timer.h"
#include "stepper.h"
#include "cli.h"
#include "flash.h"
#include "version.h"
#include "dial.h"
#include "hpgl.h"
#include "display.h"
void setup(void);


void setup(void)
{
	// Watchdogging disabled -- No use while debugging / testing 
	//wdt_enable( WDTO_30MS );
	keypad_init( );
	display_init();
	usb_init();
	timer_init( );
	stepper_init( );
	flash_init( );
	hpgl_init();
	dial_init( );
		
	sei();					// Start interrupts -- Motors will home immediately following this
	
	msleep(100);
	
	display_print(VERSION);
	
	usb_puts("\f");
	usb_puts(VERSION);

}


int main( void )
{
	setup();
	
	// Create a fast state counter to decouple screen refresh from the input checks
	uint8_t display_divider = 0; 
	
	// Draw the starting state layout once immediately after initialization
	oled_display_update(); 

    while( 1 )
    {
        // 1. ABSOLUTE HIGHEST PRIORITY: Process USB commands and G-code/HPGL movement steps.
        cli_poll( ); 
		wdt_reset( );
		
		// 2. TIMED TASK MANAGER: Executes every 40 milliseconds (25 Hz)
		if( flag_25Hz )
		{
			flag_25Hz = 0;
			
			// Read pots/dials and check keypresses immediately
			dial_poll( );  
			keypad_poll( ); 

			// --- DECOUPLED DISPLAY THROTTLING ---
			// display_divider increments on every 40ms tick:
			// 0 -> Skip (Run key/dial polls)
			// 1 -> Trigger Screen Refresh -> Reset to 0
			// This limits the screen to a comfortable ~12.5 FPS, clearing 
			// up massive processing windows for the rest of your system tasks.
			display_divider++;
			if (display_divider >= 2)
			{
				display_divider = 0;
				oled_display_update(); // Clears buffer, renders objects, and bursts data over SPI
			}
		}
		
		if( flag_Hz )
		{
			flag_Hz = 0;
		}
    }
}
