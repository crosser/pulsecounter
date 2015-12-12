#include "Pulsecounter.h"
#include "Hal.h"

static void gpioHandler(uint8_t id);
static void tickHandler(void);
static int32_t cold = 0;
static int32_t hot  = 0;
static bool connected = false;
static bool updatable = false;

void main() {
    Hal_init();
    Hal_gpioEnable(gpioHandler);
    Pulsecounter_setDeviceName("PULS-CNTR");
    Pulsecounter_start();
    Hal_idleLoop();
}

static void gpioHandler(uint8_t id) {
    uint8_t i;

    switch (id) {
    case 0:
        /* Pulsecounter_accept(true); */
        updatable = true;
        if (connected) {
            Pulsecounter_coldTick_indicate();
            Pulsecounter_hotTick_indicate();
        }
        Hal_greenLedOn();
        Hal_redLedOn();
        Hal_delay(10);
        Hal_greenLedOff();
        Hal_redLedOff();
        Hal_tickStart(15000, tickHandler);
        break;
    case 1:
        cold++;
        if (connected)
            Pulsecounter_coldTick_indicate();
        Hal_greenLedOn();
        Hal_delay(10);
        Hal_greenLedOff();
        break;
    case 2:
        hot++;
        if (connected)
            Pulsecounter_hotTick_indicate();
        Hal_redLedOn();
        Hal_delay(10);
        Hal_redLedOff();
        break;
    default:
        for (i = 0; i < 5; i++) {
            Hal_greenLedOn();
            Hal_redLedOn();
            Hal_delay(10);
            Hal_greenLedOff();
            Hal_redLedOff();
            Hal_delay(10);
        }
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
    updatable = false;
    /* Pulsecounter_accept(false); */
}

/* -------- SCHEMA CALLBACKS -------- */

void Pulsecounter_connectHandler(void) {
    connected = true;
    Hal_connected();
    Hal_redLedOn();
    Hal_delay(100);
    Hal_redLedOff();
    Hal_greenLedOn();
    Hal_delay(100);
    Hal_greenLedOff();
}

void Pulsecounter_disconnectHandler(void) {
    connected = false;
    updatable = false;
    Hal_greenLedOn();
    Hal_delay(100);
    Hal_greenLedOff();
    Hal_redLedOn();
    Hal_delay(100);
    Hal_redLedOff();
    /* Hal_tickStart(15000, tickHandler); */
    Hal_disconnected();
}

void Pulsecounter_coldTick_fetch(Pulsecounter_coldTick_t* const output) {
    *output = cold;
}

void Pulsecounter_hotTick_fetch(Pulsecounter_hotTick_t* const output) {
    *output = hot;
}

void Pulsecounter_coldSet_store(Pulsecounter_coldSet_t* const input) {
    if (updatable)
        cold = *input;
}

void Pulsecounter_hotSet_store(Pulsecounter_hotSet_t* const input) {
    if (updatable)
        hot = *input;
}
