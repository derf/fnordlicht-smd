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


#define soft_reset()\
do {\
	wdt_enable(WDTO_15MS);\
	for(;;) {}\
} while(0)

/* global flag(=bit) structure */
struct flags_t {
	/* set by pwm interrupt after burst, signals the beginning of a new pwm
	 * cycle to the main loop. */
	uint8_t new_cycle:1;
};

struct global_t {
	struct flags_t flags;
};

extern volatile struct global_t global;

void set_color(uint8_t channel, uint8_t colorvalue);
void set_fade(uint8_t channel, uint8_t colorvalue, uint16_t duration);

uint8_t reboot;

/* keep state for usb-commands (controlmsg can send only 5bytes) */
uint8_t cmd_color_param_r;
uint8_t cmd_color_param_g;
uint8_t cmd_color_param_b;
uint8_t cmd_color_param_w;

uint16_t cmd_fade_duration;

uint8_t busid;
uint8_t deviceid;

