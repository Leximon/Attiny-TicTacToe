#define F_CPU 1000000UL

#include "main.h"

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#define CELL_STATE_OFF 0b00
#define CELL_RED 0b01
#define CELL_GREEN 0b10
#define CELL_ORANGE 0b11

#define LED_COL_PORT PORTB
#define LED_COL_DDR DDRB
#define LED_COL_1 (1 << PB0)
#define LED_COL_2 (1 << PB1)
#define LED_COL_3 (1 << PB2)
#define ALL_LED_COLS (LED_COL_1 | LED_COL_2 | LED_COL_3)

static const uint8_t LED_COLS[] PROGMEM = {LED_COL_1, LED_COL_2, LED_COL_3};

#define LED_ROW_PORT PORTD
#define LED_ROW_DDR DDRD
#define LED_ROW_1_R (1 << PD1)
#define LED_ROW_1_G (1 << PD0)
#define LED_ROW_2_R (1 << PD2)
#define LED_ROW_2_G (1 << PD3)
#define LED_ROW_3_R (1 << PD4)
#define LED_ROW_3_G (1 << PD5)
#define ALL_LED_ROWS_RG (LED_ROW_1_R | LED_ROW_1_G | LED_ROW_2_R | LED_ROW_2_G | LED_ROW_3_R | LED_ROW_3_G)

static const uint8_t LED_ROWS_R[] PROGMEM = {LED_ROW_1_R, LED_ROW_2_R, LED_ROW_3_R};
static const uint8_t LED_ROWS_G[] PROGMEM = {LED_ROW_1_G, LED_ROW_2_G, LED_ROW_3_G};

#define BUTTON_COL_A_PORT PORTD
#define BUTTON_COL_A_DDR DDRD
#define BUTTON_COL_1A (1 << PD6)
#define ALL_BUTTON_COLS_A (BUTTON_COL_1A)

#define BUTTON_COL_B_PORT PORTB
#define BUTTON_COL_B_DDR DDRB
#define BUTTON_COL_2B (1 << PB3)
#define BUTTON_COL_3B (1 << PB4)
#define ALL_BUTTON_COLS_B (BUTTON_COL_2B | BUTTON_COL_3B)

#define BUTTON_ROW_PORT PORTB
#define BUTTON_ROW_DDR DDRB
#define BUTTON_ROW_PIN PINB
#define BUTTON_ROW_1 (1 << PB7)
#define BUTTON_ROW_2 (1 << PB6)
#define BUTTON_ROW_3 (1 << PB5)
#define ALL_BUTTON_ROWS (BUTTON_ROW_1 | BUTTON_ROW_2 | BUTTON_ROW_3)

static const uint8_t BUTTON_ROWS[] PROGMEM = {BUTTON_ROW_1, BUTTON_ROW_2, BUTTON_ROW_3};

#define setOutput(port, flags) port |= flags
#define setInput(port, flags) port &= ~(flags)
#define setHigh(port, flags) port |= flags
#define setLow(port, flags) port &= ~(flags)
#define enablePullUp(port, flags) port |= flags

uint8_t displayCells[3];

[[noreturn]] int main() {
    setOutput(LED_COL_DDR, ALL_LED_COLS);
    setOutput(LED_ROW_DDR, ALL_LED_ROWS_RG);

    setInput(BUTTON_ROW_DDR, ALL_BUTTON_ROWS);
    enablePullUp(BUTTON_ROW_PORT, ALL_BUTTON_ROWS);

    TCCR0B |= (1 << CS01); // Set Timer0 prescaler to 8
    // TCCR0B |= (1 << CS02); // Set Timer0 prescaler to 256
    // TCCR0B |= (1 << CS02) | (1 << CS00); // Set Timer0 prescaler to 1024
    TIMSK |= (1 << TOIE0); // Enable Timer0 overflow interrupt
    sei();

    while (true) {
        readButtons();
    }
}

void readButtons() {
    uint8_t pressedCol = 255;
    uint8_t pressedRow = 255;

    for (uint8_t col = 0; col < 3; col++) {
        // set tristate for all columns
        setInput(BUTTON_COL_A_DDR, ALL_BUTTON_COLS_A);
        setInput(BUTTON_COL_B_DDR, ALL_BUTTON_COLS_B);

        // set one of the columns to low by setting it as output
        switch (col) {
            case 0: setOutput(BUTTON_COL_A_DDR, BUTTON_COL_1A); break;
            case 1: setOutput(BUTTON_COL_B_DDR, BUTTON_COL_2B); break;
            case 2: setOutput(BUTTON_COL_B_DDR, BUTTON_COL_3B); break;
            default: break;
        }

        __asm__("nop");

        uint8_t pin = BUTTON_ROW_PIN;

        for (uint8_t row = 0; row < 3; row++) {
            uint8_t rowFlag = pgm_read_byte(&BUTTON_ROWS[row]);

            if (pin & rowFlag)
                continue; // button not pressed

            if (pressedRow != 255 || pressedCol != 255)
                return; // more than one button pressed: ignore


            pressedRow = row;
            pressedCol = col;
        }
    }

    if (pressedRow != 255 && pressedCol != 255)
        buttonPressed(pressedRow, pressedCol);
}

bool isGreensTurn = false;

void buttonPressed(uint8_t row, uint8_t col) {
    uint8_t cell = getCell(displayCells, row, col);
    if (cell != CELL_STATE_OFF)
        return;


    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        setCell(displayCells, row, col, isGreensTurn ? CELL_GREEN : CELL_RED);
    }
    isGreensTurn = !isGreensTurn;
}

void setCell(uint8_t cells[3], uint8_t row, uint8_t col, uint8_t state) {
    uint8_t bitShift = row * 2;
    cells[col] &= ~(0b11 << bitShift);
    cells[col] |= state << bitShift;
}

uint8_t getCell(const uint8_t cells[3], uint8_t row, uint8_t col) {
    uint8_t bitShift = row * 2;
    return (cells[col] >> bitShift) & 0b11;
}

// light up the LEDs
ISR(TIMER0_OVF_vect) {
    volatile static uint8_t colIndex = 0;

    setLow(LED_ROW_PORT, ALL_LED_ROWS_RG);
    setLow(LED_COL_PORT, ALL_LED_COLS);

    uint8_t col = pgm_read_byte(&LED_COLS[colIndex]);
    setHigh(LED_COL_PORT, col);

    for (uint8_t row = 0; row < 3; row++) {
        uint8_t cellState = getCell(displayCells, row, colIndex);

        if (cellState == CELL_RED || cellState == CELL_ORANGE) {
            uint8_t rowR = pgm_read_byte(&LED_ROWS_R[row]);
            setHigh(LED_ROW_PORT, rowR);
        }
        if (cellState == CELL_GREEN || cellState == CELL_ORANGE) {
            uint8_t rowG = pgm_read_byte(&LED_ROWS_G[row]);
            setHigh(LED_ROW_PORT, rowG);
        }
    }

    colIndex++;
    if (colIndex == 3)
        colIndex = 0;
}