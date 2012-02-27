// Copyright (C) 2008 Timo Boettcher
//
// This is free software, licensed under the terms of the GNU General
// Public License as published by the Free Software Foundation.
// ======================================================================

#define	F_CPU		12000000	// 12MHz CPUclock

#include <avr/io.h>
#include <util/delay.h>

#define	USB_DDR				DDRD
#define	USB_PORT			PORTD
#define	USB_DPLUS			2
#define	USB_DMINUS_PULLUP		4
#define	USB_DMINUS			7

#define LED01_DDR			DDRD
#define LED01_PORT			PORTD
#define LED0				5	// RED
#define LED1				6	// GREEN

#define LED23_DDR			DDRB
#define LED23_PORT			PORTB
#define LED2				1	// BLUE
#define LED3				2	// WHITE

// ----------------------------------------------------------------------
static void delay_seconds (uint8_t seconds) {
	uint16_t i = 0;
	uint16_t j = 0;
	for (i = 0; i <= seconds; i ++) {
		for (j = 0; j <= 100; j ++) {
			_delay_ms(10);
		}
	}
}


// ----------------------------------------------------------------------
extern int main (void) {
	DDRB |= 0xff;					// Alle Pins des Ports B als Ausgang definieren
	DDRD |= 0xff;					// Alle Pins des Ports D als Ausgang definieren

	uint8_t i = 0;
	while (1) {
		for (i = 1; i < 3; i += 1) {
			//USB_PORT |= _BV(USB_DMINUS);		//Switch On
			LED01_PORT |= _BV(LED0);		//Switch On
			delay_seconds(i);

			//USB_PORT &= ~_BV(USB_DMINUS);		//Switch Off
			LED01_PORT &= ~_BV(LED0);		//Switch Off
			delay_seconds(i);

			//USB_PORT |= _BV(USB_DMINUS_PULLUP);	//Switch On
			LED01_PORT |= _BV(LED1);		//Switch On
			delay_seconds(i);

			//USB_PORT &= ~_BV(USB_DMINUS_PULLUP);	//Switch Off
			LED01_PORT &= ~_BV(LED1);		//Switch Off
			delay_seconds(i);

			//USB_PORT |= _BV(USB_DPLUS);		//Switch On
			LED23_PORT |= _BV(LED2);		//Switch On
			delay_seconds(i);

			//USB_PORT &= ~_BV(USB_DPLUS);		//Switch Off
			LED23_PORT &= ~_BV(LED2);		//Switch Off
			delay_seconds(i);

			LED23_PORT |= _BV(LED3);		//Switch On
			delay_seconds(i);

			LED23_PORT &= ~_BV(LED3);		//Switch Off
			delay_seconds(i);

		}

	}
	return 0;
}
