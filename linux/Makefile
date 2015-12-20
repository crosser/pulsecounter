CFLAGS += -g -I$(BLUEZ) `pkg-config --cflags glib-2.0` \
	`pkg-config mariadb --cflags`
LDLIBS = $(BLUEZ)/lib/.libs/libbluetooth-internal.a \
	$(BLUEZ)/src/.libs/libshared-glib.a \
	`pkg-config --libs glib-2.0` \
	`pkg-config mariadb --libs`

all: pulsecounter

pulsecounter.o: pulsecounter.c dbstore.h

dbstore.o: dbstore.c dbstore.h

pulsecounter: pulsecounter.o dbstore.o \
	$(BLUEZ)/attrib/att.o $(BLUEZ)/attrib/gatt.o \
	$(BLUEZ)/attrib/gattrib.o $(BLUEZ)/btio/btio.o \
	$(BLUEZ)/attrib/utils.o $(BLUEZ)/src/log.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

testconf: dbstore.c dbstore.h
	$(CC) -g `pkg-config --cflags glib-2.0` `pkg-config mariadb --cflags` \
		-DTEST_CONFIG $< `pkg-config --libs glib-2.0` \
		`pkg-config mariadb --libs` -o $@
