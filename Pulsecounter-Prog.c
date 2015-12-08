#include "Pulsecounter.h"
#include "Hal.h"

static void buttonHandler(void);

void main() {
    Hal_init();
    Hal_buttonEnable(buttonHandler);
    Pulsecounter_start();
    Hal_idleLoop();
}

static void buttonHandler(void) {
    Hal_ledOn();
    Hal_delay(500);
    Hal_ledOff();
    Pulsecounter_event3_indicate();
}

/* -------- SCHEMA CALLBACKS -------- */

void Pulsecounter_connectHandler(void) {
    Hal_connected();
}

void Pulsecounter_disconnectHandler(void) {
    Hal_disconnected();
}

void Pulsecounter_event3_fetch(Pulsecounter_event3_t* const output) {
    *output = 3;
}

void Pulsecounter_event4_fetch(Pulsecounter_event4_t* const output) {
    *output = 4;
}

void Pulsecounter_event5_fetch(Pulsecounter_event5_t* const output) {
    *output = 5;
}
