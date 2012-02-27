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

#ifndef _FNORDLICHT_CONFIG_H
#define _FNORDLICHT_CONFIG_H


/*************************************************************
 * FEATURES                                                  *
 *************************************************************/

/* debug defines */
#ifndef DEBUG
#define DEBUG 0
#endif

/* include the script intpreter per default */
#ifndef STATIC_SCRIPTS
#define STATIC_SCRIPTS 0
#endif

/* disable rc5-decoder per default */
#ifndef RC5_DECODER
#define RC5_DECODER 0
#endif

/* disable scripts speed control per default */
#ifndef SCRIPT_SPEED_CONTROL
#define SCRIPT_SPEED_CONTROL 0
#endif

/* enable this if you want to control a fnordlicht via RS485 */
#ifndef RS485_CTRL
#define RS485_CTRL 0
#endif

/* test all LEDs on powerup */
#ifndef COLORFUL_INIT
#define COLORFUL_INIT 1
#endif

/* enable USB */
#ifndef USB
#define USB 1
#endif

/*************************************************************
 * PROTOCOL                                                  *
 *************************************************************/

/* USB */
#define FUNC_VERSION			0
#define FUNC_RESET			1
#define FUNC_ECHO			2
#define FUNC_ENUMERATE			3
#define FUNC_GET_STATUS			4
#define FUNC_PREPARE_COLOR_PARAM	5
#define FUNC_PREPARE_FADE_PARAM		6
#define FUNC_EXEC_SET_COLOR		7
#define FUNC_EXEC_FADETO_COLOR		8

/*************************************************************
 * HARDWARE                                                  *
 *************************************************************/

/* color <-> channel assignment */
/* Reihenfolge wie auf der Webseite (umgekehrt)
#define LED01_PORT PORTD
#define LED01_DDR DDRD
#define LED23_PORT PORTB
#define LED23_DDR DDRB

#define LED_CHANNEL0 PD5
#define LED_CHANNEL1 PD6
#define LED_CHANNEL2 PB1
#define LED_CHANNEL3 PB2

#define CHANNEL_RED     0
#define CHANNEL_GREEN   1
#define CHANNEL_BLUE    2
#define CHANNEL_WHITE   3
#define CHANNEL0_PWM	OCR0B
#define CHANNEL1_PWM	OCR0A
#define CHANNEL2_PWM	OCR1AL
#define CHANNEL3_PWM	OCR1BL */

/* Reihenfolge wie in der Anleitung */
#define LED23_PORT PORTD
#define LED23_DDR DDRD
#define LED01_PORT PORTB
#define LED01_DDR DDRB

#define LED_CHANNEL3 PD5
#define LED_CHANNEL2 PD6
#define LED_CHANNEL1 PB1
#define LED_CHANNEL0 PB2

#define CHANNEL_RED     3
#define CHANNEL_GREEN   2
#define CHANNEL_BLUE    1
#define CHANNEL_WHITE   0
#define CHANNEL3_PWM	OCR0B
#define CHANNEL2_PWM	OCR0A
#define CHANNEL1_PWM	OCR1AL
#define CHANNEL0_PWM	OCR1BL




#include <avr/version.h>

/* check for avr-libc version */
#if __AVR_LIBC_VERSION__ < 10402UL
#error newer libc version (>= 1.4.2) needed!
#endif

/* check if cpu speed is defined */
#ifndef F_CPU
#error please define F_CPU!
#endif

/* check if this cpu is supported */
#if !(defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__))
#error "this cpu isn't supported yet!"
#endif

/* cpu specific configuration registers */
#if defined(__AVR_ATmega8__)
/* {{{ */
#define _ATMEGA8

#define _TIMSK_TIMER1 TIMSK
#define _UCSRB_UART0 UCSRB
#define _UDRIE_UART0 UDRIE
#define _TXEN_UART0 TXEN
#define _RXEN_UART0 RXEN
#define _RXCIE_UART0 RXCIE
#define _UBRRH_UART0 UBRRH
#define _UBRRL_UART0 UBRRL
#define _UCSRC_UART0 UCSRC
#define _UCSZ0_UART0 UCSZ0
#define _UCSZ1_UART0 UCSZ1
#define _SIG_UART_RECV_UART0 SIG_UART_RECV
#define _SIG_UART_DATA_UART0 SIG_UART_DATA
#define _UDR_UART0 UDR
#define UCSR0A UCSRA
#define UCSR0C UCSRC
#define MPCM0 MPCM
#define UCSZ00 UCSZ0
#define UCSZ01 UCSZ1
#define UCSZ02 UCSZ2
#define UBRR0H UBRRH
#define UBRR0L UBRRL
#define UCSR0B UCSRB
#define RXEN0 RXEN
#define TXEN0 TXEN
#define RXC0 RXC
#define RXB80 RXB8
#define UDR0 UDR

/* }}} */
#elif defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__)
/* {{{ */
#define _ATMEGA88

#define _TIMSK_TIMER1 TIMSK1
#define _UCSRB_UART0 UCSR0B
#define _UDRIE_UART0 UDRIE0
#define _TXEN_UART0 TXEN0
#define _RXEN_UART0 RXEN0
#define _RXCIE_UART0 RXCIE0
#define _UBRRH_UART0 UBRR0H
#define _UBRRL_UART0 UBRR0L
#define _UCSRC_UART0 UCSR0C
#define _UCSZ0_UART0 UCSZ00
#define _UCSZ1_UART0 UCSZ01
#define _SIG_UART_RECV_UART0 SIG_USART_RECV
#define _SIG_UART_DATA_UART0 SIG_USART_DATA
#define _UDR_UART0 UDR0
#define TCCR0   TCCR0B
#define TIFR    TIFR0
/* }}} */
#endif

#endif /* _FNORDLICHT_CONFIG_H */
