#include "Pulsecounter.h"
#include "Hal.h"

static void buttonHandler(void);
static void tickHandler(void);
static bool connected = false;

void main() {
    Hal_init();
    Hal_buttonEnable(buttonHandler);
    Pulsecounter_setDeviceName("PULS-CNTR");
    Pulsecounter_start();
    Hal_idleLoop();
}

static void buttonHandler(void) {
    uint8_t i;

    if (connected)
        Pulsecounter_event3_indicate();
    else
        Pulsecounter_accept(true);
    for (i = 0; i < 3; i++) {
        Hal_greenLedOn();
        Hal_redLedOn();
        Hal_delay(100);
        Hal_greenLedOff();
        Hal_redLedOff();
    }
    Hal_tickStart(5000, tickHandler);
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

void Pulsecounter_event3_fetch(Pulsecounter_event3_t* const output) {
    *output = buttonCnt;
}

void Pulsecounter_event4_fetch(Pulsecounter_event4_t* const output) {
    *output = 4;
}

void Pulsecounter_event5_fetch(Pulsecounter_event5_t* const output) {
    *output = 5;
}
