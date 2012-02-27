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

#include "common.h"
#include "fnordlicht.h"
#include "pwm.h"

/* TYPES AND PROTOTYPES */
volatile struct global_pwm_t global_pwm;

/* FUNCTIONS AND INTERRUPTS */

/** init timer 1 */
inline void init_timer(void)
/*{{{*/ {
	/* Timer for PWM of LEDs */
	TCCR0A = _BV(WGM00) | _BV(COM0A1) | _BV(COM0B1);
	TCCR0B = _BV(CS00) | _BV(CS01);
	TCCR1A = _BV(WGM10) | _BV(COM1A1) | _BV(COM1B1);
	TCCR1B = _BV(CS10) | _BV(CS11);
	TIMSK0 = _BV(TOIE0);
}

/* }}} */

/** init pwm */
inline void init_pwm(void)
/*{{{*/ {
	uint8_t i;

	init_timer();

	CHANNEL0_PWM = 0;
	CHANNEL1_PWM = 0;
	CHANNEL2_PWM = 0;
	CHANNEL3_PWM = 0;

	for (i=0; i<PWM_CHANNELS; i++) {
		global_pwm.channels[i].brightness = 0;
		global_pwm.channels[i].target_brightness = 0;
		global_pwm.channels[i].flags.target_reached = 1;
		global_pwm.channels[i].fade_steps_remaining = 0;
		global_pwm.channels[i].fade_steps_pause = 0;
		global_pwm.channels[i].fade_steps_pause_todo = 0;
		global_pwm.channels[i].fade_step_change = 0;
	}
}
/* }}} */

/** fade any channels not already at their target brightness */
void update_brightness(void)
/* {{{ */ {
	uint8_t i;

	/* iterate over the channels */
	for (i=0; i<PWM_CHANNELS; i++) {
		
		/* fade channel if not already at target brightness */
		if (global_pwm.channels[i].flags.target_reached == 0) {

			/* wait to make the fade last as long as specified */
			global_pwm.channels[i].fade_steps_pause_todo--;
			if (global_pwm.channels[i].fade_steps_pause_todo == 0) {
				global_pwm.channels[i].fade_steps_pause_todo = global_pwm.channels[i].fade_steps_pause;

				global_pwm.channels[i].fade_steps_remaining--;
				if (global_pwm.channels[i].fade_steps_remaining > 0) {
					if ((global_pwm.channels[i].fade_step_change > 0) && (255 - global_pwm.channels[i].fade_step_change < global_pwm.channels[i].brightness)) {
						/* prevent overshoot */
						set_brightness(i, global_pwm.channels[i].target_brightness);
						global_pwm.channels[i].brightness = global_pwm.channels[i].target_brightness;
						global_pwm.channels[i].flags.target_reached = 1;
					} else if ((global_pwm.channels[i].fade_step_change < 0) && (-global_pwm.channels[i].fade_step_change > global_pwm.channels[i].brightness)) {
						/* prevent overshoot */
						set_brightness(i, global_pwm.channels[i].target_brightness);
						global_pwm.channels[i].brightness = global_pwm.channels[i].target_brightness;
						global_pwm.channels[i].flags.target_reached = 1;
					} else {
						/* fade normal */
						global_pwm.channels[i].brightness += global_pwm.channels[i].fade_step_change;
						add_brightness(i, global_pwm.channels[i].fade_step_change);
					}
				} else {
					/* target brightness has been reached, set flag */
					set_brightness(i, global_pwm.channels[i].target_brightness);
					global_pwm.channels[i].brightness = global_pwm.channels[i].target_brightness;
					global_pwm.channels[i].flags.target_reached = 1;
				}
			}
		}
	}
} /* }}} */

void set_brightness(uint8_t channel, uint8_t brightness) { /* {{{ */
	if (channel==0) {
		CHANNEL0_PWM = brightness;
	} else if (channel==1) {
		CHANNEL1_PWM = brightness;
	} else if (channel==2) {
		CHANNEL2_PWM = brightness;
	} else if (channel==3) {
		CHANNEL3_PWM = brightness;
	}
}
/* }}} */

void add_brightness(uint8_t channel, uint8_t addbrightness) { /* {{{ */
	if (channel==0) {
		CHANNEL0_PWM += addbrightness;
	} else if (channel==1) {
		CHANNEL1_PWM += addbrightness;
	} else if (channel==2) {
		CHANNEL2_PWM += addbrightness;
	} else if (channel==3) {
		CHANNEL3_PWM += addbrightness;
	}
}
/* }}} */

/** timer0 overflow interrupt */
ISR(SIG_OVERFLOW0) { /* {{{ */
	static uint8_t count = 0;

	if (count++ == 3) {
		/* signal new cycle to main procedure */
		global.flags.new_cycle = 1;
		count = 0;
	}
} /*}}}*/
