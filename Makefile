GCCARCH = msp430
MCU = msp430g2553
COMMAND_PREFIX = $(GCCARCH)-
CC = $(COMMAND_PREFIX)gcc
LD = $(COMMAND_PREFIX)ld
UPLOAD = mspdebug rf2500
EMBUILDER = em-builder

APPNAME = Pulsecounter
MAIN = $(APPNAME)-Prog
OUTFILE = $(MAIN).out
OBJECTS = $(MAIN).o $(APPNAME).o Hal.o

COPTS = -mmcu=$(MCU)
LDOPTS = -mmcu=$(MCU) -Wl,-Map=$(MAIN).map,--gc-sections
CFLAGS = -std=gnu99 -O2 -w -ffunction-sections -fdata-sections \
	-fpack-struct=1 -fno-strict-aliasing -fomit-frame-pointer \
	-c -g -IHal -IEm $(COPTS)

all: $(OUTFILE)

load: $(OUTFILE)
	$(UPLOAD) "prog $(OUTFILE)"

clean:
	rm -f $(OUTFILE) $(OBJECTS)

em-clean: clean
	rm -rf Em

$(OUTFILE): $(OBJECTS)
	$(CC) -o $(OUTFILE) $^ $(LDOPTS)

#.c.o:
#	$(CC) $< -o $@ $(CFLAGS)

$(MAIN).o: $(MAIN).c Em/$(APPNAME).c
	$(CC) $< -o $@ $(CFLAGS)

$(APPNAME).o: Em/$(APPNAME).c
	$(CC) $< -o $@ $(CFLAGS)

Hal.o: Hal/Hal.c
	$(CC) $< -o $@ $(CFLAGS)

Em/$(APPNAME).c: $(APPNAME).ems
	$(EMBUILDER) -v $<

