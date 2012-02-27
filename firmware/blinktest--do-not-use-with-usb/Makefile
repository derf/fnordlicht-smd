# ======================================================================
#
# modified by Timo Boettcher
#
# This is free software, licensed under the terms of the GNU General
# Public License as published by the Free Software Foundation.
# ======================================================================

USBTINY		= ../usbtiny
TARGET_ARCH	= -mmcu=atmega88
OBJECTS		= main.o
FLASH_CMD	= avrdude -pm88 -U flash:w:main.hex


CC	= avr-gcc
CFLAGS	= -Os -g -Wall -I. -I$(USBTINY)
ASFLAGS	= -Os -g -Wall -I.
LDFLAGS	= -g
MODULES = $(OBJECTS)
UTIL	= $(USBTINY)/../util

main.hex:

all:		main.hex

clean:
	rm -f main.elf *.o

clobber:	clean
	rm -f main.hex

main.elf:	$(MODULES)
	$(LINK.o) -o $@ $(MODULES)

main.hex:	main.elf 
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex

disasm:		main.elf
	avr-objdump -S main.elf

flash:		main.hex
	$(FLASH_CMD)
