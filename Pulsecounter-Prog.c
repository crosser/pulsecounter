#include "Pulsecounter.h"
#include "Hal.h"

static void gpioHandler(uint8_t id);
static void tickHandler(void);
static bool connected = false;
static int32_t base4 = 0;
static int32_t base5 = 0;
static int32_t event4 = 0;
static int32_t event5 = 0;

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
        Pulsecounter_accept(true);
            Hal_greenLedOn();
            Hal_redLedOn();
            Hal_delay(10);
            Hal_greenLedOff();
            Hal_redLedOff();
        Hal_tickStart(5000, tickHandler);
        break;
    case 1:
        event4++;
        if (connected)
            Pulsecounter_event4_indicate();
        Hal_greenLedOn();
        Hal_delay(10);
        Hal_greenLedOff();
        break;
    case 2:
        event5++;
        if (connected)
            Pulsecounter_event5_indicate();
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
    Pulsecounter_accept(false);
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
    Hal_greenLedOn();
    Hal_delay(100);
    Hal_greenLedOff();
    Hal_redLedOn();
    Hal_delay(100);
    Hal_redLedOff();
    Hal_tickStart(5000, tickHandler);
    Hal_disconnected();
}

void Pulsecounter_event4_fetch(Pulsecounter_event4_t* const output) {
    *output = base4 + event4;
}

void Pulsecounter_event5_fetch(Pulsecounter_event5_t* const output) {
    *output = base5 + event5;
}

void Pulsecounter_base4_fetch(Pulsecounter_base4_t* const output) {
    *output = base4;
}

void Pulsecounter_base4_store(Pulsecounter_base4_t* const input) {
    base4 = *input - event4;
}

void Pulsecounter_base5_fetch(Pulsecounter_base5_t* const output) {
    *output = base5;
}

void Pulsecounter_base5_store(Pulsecounter_base5_t* const input) {
    base5 = *input - event5;
}
