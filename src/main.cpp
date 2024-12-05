#define F_CPU 1000000UL

#include "main.h"

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define CELL_STATE_OFF 0b00
#define CELL_STATE_RED 0b01
#define CELL_STATE_GREEN 0b10

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

#define setOutput(port, flags) port |= flags
#define setInput(port, flags) port &= ~(flags)
#define setHigh(port, flags) port |= flags
#define setLow(port, flags) port &= ~(flags)

uint8_t displayCells[3];

[[noreturn]] int main() {
    setOutput(LED_COL_DDR, ALL_LED_COLS);
    setOutput(LED_ROW_DDR, ALL_LED_ROWS_RG);

    TCCR0B |= (1 << CS01); // Set Timer0 prescaler to 8
    TIMSK |= (1 << TOIE0); // Enable Timer0 overflow interrupt
    sei();

    while (true) {

    }
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

ISR(TIMER0_OVF_vect) {
    volatile static uint8_t colIndex = 0;

    setLow(LED_ROW_PORT, ALL_LED_ROWS_RG);
    setLow(LED_COL_PORT, ALL_LED_COLS);

    uint8_t col = pgm_read_byte(&LED_COLS[colIndex]);
    setHigh(LED_COL_PORT, col);

    for (uint8_t row = 0; row < 3; row++) {
        uint8_t cellState = getCell(displayCells, row, colIndex);

        if (cellState == CELL_STATE_RED) {
            uint8_t rowR = pgm_read_byte(&LED_ROWS_R[row]);
            setHigh(LED_ROW_PORT, rowR);
        } else if (cellState == CELL_STATE_GREEN) {
            uint8_t rowG = pgm_read_byte(&LED_ROWS_G[row]);
            setHigh(LED_ROW_PORT, rowG);
        }
    }

    colIndex++;
    if (colIndex == 3)
        colIndex = 0;
}