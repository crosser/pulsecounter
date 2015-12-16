CFLAGS += -I$(BLUEZ) `pkg-config --cflags glib-2.0`
LDLIBS = $(BLUEZ)/lib/.libs/libbluetooth-internal.a \
	$(BLUEZ)/src/.libs/libshared-glib.a \
	`pkg-config --libs glib-2.0`

all: pulsecounter

pulsecounter: pulsecounter.o \
	$(BLUEZ)/attrib/att.o $(BLUEZ)/attrib/gatt.o \
	$(BLUEZ)/attrib/gattrib.o $(BLUEZ)/btio/btio.o \
	$(BLUEZ)/attrib/utils.o $(BLUEZ)/src/log.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@


#	$(BLUEZ)/attrib/interactive.o \
#	$(BLUEZ)/client/display.o
