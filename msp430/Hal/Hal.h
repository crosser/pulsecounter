/**
 * Hal.h -- HAL Interface Definitions
 *
 * This example HAL is intentionally simple.  The implementation is limited to:
 *
 * BUTTON -- a single button that when pressed will cause an interrupt.
 * DELAY -- a delay routine that can delay by n milliseconds.
 * INIT -- set the hardware up to its initial state
 * LED -- a user LED that is available for application control.
 * TICK -- a timer that can be set to interrupt every n milliseconds
 * IDLE LOOP -- an event driven idle loop for controlling the EAP
 *
 * For information on Hal implementations for specific target hardware platforms,
 * visit the http://wiki.em-hub.com/display/ED.
 *
 **/

#ifndef Hal__H
#define Hal__H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*Hal_Handler)(uint8_t id);

/**
 * --------- Hal_buttonEnable ---------
 *
 * Enable the button interrupt and connect it to the user's buttonHandler
 *
 * When the button is pressed, it will cause an interrupt that will cause BUTTON event
 * to be entered into the event list.  Once dispatched by the idle loop, the user's
 * buttonHandler will be called.
 *
 * Inputs:
 *   buttonHandler - pointer to the user's handler to be called after interrupt
 *
 * Returns:
 *   None
 *
 * Side effects:
 *   BUTTON interrupt enabled
 *
 **/
extern void Hal_gpioEnable(Hal_Handler handler);
/**
 * --------- Hal_connected ---------
 *
 * Called whenever the MCM peripheral connects to a central.
 *
 * Could do other things associated with connection to the central.
 *
 * Inputs:
 *   None
 *
 * Returns:
 *   None
 *
 **/
extern void Hal_connected(void);
/**
 * --------- Hal_delay ---------
 *
 * Delays for the specified number of milliseconds.
 *
 * In this example, delay is done with CPU spinning for simplicity's sake.
 * This could easily use a timer interrupt for more power savings.
 *
 * Inputs:
 *   msecs - the number of milliseconds to delay
 *
 * Returns:
 *   None
 *
 * Side Effects:
 *   None
 *
 **/
extern void Hal_delay(uint16_t msecs);
/**
 * --------- Hal_disconnected ---------
 *
 * Called whenever the MCM peripheral disconnects from a central.
 *
 * Could do other things associated with connection to the central.
 *
 * Inputs:
 *   None
 *
 * Returns:
 *   None
 *
 **/
extern void Hal_disconnected(void);
/**
 * --------- Hal_idleLoop ---------
 *
 * The idle loop that controls EAP operations.
 *
 * The hal implements an event driven "idle loop" scheduler.
 * When there are no events pending, the idle loop sleeps.
 * When an event happens, the idle loop wakes up, and dispatches
 * to the appropriate event handler.
 *
 * The dispatching is done through a handlerTab that has one entry for each type of event.
 * Each handlerTab entry should be a handler of type hal_handler *.
 * There are currently three types of events, i.e. entries in the handlerTab:
 *   BUTTON_HANDLER_ID:    handler to call upon a button press
 *   TICK_HANDLER_ID:      handler to call upon a timer interrupt
 *   DISPATCH_HANDLER_ID:  handler to call upon a received message from the MCM
 *
 * Inputs:
 *   None
 *
 * Returns:
 *   None
 *
 * Side Effects:
 *   dispatches events as they come in
 *
 **/
extern void Hal_idleLoop(void);
/**
 * --------- Hal_init ---------
 *
 * Initialize the hardware
 *
 * Initializes the EAP and MCM into their reset state.  Should be called first.
 * Sets up the clock, ports, watchdog timer, etc.
 *
 *
 * Inputs:
 *   None
 *
 * Returns:
 *   None
 *
 * Side Effects:
 *   EAP and MCM in their initial state.
 *
 **/
extern void Hal_init(void);
/**
 * --------- Hal_ledOff ---------
 *
 * Turns the user LED off.
 *
 * Inputs:
 *   None
 *
 * Returns:
 *   None
 *
 * Side Effects:
 *   User LED off.
 *
 **/
extern void Hal_greenLedOff(void);
extern void Hal_redLedOff(void);
/**
 * --------- Hal_ledOn ---------
 *
 * Turns the user LED on.
 *
 * Inputs:
 *   None
 *
 * Returns:
 *   None
 *
 * Side Effects:
 *   User LED on.
 *
 **/
extern void Hal_greenLedOn(void);
extern void Hal_redLedOn(void);
/**
 * --------- Hal_ledRead ---------
 *
 * Returns the user LED state.
 *
 * Inputs:
 *   None
 *
 * Returns:
 *   Bool -  (true = user LED is on, false = user LED is off)
 *
 * Side Effects:
 *   None
 *
 **/
extern bool Hal_greenLedRead(void);
extern bool Hal_redLedRead(void);
/**
 * --------- Hal_ledToggle ---------
 *
 * Toggles the user LED.
 *
 * Inputs:
 *   None
 *
 * Returns:
 *   None
 *
 * Side Effects:
 *   User LED toggles state.
 *
 **/
extern void Hal_greenLedToggle(void);
extern void Hal_redLedToggle(void);
/**
 * --------- Hal_tickStart ---------
 *
 * Sets up the timer to interrupt every msecs milliseconds and the user's tickHandler
 * that will be called upon interrupt.
 *
 * Enable a timer interrupt every msecs ms.  The interrupt will cause a TICK event
 * to be entered into the event list.  Once dispatched by the idle loop, the user's
 * tickHandler will be called.
 *
 * Inputs:
 *   msecs - the number of milliseconds between tick interrupts
 *   tickHandler - the address of the user's tick handler that will be called
 *
 * Returns:
 *   Future clock when handler will be called
 *
 * Side Effects:
 *   tickhandler called by the idle loop
 *
 **/
extern uint16_t Hal_tickStart(uint16_t msecs, void (*handler)(uint16_t clock));
extern void Hal_tickStop(void);

/**
 * --------- Hal_gpioCount ---------
 *
 * Returns the number of interrups encounted on gpio `id`
 *
 * Inputs:
 *   id if the gpio (0-2 for gpio 3-5)
 *
 * Returns:
 *   Counted interrupts
 *
 * Side Effects:
 *   Resets the accumulator (counting restarts from zero).
 *
 **/
extern uint32_t Hal_gpioCount(uint8_t id);

#ifdef __cplusplus
}
#endif

#endif /* Hal__H */
