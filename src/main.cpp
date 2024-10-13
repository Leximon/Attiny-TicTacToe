#define F_CPU 1000000L

#include "main.h"

static const uint16_t MARKER_PLACED[] PROGMEM = {100, 100, 100, 50};

static const uint16_t *currentMelody = nullptr;
static uint8_t currentMelodyLength = 0;
static uint8_t currentMelodyIndex = 0;

static uint8_t board[ROWS] = {0, 0, 0};

int main() {
    DDRA |= _BV(BUZZER); // set buzzer as output
    TCCR0A |= _BV(WGM01); // timer0 CTC mode
    TCCR0B |= _BV(CS01); // timer0 prescaler div 8

    TCCR1B |= _BV(CS12) | _BV(CS10); // timer1 prescaler div 1024

    DDRA |= _BV(BUTTON_GROUNDING_ENABLE); // set switch pull-down enable pin as output

    while (true) {
        lightUpLeds();
        readButtons();
        handleMelody();
    }
}

void lightUpLeds() {
    PORTA &= ~_BV(BUTTON_GROUNDING_ENABLE); // disable button grounding !IMPORTANT! to prevent short circuit
    __asm__("nop");

    DDRA |= ALL_ROW_MASK; // set all row leds as output
    PORTA &= ~ALL_ROW_MASK; // set all row leds to LOW
    DDRB |= ALL_COL_MASK; // set all column leds as output
    PORTB |= ALL_COL_MASK; // set all column leds to HIGH

    for (uint8_t column = 0; column < COLUMNS; column++) {
        PORTB |= ALL_COL_MASK; // set all columns to HIGH

        lightUpLedsInColumn(column, board[column]);
        _delay_us(100);

        PORTA &= ~ALL_ROW_MASK; // turn off all row leds
    }

    PORTB |= ALL_COL_MASK; // set all columns to HIGH to prevent ghosting
}

void lightUpLedsInColumn(uint8_t column, uint8_t data) {
    switch (column) {
        case 0: PORTB &= ~_BV(COL_0); break;
        case 1: PORTB &= ~_BV(COL_1); break;
        case 2: PORTB &= ~_BV(COL_2); break;
        default: break;
    }

    for (uint8_t row = 0; row < ROWS; row++) {
        bool red = (data >> (row * 2 + 1)) & 0x1;

        bool isSet = (data >> (row * 2)) & 0x1;
        if (isSet) {
            PORTA |= _BV(getLedRowPin(row, red));
        }
    }
}

int getLedRowPin(uint8_t row, bool red) {
    switch (row) {
        case 0: return red ? PA4 : PA5;
        case 1: return red ? PA2 : PA3;
        case 2: return red ? PA0 : PA1;
        default:
            return -1;
    }
}

void tone(uint8_t level) {
    if (level == 0) {
        DDRA &= ~_BV(BUZZER);
        TCCR0A &= ~_BV(COM0B0);
        return;
    }

    DDRA |= _BV(BUZZER);
    TCCR0A |= _BV(COM0B0);
    OCR0A = level;
}

void playMelody(const uint16_t *melody, uint8_t length) {
    currentMelody = melody;
    currentMelodyLength = length;
    currentMelodyIndex = 0;

    uint16_t level = pgm_read_word(&melody[1]);
    tone(level);

    TCNT1 = 0;
}

void handleMelody() {
    if (currentMelody == nullptr) {
        return;
    }

    uint16_t duration = pgm_read_word(&currentMelody[currentMelodyIndex * 2]);

    if (TCNT1 >= duration) {
        TCNT1 = 0;

        currentMelodyIndex++;
        if (currentMelodyIndex >= currentMelodyLength) {
            currentMelody = nullptr;
            currentMelodyLength = 0;
            currentMelodyIndex = 0;
            tone(0);
            return;
        }

        uint16_t level = pgm_read_word(&currentMelody[currentMelodyIndex * 2 + 1]);
        tone(level);
    }

}

void readButtons() {
    /*
     * due to a design flaw, it's not possible to press multiple buttons at the same time without lighting up some of the leds
     * therefore we have to return early to prevent grounding the led cathodes that are also connected to the buttons
     */

    DDRA &= ~ALL_BUTTON_PA_MASK; // PORT-A set as input
    PORTA &= ~ALL_BUTTON_PA_MASK; // PORT-A disconnect to prevent lighting up some of the leds

    DDRB &= ~ALL_BUTTON_PB_MASK; // PORT-B set as input
    PORTB |= ALL_BUTTON_PB_MASK; // PORT-B enable pull-up resistor

    PORTA |= _BV(BUTTON_GROUNDING_ENABLE); // enable button grounding

    __asm__("nop");
    __asm__("nop");
    uint8_t dataPB = PINB;
    if (bit_is_clear(dataPB, BUTTON_0_0)) {
        onButtonPressed(0, 0);
        return;
    }
    if (bit_is_clear(dataPB, BUTTON_0_1)) {
        onButtonPressed(0, 1);
        return;
    }
    if (bit_is_clear(dataPB, BUTTON_0_2)) {
        onButtonPressed(0, 2);
        return;
    }

    PORTA |= ALL_BUTTON_PA_MASK; // PORT-A enable pull-up resistor

    __asm__("nop");
    __asm__("nop");
    uint8_t dataPA = PINA;
    if (bit_is_clear(dataPA, BUTTON_1_0)) {
        onButtonPressed(1, 0);
        return;
    }
    if (bit_is_clear(dataPA, BUTTON_1_1)) {
        onButtonPressed(1, 1);
        return;
    }
    if (bit_is_clear(dataPA, BUTTON_1_2)) {
        onButtonPressed(1, 2);
        return;
    }
    if (bit_is_clear(dataPA, BUTTON_2_0)) {
        onButtonPressed(2, 0);
        return;
    }
    if (bit_is_clear(dataPA, BUTTON_2_1)) {
        onButtonPressed(2, 1);
        return;
    }
    if (bit_is_clear(dataPA, BUTTON_2_2)) {
        onButtonPressed(2, 2);
        return;
    }
}

bool isRedsTurn = false;


void onButtonPressed(uint8_t column, uint8_t row) {
    if (board[column] >> (row * 2) & 0x1) {
        return;
    }

    board[column] |= (isRedsTurn ? 0b11 : 0b01) << (row * 2);
    isRedsTurn = !isRedsTurn;

    playMelody(MARKER_PLACED, sizeof(MARKER_PLACED) / sizeof(uint16_t) / 2);
}