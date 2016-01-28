/*
 * ============ Hardware Abstraction Layer for MSP-EXP430G2 LaunchPad ============
 */

#include "Hal.h"
#include "Em_Message.h"

#include <msp430.h>


/* -------- INTERNAL FEATURES -------- */

#define GREEN_LED_CONFIG()                (P1DIR |= BIT6)
#define GREEN_LED_ON()                    (P1OUT |= BIT6)
#define GREEN_LED_OFF()                   (P1OUT &= ~BIT6)
#define GREEN_LED_READ()                  (P1OUT & BIT6)
#define GREEN_LED_TOGGLE()                (P1OUT ^= BIT6)

#define RED_LED_CONFIG()                  (P1DIR |= BIT0)
#define RED_LED_ON()                      (P1OUT |= BIT0)
#define RED_LED_OFF()                     (P1OUT &= ~BIT0)
#define RED_LED_READ()                    (P1OUT & BIT0)
#define RED_LED_TOGGLE()                  (P1OUT ^= BIT0)

#define GPIO_CONFIG(mask)           (P1DIR &= ~mask, P1REN |= mask, P1OUT |= mask, P1IES |= mask);
#define GPIO_ENABLE(mask)           (P1IFG &= ~mask, P1IE |= mask)
#define GPIO_DISABLE(mask)          (P1IE &= ~mask, P1IFG &= ~mask)
#define GPIO_FIRED(mask)            (P1IFG & mask)
#define GPIO_ACK(mask)              (P1IFG &= ~mask)
#define GPIO_LOW(mask)              (!(P1IN & mask))
#define GPIO_DEBOUNCE_MSECS         100

#define EAP_RX_BUF                  UCA0RXBUF
#define EAP_TX_BUF                  UCA0TXBUF

#define EAP_RX_VECTOR               USCIAB0RX_VECTOR
#define EAP_TX_VECTOR               USCIAB0TX_VECTOR
#define EAP_TX_ACK_VECTOR           PORT2_VECTOR

#define EAP_RX_ENABLE()             (P1SEL |= BIT1, P1SEL2 |= BIT1)
#define EAP_RX_DISABLE()            (P1SEL &= ~BIT1, P1SEL2 &= ~BIT1)
#define EAP_TX_ENABLE()             (P1SEL |= BIT2, P1SEL2 |= BIT2)
#define EAP_TX_DISABLE()            (P1SEL &= ~BIT2, P1SEL2 &= ~BIT2)

#define EAP_RX_ACK_CONFIG()         (P2DIR |= BIT0)
#define EAP_RX_ACK_SET()            (P2OUT |= BIT0)
#define EAP_RX_ACK_CLR()            (P2OUT &= ~BIT0)

#define EAP_TX_ACK_CONFIG()         (P2DIR &= ~BIT1, P2IES |= BIT1, P2IFG &= ~BIT1, P2IE |= BIT1)
#define EAP_TX_ACK_TST()            (P2IFG & BIT1)
#define EAP_TX_ACK_CLR()            (P2IFG &= ~BIT1)

#define EAP_RX_INT_CLR()            (IFG2 &= ~UCA0RXIFG)
#define EAP_RX_INT_ENABLE()         (IE2 |= UCA0RXIE)
#define EAP_TX_INT_CLR()            (IFG2 &= ~UCA0TXIFG)
#define EAP_TX_INT_DISABLE()        (IE2 &= ~UCA0TXIE)
#define EAP_TX_INT_ENABLE()         (IE2 |= UCA0TXIE)

#define MCLK_TICKS_PER_MS           1000L
#define ACLK_TICKS_PER_SECOND       1500L /* was 12000L with divider /1 */
#define UART_WATCHDOG_PERIOD        (ACLK_TICKS_PER_SECOND * 250) / 1000

#define UART_WATCH_DISABLE()        (TA1CCTL1 = 0)                                              // Turn off CCR1 Interrupt
#define UART_WATCH_ENABLE()         (TA1CCR1 = TA1R + UART_WATCHDOG_PERIOD, TA1CCTL1 = CCIE)    // Set CCR1, and Enable CCR1 Interrupt

#ifdef __GNUC__
#define DINT()                      __disable_interrupt()
#define EINT()                      __enable_interrupt()
#define INTERRUPT
#define SLEEP()                     _BIS_SR(LPM3_bits + GIE)
#define WAKEUP()                    _BIC_SR_IRQ(LPM3_bits)
#endif

#ifdef __TI_COMPILER_VERSION__
#define DINT()                      (_disable_interrupt())
#define EINT()                      (_enable_interrupt())
#define INTERRUPT interrupt
#define SLEEP()                     (__bis_SR_register(LPM3_bits + GIE))
#define WAKEUP()                    (__bic_SR_register_on_exit(LPM3_bits))
#endif

#define NUM_HANDLERS 5

#define EVENT3_HANDLER_ID      0
#define EVENT4_HANDLER_ID      1
#define EVENT5_HANDLER_ID      2
#define TICK_HANDLER_ID        3
#define DISPATCH_HANDLER_ID    4

static void gpioHandler(uint8_t id);
static void postEvent(uint8_t handlerId);

static Hal_Handler appSettleHandler;
static void (*appJitterHandler)(uint8_t id, uint32_t count);
static volatile uint16_t handlerEvents = 0;
static uint16_t clockTick = 0;
static Hal_Handler handlerTab[NUM_HANDLERS];
static volatile uint32_t gpioCount[3] = {0};
static bool timerActive[3] = {false, false, false};
static uint16_t timerPoint[3];

/* -------- INTERNAL FUNCTIONS -------- */

static uint32_t getCount(uint8_t id) {
    DINT();
    uint32_t count = gpioCount[id];
    gpioCount[id] = 0;
    EINT();
    return count;
}

static void setTimer(uint8_t id, uint16_t delay) {
    uint8_t i;
    uint16_t now, left;

    timerActive[id] = true;
    // enable clock if it was disabled to save power?
    now = TA1R;
    timerPoint[id] = now + delay;
    left = ACLK_TICKS_PER_SECOND;
    for (i = 0; i < 3; i++)
        if (timerActive[i] && (timerPoint[i] - now) < left) {
            left = timerPoint[i] - now;
        }
    TA1CCR0 = now + left;
    TA1CCTL0 = CCIE;
}

static void clearTimer(uint8_t id) {
    uint8_t i;
    bool keep = false;

    timerActive[id] = false;
    for (i = 0; i < 3; i++)
        if (timerActive[i]) keep = true;
    if (!keep) {
        TA1CCTL0 = 0;
        // disable clock to save power?
    }
}

static void gpioHandler(uint8_t id) {
    if (timerActive[id])
        return;
    setTimer(id, ACLK_TICKS_PER_SECOND); // One second ahead
}

static void tickHandler(uint16_t clock) {
    uint8_t i;

    for (i = 0; i < 3; i++)
        if (timerActive[i] && timerPoint[i] == clock) {
            uint32_t count = getCount(i);
            uint16_t mask = BIT3 << i;

            if (count) {
                setTimer(i, ACLK_TICKS_PER_SECOND); // One second ahead
                if (appJitterHandler) (*appJitterHandler)(i, count);
            } else {
                clearTimer(i);
                if (GPIO_LOW(mask) && appSettleHandler) (*appSettleHandler)(i);
            }
        }
    // if all timers are unset, disable ticker.
}

static void postEvent(uint8_t handlerId) {
    uint8_t key = Em_Hal_lock();
    handlerEvents |= 1 << handlerId;
    Em_Hal_unlock(key);
}

/* -------- APP-HAL INTERFACE -------- */

void Hal_gpioEnable(Hal_Handler shandler, void (*jhandler)(uint8_t id, uint32_t count)) {
    uint8_t id;
    uint16_t mask;

    for (id = 0, mask = BIT3; id < 3; id++, mask <<= 1) {
        handlerTab[id] = gpioHandler;
        appSettleHandler = shandler;
        appJitterHandler = jhandler;
        (P1DIR &= ~mask, P1REN |= mask, P1OUT |= mask, P1IES |= mask);
        Hal_delay(100);
        (P1IFG &= ~mask, P1IE |= mask);
    }
    handlerTab[TICK_HANDLER_ID] = tickHandler;
}

void Hal_connected(void) {
}

void Hal_delay(uint16_t msecs) {
    while (msecs--) {
        __delay_cycles(MCLK_TICKS_PER_MS);
    }
}

void Hal_disconnected(void) {
}

void Hal_init(void) {

    /* setup clocks */

    WDTCTL = WDTPW + WDTHOLD;
    /* MCLK = DCOCLK */
    /* MCLK divider = /1 */
    /* SMCLK divider = /1 */
    BCSCTL2 = SELM_0 + DIVM_0 + DIVS_0;
    if (CALBC1_1MHZ != 0xFF) {
        DCOCTL = 0x00;
        BCSCTL1 = CALBC1_1MHZ;      /* Set DCO to 1MHz */
        DCOCTL = CALDCO_1MHZ;
    }
    /* XT2 is off (Not used for MCLK/SMCLK) */
    /* ACLK divider = /8 */
    BCSCTL1 |= XT2OFF + DIVA_3;
    /* XT2 range = 0.4 - 1 MHz */
    /* LFXT1 range/VLO = VLOCLK (or 3-16 MHz if XTS=1) */
    /* Capacitor 6 pF */
    BCSCTL3 = XT2S_0 + LFXT1S_2 + XCAP_1;

    /* setup LEDs */

    GREEN_LED_CONFIG();
    GREEN_LED_OFF();
    RED_LED_CONFIG();
    RED_LED_OFF();

    /* setup TimerA1 */
    TA1CTL = TASSEL_1 + MC_2;           // ACLK, Continuous mode
    UART_WATCH_DISABLE();

    /* setup UART */

    UCA0CTL1 |= UCSWRST;

    EAP_RX_ENABLE();
    EAP_TX_ENABLE();

    EAP_RX_ACK_CONFIG();
    EAP_RX_ACK_SET();

    EAP_TX_ACK_CONFIG();

    // suspend the MCM
    EAP_RX_ACK_CLR();

    UCA0CTL1 = UCSSEL_2 + UCSWRST;
    UCA0MCTL = UCBRF_0 + UCBRS_6;
    UCA0BR0 = 8;
    UCA0CTL1 &= ~UCSWRST;

    handlerTab[DISPATCH_HANDLER_ID] = Em_Message_dispatch;
}

void Hal_idleLoop(void) {

    EINT();
    for (;;) {

        // atomically read/clear all handlerEvents
        DINT();
        uint16_t events = handlerEvents;
        handlerEvents = 0;

        if (events) {   // dispatch all current events
            EINT();
            uint16_t mask;
            uint8_t id;

            for (id = 0, mask = 0x1; id < NUM_HANDLERS; id++, mask <<= 1) {
                if ((events & mask) && handlerTab[id]) {
                    if (id == TICK_HANDLER_ID) {
                        uint16_t now = TA1R;
                        handlerTab[id](now);
                    } else {
                        handlerTab[id](id);
                    }
                }
            }
        }
        else {          // await more events
            SLEEP();    // this also enables interrupts
        }
    }
}

void Hal_greenLedOn(void) {
    GREEN_LED_ON();
}

void Hal_greenLedOff(void) {
    GREEN_LED_OFF();
}

bool Hal_greenLedRead(void) {
    return GREEN_LED_READ();
}

void Hal_greenLedToggle(void) {
    GREEN_LED_TOGGLE();
}

void Hal_redLedOn(void) {
    RED_LED_ON();
}

void Hal_redLedOff(void) {
    RED_LED_OFF();
}

bool Hal_redLedRead(void) {
    return RED_LED_READ();
}

void Hal_redLedToggle(void) {
    RED_LED_TOGGLE();
}

uint16_t Hal_tickStart(uint16_t msecs, void (*handler)(uint16_t clock)) {
    handlerTab[TICK_HANDLER_ID] = handler;
    clockTick = (ACLK_TICKS_PER_SECOND * msecs) / 1000;
    uint16_t then = TA1R + clockTick;
    TA1CCR0 = then;               // Set the CCR0 interrupt for msecs from now.
    TA1CCTL0 = CCIE;                            // Enable the CCR0 interrupt
    return then;
}

void Hal_tickStop(void) {
    handlerTab[TICK_HANDLER_ID] = 0;
    TA1CCR0 = 0;
    TA1CCTL0 = 0;
}

/* -------- SRT-HAL INTERFACE -------- */

uint8_t Em_Hal_lock(void) {
        uint8_t key = _get_interrupt_state();
    #ifdef __GNUC__
        __disable_interrupt();
    #endif
    #ifdef __TI_COMPILER_VERSION__
        _disable_interrupt();
    #endif
        return key;
}

void Em_Hal_reset(void) {
    uint8_t key = Em_Hal_lock();
    EAP_RX_ACK_CLR();    // suspend the MCM
    Hal_delay(100);
    EAP_RX_ACK_SET();    // reset the MCM
    Hal_delay(500);
    EAP_RX_INT_CLR();
    EAP_TX_INT_CLR();
    EAP_TX_ACK_CLR();
    EAP_RX_INT_ENABLE();
    Em_Hal_unlock(key);
}

void Em_Hal_startSend() {
    EAP_TX_BUF = Em_Message_startTx();
}

void Em_Hal_unlock(uint8_t key) {
        _set_interrupt_state(key);
}

void Em_Hal_watchOff(void) {
    UART_WATCH_DISABLE();
}

void Em_Hal_watchOn(void) {
    UART_WATCH_ENABLE();
}

/* -------- INTERRUPT SERVICE ROUTINES -------- */

#ifdef __GNUC__
    __attribute__((interrupt(PORT1_VECTOR)))
#endif
#ifdef __TI_COMPILER_VERSION__
    #pragma vector=PORT1_VECTOR
#endif
INTERRUPT void gpioIsr(void) {
    uint8_t id;
    uint16_t mask;

    for (id = 0, mask = BIT3; id < 3; id++, mask <<= 1)
        if (GPIO_FIRED(mask)) {
            gpioCount[id]++;
            postEvent(id);
            GPIO_ACK(mask);
        }
    WAKEUP();
}

#ifdef __GNUC__
    __attribute__((interrupt(EAP_RX_VECTOR)))
#endif
#ifdef __TI_COMPILER_VERSION__
    #pragma vector=EAP_RX_VECTOR
#endif
INTERRUPT void rxIsr(void) {
    uint8_t b = EAP_RX_BUF;
    Em_Message_startRx();
    EAP_RX_ACK_CLR();
    EAP_RX_ACK_SET();
    if (Em_Message_addByte(b)) {
        postEvent(DISPATCH_HANDLER_ID);
    }
    WAKEUP();
}

#ifdef __GNUC__
    __attribute__((interrupt(TIMER1_A0_VECTOR)))
#endif
#ifdef __TI_COMPILER_VERSION__
    #pragma vector=TIMER1_A0_VECTOR
#endif
INTERRUPT void timerIsr(void) {
    // TA1CCR0 += clockTick;
    postEvent(TICK_HANDLER_ID);
    WAKEUP();
}

#ifdef __GNUC__
    __attribute__((interrupt(EAP_TX_ACK_VECTOR)))
#endif
#ifdef __TI_COMPILER_VERSION__
    #pragma vector=EAP_TX_ACK_VECTOR
#endif
INTERRUPT void txAckIsr(void) {
    if (EAP_TX_ACK_TST()) {
        uint8_t b;
        if (Em_Message_getByte(&b)) {
            EAP_TX_BUF = b;
        }
        EAP_TX_ACK_CLR();
    }
    WAKEUP();
}

#ifdef __GNUC__
    __attribute__((interrupt(TIMER1_A1_VECTOR)))
#endif
#ifdef __TI_COMPILER_VERSION__
    #pragma vector=TIMER1_A1_VECTOR
#endif
INTERRUPT void uartWatchdogIsr(void) {
    switch (TA1IV) {
    case  2:  // CCR1
        UART_WATCH_DISABLE();
        Em_Message_restart();
        WAKEUP();
        break;
    }
}
