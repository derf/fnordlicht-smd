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


#ifndef PWM_H
#define PWM_H

/* possible pwm interrupts in a pwm cycle */
#define PWM_MAX_TIMESLOTS (PWM_CHANNELS+1)

/* contains all the data for one color channel */
struct channel_t
/*{{{*/ {
	/* brightness of channel */
	uint8_t brightness;

	/* desired brightness for this channel */
	uint8_t target_brightness;

	/* sleep between steps */
	uint16_t fade_steps_pause;

	/* part of the pause that still has to be slept */
	uint16_t fade_steps_pause_todo;

	/* steps left to make */
	uint8_t fade_steps_remaining;

	/* value to add/subtract on each step */
	int8_t fade_step_change;

	/* flags for this channel, implemented as a bitvector field */
	struct {
		/* this channel reached has recently reached it's desired target brightness */
		uint8_t target_reached:1;
	} flags;

}; /*}}}*/

struct global_pwm_t {
	/* current channel records */
	struct channel_t channels[PWM_CHANNELS];
};

extern volatile struct global_pwm_t global_pwm;

/* prototypes */
void init_timer(void);
void init_pwm(void);
void update_brightness(void);

void set_brightness(uint8_t channel, uint8_t brightness);
void add_brightness(uint8_t channel, uint8_t addbrightness);

#endif
