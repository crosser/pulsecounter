#include "Pulsecounter.h"
#include "Hal.h"

static void gpioHandler(uint8_t id);
static void tickHandler(void);
static int32_t cold = 0;
static int32_t hot  = 0;
static bool connected = false;

void main() {
    Hal_init();
    Hal_gpioEnable(gpioHandler);
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

static void gpioHandler(uint8_t id) {
    uint8_t i;

    switch (id) {
    case 0:
        /* Pulsecounter_accept(true); */
        if (connected) {
            Pulsecounter_coldTick_indicate();
            Hal_delay(100);
            Pulsecounter_hotTick_indicate();
        }
        blink(3, 2);
        Hal_tickStart(15000, tickHandler);
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

static void tickHandler(void) {
    uint8_t i;

    Hal_tickStop();
    if (connected)
        return;
    for (i = 0; i < 3; i++) {
        Hal_greenLedOn();
        Hal_delay(50);
        Hal_redLedOn();
        Hal_delay(50);
        Hal_redLedOff();
        Hal_delay(50);
        Hal_greenLedOff();
    }
    /* Pulsecounter_accept(false); */
}

/* -------- SCHEMA CALLBACKS -------- */

void Pulsecounter_connectHandler(void) {
    connected = true;
    Hal_tickStop();
    blink(1, 5);
}

void Pulsecounter_disconnectHandler(void) {
    connected = false;
    /* Hal_tickStart(15000, tickHandler); */
    Hal_disconnected();
    blink(2, 5);
}

void Pulsecounter_coldTick_fetch(Pulsecounter_coldTick_t* const output) {
    *output = cold;
}

void Pulsecounter_hotTick_fetch(Pulsecounter_hotTick_t* const output) {
    *output = hot;
}
