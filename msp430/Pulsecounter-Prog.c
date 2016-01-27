#include "Pulsecounter.h"
#include "Hal.h"

static void settleHandler(uint8_t id);
static void jitterHandler(uint8_t id, uint32_t count);
static int32_t cold = 0;
static int32_t hot  = 0;
static int32_t coldJ = 0;
static int32_t hotJ  = 0;
static bool connected = false;

void main() {
    Hal_init();
    Hal_gpioEnable(settleHandler, jitterHandler);
    Pulsecounter_setDeviceName("PULS-CNTR");
    Pulsecounter_start();
    Hal_idleLoop();
}

static void blink(uint8_t which, uint8_t count) {
    uint8_t i;
    for (i = 0; i < count; i++) {
        if (i) Hal_delay(50);
        if (which & 1) Hal_greenLedOn();
        if (which & 2) Hal_redLedOn();
        Hal_delay(50);
        if (which & 1) Hal_greenLedOff();
        if (which & 2) Hal_redLedOff();
    }
}

static void settleHandler(uint8_t id) {
    switch (id) {
    case 0:
        /* Pulsecounter_accept(true); */
        if (connected) {
            Pulsecounter_coldTick_indicate();
            Hal_delay(100);
            Pulsecounter_hotTick_indicate();
        }
        blink(3, 2);
        break;
    case 1:
        cold++;
        if (connected)
            Pulsecounter_coldTick_indicate();
        blink(1, 2);
        break;
    case 2:
        hot++;
        if (connected)
            Pulsecounter_hotTick_indicate();
        blink(2, 2);
        break;
    default:
        blink(3, 15);
    }
}

static void jitterHandler(uint8_t id, uint32_t count) {
    switch (id) {
    case 0:
        blink(3, count);
        break;
    case 1:
        coldJ = count;
        if (connected)
            Pulsecounter_coldJitter_indicate();
        blink(1, 1);
        break;
    case 2:
        hotJ = count;
        if (connected)
            Pulsecounter_hotJitter_indicate();
        blink(2, 1);
        break;
    default:
        blink(3, 13);
    }
}

/* -------- SCHEMA CALLBACKS -------- */

void Pulsecounter_connectHandler(void) {
    connected = true;
    blink(1, 5);
}

void Pulsecounter_disconnectHandler(void) {
    connected = false;
    blink(2, 5);
}

void Pulsecounter_coldTick_fetch(Pulsecounter_coldTick_t* const output) {
    *output = cold;
}

void Pulsecounter_hotTick_fetch(Pulsecounter_hotTick_t* const output) {
    *output = hot;
}

void Pulsecounter_coldJitter_fetch(Pulsecounter_coldJitter_t* output) {
    *output = coldJ;
}

void Pulsecounter_hotJitter_fetch(Pulsecounter_hotJitter_t* output) {
    *output = hotJ;
}
