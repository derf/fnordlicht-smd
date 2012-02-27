/* vim:fdm=marker ts=4 et ai
 * {{{
 *
 *		 fnordlicht-smd firmware
 *
 *  by Timo Boettcher <fnordlicht@spida.net>
 *   based on work by
 *  Alexander Neumann <alexander@bumpern.de> and
 *  Lars Noschinski <lars@public.noschinski.de>
 *
 * see http://www.spida.net/projects/fnordlicht-smd/ for details
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 }}} */


/* includes */
#include "config.h"

#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <stdlib.h>

#include "common.h"
#include "fnordlicht.h"
#include "pwm.h"

#include <util/delay.h>

#if RC5_DECODER
#include "rc5.h"
#endif

#if USB
#include "usbdrv.h"
#endif

#if STATIC_SCRIPTS

/* include static scripts */
#include "static_scripts.h"
#include "testscript.h"

#endif


/* structs */
volatile struct global_t global = {{0}};

/* prototypes */
void (*jump_to_bootloader)(void) = (void *)0xc00;
static inline void init_output(void);

/** init output channels */
void init_output(void) { /* {{{ */
	/* configure PC0-PC3 as outputs */
	LED01_DDR = _BV(LED_CHANNEL0) | _BV(LED_CHANNEL1);
	LED23_DDR = _BV(LED_CHANNEL2) | _BV(LED_CHANNEL3);
	/* set all channels high -> leds off */
#if HAS_INVERTER
	LED01_PORT = _BV(LED_CHANNEL0) | _BV(LED_CHANNEL1);
	LED23_PORT = _BV(LED_CHANNEL2) | _BV(LED_CHANNEL3);
#else
	LED01_PORT = ~_BV(LED_CHANNEL0) && ~_BV(LED_CHANNEL1);
	LED23_PORT = ~_BV(LED_CHANNEL2) && ~_BV(LED_CHANNEL3);
#endif
}
/* }}} */


#if USB
USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) { /* {{{ */
	usbRequest_t	*rq = (void *)data;
	uchar		status = 0;

	if (rq->bRequest == FUNC_VERSION) {
		static uchar	replyBuf[6];
		usbMsgPtr = replyBuf;
		/* Hardware Version */
		replyBuf[0] = 0x0;
		replyBuf[1] = 0x2;
		/* Software Version */
		replyBuf[2] = 0x0;
		replyBuf[3] = 0x0c;	// 12
		/* Protocol Version */
		replyBuf[4] = 0x0;
		replyBuf[5] = 0x3;
		return 6;
	}
	if (rq->bRequest == FUNC_RESET) {
		reboot = 1;
		return 0;
	}
	if (rq->bRequest == FUNC_ECHO) {
		static uchar	replyBuf[8];
		usbMsgPtr = replyBuf;
		replyBuf[0] = rq->wValue.bytes[0];
		replyBuf[1] = rq->wIndex.bytes[0];
		replyBuf[2] = rq->wValue.bytes[0];
		replyBuf[3] = rq->wIndex.bytes[0];
		replyBuf[4] = rq->wValue.bytes[0];
		replyBuf[5] = rq->wIndex.bytes[0];
		replyBuf[6] = rq->wValue.bytes[0];
		replyBuf[7] = rq->wIndex.bytes[0];
		return 8;
	}
	if (rq->bRequest == FUNC_ENUMERATE) {
		static uchar	replyBuf[1];
		busid = rq->wIndex.bytes[0];
		deviceid = rq->wValue.bytes[0];
		/* check if we are meant */
		usbMsgPtr = replyBuf;
		replyBuf[0] = 0x1;
		return 1;
	}
	if (rq->bRequest == FUNC_GET_STATUS) {
		static uchar	replyBuf[4];
		usbMsgPtr = replyBuf;
		busid = rq->wIndex.bytes[0];
		deviceid = rq->wValue.bytes[0];
		/* check if we are meant */
		replyBuf[3] = CHANNEL3_PWM;
		replyBuf[2] = CHANNEL2_PWM;
		replyBuf[1] = CHANNEL1_PWM;
		replyBuf[0] = CHANNEL0_PWM;
		return 4;
	}
	if (rq->bRequest == FUNC_PREPARE_COLOR_PARAM) {
		cmd_color_param_r = rq->wIndex.bytes[0];
		cmd_color_param_g = rq->wIndex.bytes[1];
		cmd_color_param_b = rq->wValue.bytes[0];
		cmd_color_param_w = rq->wValue.bytes[1];
		return 0;
	}
	if (rq->bRequest == FUNC_PREPARE_FADE_PARAM) {
		cmd_fade_duration = (rq->wValue.bytes[1]<<8) + rq->wValue.bytes[0];
		return 0;
	}
	if (rq->bRequest == FUNC_EXEC_SET_COLOR) {
		busid = rq->wIndex.bytes[0];
		deviceid = rq->wValue.bytes[0];
		/* check if we are meant */
		CHANNEL0_PWM = cmd_color_param_r;
		CHANNEL1_PWM = cmd_color_param_g;
		CHANNEL2_PWM = cmd_color_param_b;
		CHANNEL3_PWM = cmd_color_param_w;

		set_color(0, cmd_color_param_r);
		set_color(1, cmd_color_param_g);
		set_color(2, cmd_color_param_b);
		set_color(3, cmd_color_param_w);
		return 0;
	}
	if (rq->bRequest == FUNC_EXEC_FADETO_COLOR) {
		busid = rq->wIndex.bytes[0];
		deviceid = rq->wValue.bytes[0];
		/* check if we are meant */
		set_fade(0, cmd_color_param_r, cmd_fade_duration);
		set_fade(1, cmd_color_param_g, cmd_fade_duration);
		set_fade(2, cmd_color_param_b, cmd_fade_duration);
		set_fade(3, cmd_color_param_w, cmd_fade_duration);
		return 0;
	}
	return 0;
}
/* }}} */
#endif

void set_color(uint8_t channel, uint8_t colorvalue) { /* {{{ */
	global_pwm.channels[channel].target_brightness = colorvalue;
	global_pwm.channels[channel].brightness = colorvalue;
	global_pwm.channels[channel].flags.target_reached = 1;
	global_pwm.channels[channel].fade_steps_remaining = 0;
	global_pwm.channels[channel].fade_steps_pause = 0;
	global_pwm.channels[channel].fade_steps_pause_todo = 0;
	global_pwm.channels[channel].fade_step_change = 0;
	
}
/* }}} */

void set_fade(uint8_t channel, uint8_t colorvalue, uint16_t duration) { /* {{{ */
	int16_t diff;

	global_pwm.channels[channel].target_brightness = colorvalue;

	if (global_pwm.channels[channel].target_brightness == global_pwm.channels[channel].brightness) {
		/* no fade necessary, target already reached */
		global_pwm.channels[channel].flags.target_reached = 1;
	} else {
		if (duration == 0) {
			/* fade with zero duration: just set it */
			set_color(channel, colorvalue);
		} else {
			/* fade it */
			diff = abs(global_pwm.channels[channel].brightness - global_pwm.channels[channel].target_brightness);
			global_pwm.channels[channel].flags.target_reached = 0;
			global_pwm.channels[channel].fade_steps_remaining = diff;
			if (duration >= diff) {
				global_pwm.channels[channel].fade_steps_pause = (uint16_t) (duration / diff); //larger than 1, not rounded down
				if (global_pwm.channels[channel].target_brightness > global_pwm.channels[channel].brightness) {
					/* fade up */
					global_pwm.channels[channel].fade_step_change = 1;
				} else if (global_pwm.channels[channel].target_brightness < global_pwm.channels[channel].brightness) {
					/* fade down */
					global_pwm.channels[channel].fade_step_change = -1;
				}
			} else {
				global_pwm.channels[channel].fade_steps_pause = 1;
				if (global_pwm.channels[channel].target_brightness > global_pwm.channels[channel].brightness) {
					/* fade up */
					global_pwm.channels[channel].fade_step_change = (uint8_t) (diff / duration);
				} else if (global_pwm.channels[channel].target_brightness < global_pwm.channels[channel].brightness) {
					/* fade down */
					global_pwm.channels[channel].fade_step_change = -( (uint8_t) (diff / duration));
				}
			}
			global_pwm.channels[channel].fade_steps_pause_todo = global_pwm.channels[channel].fade_steps_pause;

		}
	}
}
/* }}} */



/** main function
 */
int main(void) { /* {{{ */
	reboot = 0;
	wdt_enable(WDTO_1S);

	init_output();

#if COLORFUL_INIT
	uint16_t j = 0;
	LED01_PORT |= _BV(LED_CHANNEL0);	//Switch On
	for (j = 0; j <= 10; j ++) {
		wdt_reset();
		_delay_ms(10);
	}
	LED01_PORT &= ~_BV(LED_CHANNEL0);	//Switch Off
	for (j = 0; j <= 17; j ++) {
		wdt_reset();
		_delay_ms(10);
	}
	LED01_PORT |= _BV(LED_CHANNEL1);	//Switch On
	for (j = 0; j <= 10; j ++) {
		wdt_reset();
		_delay_ms(10);
	}
	LED01_PORT &= ~_BV(LED_CHANNEL1);	//Switch Off
	for (j = 0; j <= 17; j ++) {
		wdt_reset();
		_delay_ms(10);
	}
	LED23_PORT |= _BV(LED_CHANNEL2);	//Switch On
	for (j = 0; j <= 10; j ++) {
		wdt_reset();
		_delay_ms(10);
	}
	LED23_PORT &= ~_BV(LED_CHANNEL2);	//Switch Off
	for (j = 0; j <= 17; j ++) {
		wdt_reset();
		_delay_ms(10);
	}
	LED23_PORT |= _BV(LED_CHANNEL3);	//Switch On
	for (j = 0; j <= 10; j ++) {
		wdt_reset();
		_delay_ms(10);
	}
	LED23_PORT &= ~_BV(LED_CHANNEL3);	//Switch Off
#endif

	init_pwm();

#if RC5_DECODER
	init_rc5();
#endif

#if STATIC_SCRIPTS
	init_script_threads();

	#if RS485_CTRL == 0
	/* start the example scripts */
	//script_threads[0].handler.execute = &memory_handler_flash;
	//script_threads[0].handler.position = (uint16_t) &blinken;
	//script_threads[0].flags.disabled = 0;
	
	script_threads[0].handler.execute = &memory_handler_flash;
	script_threads[0].handler.position = (uint16_t) &blinken;
	script_threads[0].flags.disabled = 0;
	#endif

#endif

#if RS485_CTRL
	/* init command bus */
	UCSR0A = _BV(MPCM0); /* enable multi-processor communication mode */
	UCSR0C = _BV(UCSZ00) | _BV(UCSZ01); /* 9 bit frame size */

	#define UART_UBRR 8 /* 115200 baud at 16mhz */
	UBRR0H = HIGH(UART_UBRR);
	UBRR0L = LOW(UART_UBRR);

	UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(UCSZ02); /* enable receiver and transmitter */
#endif

#if USB
	usbInit();
	usbDeviceDisconnect();	/* enforce re-enumeration, do this while interrupts are disabled! */
	uchar i = 0;
	while(--i){		/* fake USB disconnect for > 250 ms */
		_delay_ms(1);
	}
	usbDeviceConnect();
	//TCCR0 = 5;		/* set prescaler to 1/1024 */
#endif

	/* enable interrupts globally */
	sei();

	while (1) {
		wdt_reset();
#if USB
		usbPoll();
#endif
//		if (TIFR & (1 << TOV0)) {
//			TIFR |= 1 << TOV0;	/* clear pending flag */
//		}
		if (reboot) {
			soft_reset();
		}
		if (global.flags.new_cycle) {
			global.flags.new_cycle = 0;
			update_brightness();
#if STATIC_SCRIPTS
			execute_script_threads();
#endif
			continue;
		}
	}


#if RC5_DECODER
	/* check if we received something via ir */
	if (global_rc5.new_data) {
		static uint8_t toggle_bit = 2;

		/* if key has been pressed again */
		if (global_rc5.received_command.toggle_bit != toggle_bit) {

			/* if code is 0x01 (key '1' on a default remote) */
			if (global_rc5.received_command.code == 0x01) {

				/* install script into thread 1 */
				script_threads[1].handler.execute = &memory_handler_flash;
				script_threads[1].handler.position = (uint16_t) &green_flash;
				script_threads[1].flags.disabled = 0;
				script_threads[1].handler_stack_offset = 0;

			}

			/* store new toggle bit state */
			toggle_bit = global_rc5.received_command.toggle_bit;

		}

		/* reset the new_data flag, so that new commands can be received */
		global_rc5.new_data = 0;

		continue;
	}
#endif

#if RS485_CTRL
	if (UCSR0A & _BV(RXC0)) {

		uint8_t address = UCSR0B & _BV(RXB80); /* read nineth bit, zero if data, one if address */
		uint8_t data = UDR0;
		static uint8_t buffer[8];
		static uint8_t fill = 0;

		if (UCSR0A & _BV(MPCM0) || address) { /* if MPCM mode is still active, or ninth bit set, this is an address packet */

			/* check if we are ment */
			if (data == 0 || data == RS485_ADDRESS) {

				/* remove MPCM flag and reset buffer fill counter */
				UCSR0A &= ~_BV(MPCM0);
				fill = 0;

				continue;

			} else {/* turn on MPCM */

				UCSR0A |= _BV(MPCM0);
				continue;

			}
		}

		/* else this is a data packet, put data into buffer */
		buffer[fill++] = data;

		if (buffer[0] == 0x01) {	/* soft reset */

			jump_to_bootloader();

		} else if (buffer[0] == 0x02 && fill == 4) { /* set color */

			CHANNEL0_PWM = buffer[1];
			CHANNEL1_PWM = buffer[2];
			CHANNEL2_PWM = buffer[3];
			CHANNEL3_PWM = buffer[4];
			for (uint8_t pos = 0; pos < PWM_CHANNELS; pos++) {
				global_pwm.channels[pos].target_brightness = buffer[pos + 1];
				global_pwm.channels[pos].brightness = buffer[pos + 1];
			}

			UCSR0A |= _BV(MPCM0); /* return to MPCM mode */

		} else if (buffer[0] == 0x03 && fill == 6) { /* fade to color */

			for (uint8_t pos = 0; pos < PWM_CHANNELS; pos++) {
				global_pwm.channels[pos].speed_h = buffer[1];
				global_pwm.channels[pos].speed_l = buffer[2];
				global_pwm.channels[pos].target_brightness = buffer[pos + 3];
			}

			UCSR0A |= _BV(MPCM0); /* return to MPCM mode */
		}

	}
#endif
}
/* }}} */
