/*
The millis() function known from Arduino
Calling millis() will return the milliseconds since the program started

Tested on atmega328p

Using content from http://www.adnbr.co.uk/articles/counting-milliseconds
Author: Monoclecat, https://github.com/monoclecat/avr-millis-function

REMEMBER: Add sei(); after initMillis() to enable global interrupts!
 */

#include "millis.h"

volatile uint32_t timer1_millis;

ISR(TIM1_COMPA_vect) {
    timer1_millis++;
}

void initMillis(uint32_t fcpu) {
    uint32_t ctc_match_overflow;

    ctc_match_overflow = ((fcpu / 1000) / 8); //when timer1 is this value, 1ms has passed

    // (Set timer to clear when matching ctc_match_overflow) | (Set clock divisor to 8)
    TCCR1B |= (1 << WGM12) | (1 << CS11);

    // high byte first, then low byte
    OCR1AH = (ctc_match_overflow >> 8);
    OCR1AL = ctc_match_overflow;

    // Enable the compare match interrupt
    TIMSK1 |= (1 << OCIE1A);

    //REMEMBER TO ENABLE GLOBAL INTERRUPTS AFTER THIS WITH sei(); !!!
}

uint32_t millis() {
    uint32_t millis_return;

    // Ensure this cannot be disrupted
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        millis_return = timer1_millis;
    }
    return millis_return;
}

void resetMillis() {
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        timer1_millis = 0;
    }
}