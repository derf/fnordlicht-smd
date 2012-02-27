CC	= gcc
CFLAGS	= $(USBFLAGS) -O -Wall
LIBS	= $(USBLIBS)

PROGRAM = Fnordlicht

all: $(PROGRAM)

.c.o:
	$(CC) $(CFLAGS) -c $<

$(PROGRAM): Fnordlicht.o
	$(CC) -o $(PROGRAM) Fnordlicht.o $(LIBS)

strip: $(PROGRAM)
	strip $(PROGRAM)

clean:
	rm -f *.o $(PROGRAM)
