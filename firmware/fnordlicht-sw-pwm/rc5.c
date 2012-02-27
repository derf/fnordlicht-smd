/* vim:fdm=marker ts=4 et ai
 * {{{
 *         fnordlicht firmware next generation
 *
 *    for additional information please
 *    see http://koeln.ccc.de/prozesse/running/fnordlicht
 *
 * (c) by Alexander Neumann <alexander@bumpern.de>
 *     Lars Noschinski <lars@public.noschinski.de>
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


#include "config.h"

#if RC5_DECODER

#include "rc5.h"
#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/* global variables */
volatile struct global_rc5_t global_rc5;

/* the rc5 bit length (= duration for one bit, encoded in rc5) is 1.8ms
 * (see atmel avr application note AVR410)
 * RC5_BIT_TICKS is the amount of timer ticks for one complete bit
 * RC5_HALF_BIT_TICKS is amount of timer ticks for half a bit
 *
 * (for cpu speed F_CPU and prescaler 1024) */

#define RC5_BIT_TICKS ((uint8_t)(F_CPU / 1024 * 1800 / 1000000))
#define RC5_HALF_BIT_TICKS ((uint8_t)(RC5_BIT_TICKS/2))

/* define maximal environment for detecting valid rc5 pulses */
#define RC5_ENVIRONMENT ((uint8_t)(RC5_BIT_TICKS / 5))


/*
 * Overview:
 * {{{
 * An rc5 word is composed of 14 bits:  SSTAAAAACCCCCC
 *   - two start bits (always 1)
 *   - one toggle bit (changes value a key is released and pressed again)
 *   - five address bits
 *   - six command bits
 *
 * The signal we actually decode is this 14-bit-word encoded in inverted manchester code.
 *
 *                          +--             --+
 * a one is encoded as:   __|   , inverted:   |__
 *
 *                        --+                 +--
 * a zero is encoded as:    |__ , inverted: __|
 *
 * which means the data signal (already inverted) for the rc5 word
 * 11000010000001 (address 0x02, command 0x01, toggle bit 0) is:
 *
 * bit          S     S     T     A     A     A     A     A     C     C     C     C     C     C
 * value     |  1  |  1  |  0  |  0  |  0  |  0  |  1  |  0  |  0  |  0  |  0  |  0  |  0  |  1  |
 *
 * signal   ----+  +--+     +--+  +--+  +--+  +-----+     +--+  +--+  +--+  +--+  +--+  +-----+  +--
 *              |__|  |_____|  |__|  |__|  |__|     |_____|  |__|  |__|  |__|  |__|  |__|     |__|
 * halfbit-
 * counter         1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27
 *
 *
 * Another example: 11100010000010 (address 0x02, command 0x02, toggle bit 1) is:
 *
 * bit          S     S     T     A     A     A     A     A     C     C     C     C     C     C
 * value     |  1  |  1  |  1  |  0  |  0  |  0  |  1  |  0  |  0  |  0  |  0  |  0  |  1  |  0  |
 *
 * signal   ----+  +--+  +--+     +--+  +--+  +-----+     +--+  +--+  +--+  +--+  +-----+     +-----
 *              |__|  |__|  |_____|  |__|  |__|     |_____|  |__|  |__|  |__|  |__|     |_____|
 * halfbit-
 * counter         1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27
 *
 *
 * Observation #1: After 27 halfbits we are done in any case. If the last bit
 *                 is a 0, we get the last pin-change interrupt at halfbit 26,
 *                 this is a special case.  In the timeout handling function
 *                 which is called after the last interrupt, we have to check
 *                 if we received 26 or 27 halfbits.  If only 26 halfbits have
 *                 been received, we have to add a zero to the decoded bit-stream.
 *
 * Observation #2: Within a sequence of two zeroes or two ones, an odd number
 *                 of pin-change interrupts occur, and a sequence of a one and
 *                 a zero (the order doesn't matter), contains an even number
 *                 of interrupts:
 *
 *                 |  1  |  1  |        |  0  |  0  |
 *
 *                 ---+  +--+              +--+  +--      3 pin-chages
 *                    |__|  |__         ___|  |__|
 *
 *
 *                 |  1  |  0  |        |  0  |  1  |
 *                                                        2 pin-changes
 *                 ---+     +---           +-----+
 *                    |_____|           ___|     |___
 *
 *
 *                 Conclusion: Every time the bit-sequence alternates between
 *                 zero and one, the parity on the number of pin-change
 *                 interrupts changes.  We can use this to determine, if the
 *                 last transmitted bit was a one or a zero, by just checking
 *                 the parity of the number of interrupts which occured before
 *                 the current one, every second halfbit (which actually means
 *                 we check after each transmitted bit).
 *
 *                 In the beginning, after the halfbit 1 (actually the second,
 *                 but since the first bit is a one, the signal level isn't
 *                 changed and we don't get an interrupt), we know a one has
 *                 transmitted. Therefore if the parity is even, a change in
 *                 the bit-sequence has happenend and the last bit was a zero,
 *                 if the parity is odd another change (or no change) happened,
 *                 the last bit was a one.
 *
 *                 Examples (from above):
 *
 *
 * bit          S     S     T     A     A     A     A     A     C     C     C     C     C     C
 * value     |  1  |  1  |  0  |  0  |  0  |  0  |  1  |  0  |  0  |  0  |  0  |  0  |  0  |  1  |
 *
 * signal   ----+  +--+     +--+  +--+  +--+  +-----+     +--+  +--+  +--+  +--+  +--+  +-----+  +--
 *              |__|  |_____|  |__|  |__|  |__|     |_____|  |__|  |__|  |__|  |__|  |__|     |__|
 * halfbit-
 * counter      0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27
 *
 * interrupt-   0  1  2  3  3  4  5  6  7  8  9  10 10 11 11 12 13 14 15 16 17 18 19 20 21 22 22 23
 * counter         |     |     |     |     |     |     |     |     |     |     |     |     |     |
 *                 v     v     v     v     v     v     v     v     v     v     v     v     v     v
 *
 * received        1     1     0     0     0     0     1     0     0     0     0     0     0     1
 * bit
 *
 *
 * Another example: 11100010000010 (address 0x02, command 0x02, toggle bit 1) is:
 *
 * bit          S     S     T     A     A     A     A     A     C     C     C     C     C     C
 * value     |  1  |  1  |  1  |  0  |  0  |  0  |  1  |  0  |  0  |  0  |  0  |  0  |  1  |  0  |
 *
 * signal   ----+  +--+  +--+     +--+  +--+  +-----+     +--+  +--+  +--+  +--+  +-----+     +-----
 *              |__|  |__|  |_____|  |__|  |__|     |_____|  |__|  |__|  |__|  |__|     |_____|
 * halfbit-
 * counter      0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27
 *
 * interrupt-   0  1  2  3  4  5  5  6  7  8  9  10 10 11 11 12 13 14 15 16 17 18 19 20 20 21 22 24
 * counter         |     |     |     |     |     |     |     |     |     |     |     |     |     |
 *                 v     v     v     v     v     v     v     v     v     v     v     v     v     v
 *
 * received        1     1     1     0     0     0     1     0     0     0     0     0     1     0
 * bit
 *
 *
 *
 *
 * General Roadmap: 1) Interrupt on the first falling edge, activate and reset a
 *                     timer, reset halfbit-counter and interrupt-counter, switch
 *                     interrupt to fire at any logical change, enable timer
 *                     overflow interrupt.
 *                  2) In external interrupt, check if one or two halfbits have
 *                     passed between this and the last interrupt by checking the
 *                     timer (using RC5_BIT_TICKS/RC5_HALF_TICKS and RC5_ENVIRONMENT),
 *                     process each detected halfbit, increment the
 *                     interrupt-counter afterwards.
 *                  3) In timer overflow interrupt (timeout), check if 26 (add a zero)
 *                     or 27 halfbits have been received, copy output, reconfigure
 *                     interrupt to fire at the falling edge again, disable timer
 *                     overflow interrupt, copy output to global public structure.
 *
 * }}}
 */


/* local variables */
static uint8_t rc5_halfbitcount;
static uint8_t rc5_interrupts;
static uint8_t rc5_disabled;
/* temp command buffer */
static struct rc5_t temp_rc5;

/* init rc5 decoder */
void init_rc5(void)
/* {{{ */ {

    /* init halfbit and interrupt counter */
    rc5_halfbitcount = 0;
    rc5_interrupts = 0;

    /* reset disable flag */
    rc5_disabled = 0;

    /* init global structures */
    global_rc5.enabled = 1;
    global_rc5.new_data = 0;
    global_rc5.received_command.raw = 0;

    /* enable timer0, set prescaler and enabled overflow interrupt */
    TCCR0 = _BV(CS02) | _BV(CS00);

    /* configure int0 to fire at any logical change */
    MCUCR |= _BV(ISC00);
    MCUCR &= ~_BV(ISC01);

    /* clear any old interrupts and enable int0 interrupt */
    GIFR = _BV(INTF0);
    GICR |= _BV(INT0);
} /* }}} */


/* int0 interrupt */
ISR(SIG_INTERRUPT0)
/* {{{ */ {

    if (global_rc5.enabled && !rc5_disabled) {

        /* if this is the first interrupt */
        if (rc5_interrupts == 0) {
            /* reset counter, clear old overflows and enable
             * timer0 overflow interrupt */
            TCNT0 = 0;
            TIFR = _BV(TOV0);
            TIMSK |= _BV(TOIE0);

            /* reset temp buffer */
            temp_rc5.raw = 0;

        /* if this is not the first interrupt */
        } else {
            /* load and reset the counter */
            uint8_t counter = TCNT0;
            TCNT0 = 0;

            /* check how many halfbits have passed since last interrupt */
            uint8_t received_bits = 0;

            /* check for two halfbits */
            if ( (counter > RC5_HALF_BIT_TICKS - RC5_ENVIRONMENT &&
                  counter < RC5_HALF_BIT_TICKS + RC5_ENVIRONMENT) ) {
                received_bits = 1;

            /* check for one halfbit */
            } else if ( (counter > RC5_BIT_TICKS - RC5_ENVIRONMENT &&
                         counter < RC5_BIT_TICKS + RC5_ENVIRONMENT) ) {
                received_bits = 2;

            /* else signal is invalid */
            } else {
                /* disable interrupt, enable timer overflow interrupt: -> timeout */
                //GICR &= ~_BV(INT0);
                rc5_disabled = 1;

                /* reset bitcounter */
                rc5_halfbitcount = 0;

                /* quit this interrupt and wait for timeout */
                return;
            }

            /* process each received halfbit */
            while (received_bits--) {
                /* increase halfbit counter, since we obviously
                 * received a halfbit */
                rc5_halfbitcount++;

                /* if the parity of the halfbit-counter is odd,
                 * we have received a complete bit,
                 * so save this bit to the structure*/

                if (rc5_halfbitcount & 1) {
                    /* shift buffer left, and set lsb, if the
                     * parity of the interrupt counter (interrupts
                     * which happened _BEFORE_ this one (=postincrement))
                     * is odd*/
                    temp_rc5.raw <<= 1;
                    temp_rc5.raw |= (rc5_interrupts & 1);
                }
            }
        }

        /* increase interrupt counter */
        rc5_interrupts++;
    }
} /* }}} */

/* timer0 overflow interrupt */
ISR(SIG_OVERFLOW0)
/* {{{ */ {
    /* disable overflow interrupt */
    TIMSK &= ~_BV(TOIE0);

    /* check if we only received 26 halfbits,
     * so the last transmitted bit was zero,
     * if 27 halfbits have been received, everything
     * went fine */
    switch (rc5_halfbitcount) {
        case 26:
                 /* add a zero */
                 temp_rc5.raw <<= 1;
                 /* and fall-through to the next case */

        case 27:
                 /* copy data to global structure */
                 global_rc5.received_command.raw = temp_rc5.raw;

                 /* signal main that new data has arrived */
                 global_rc5.new_data = 1;
    }

    /* reset counter */
    rc5_halfbitcount = 0;
    rc5_interrupts = 0;

    /* if decoder is enabled, reconfigure int0 */
    if (global_rc5.enabled) {

        /* re-enable int0 */
        rc5_disabled = 0;

    }

} /* }}} */

#endif
