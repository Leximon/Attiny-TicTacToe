#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/delay.h>

#define LED_COL_PORT PORTB
#define LED_COL_DDR DDRB
#define LED_COL_1 (1 << PB0)
#define LED_COL_2 (1 << PB1)
#define LED_COL_3 (1 << PB2)
#define ALL_LED_COLS (LED_COL_1 | LED_COL_2 | LED_COL_3)

#define LED_ROW_PORT PORTD
#define LED_ROW_DDR DDRD
#define LED_ROW_1_R (1 << PD1)
#define LED_ROW_1_G (1 << PD0)
#define LED_ROW_2_R (1 << PD2)
#define LED_ROW_2_G (1 << PD3)
#define LED_ROW_3_R (1 << PD4)
#define LED_ROW_3_G (1 << PD5)
#define ALL_LED_ROWS (LED_ROW_1_R | LED_ROW_1_G | LED_ROW_2_R | LED_ROW_2_G | LED_ROW_3_R | LED_ROW_3_G)

#define setOutput(port, flags) port |= flags
#define setInput(port, flags) port &= ~(flags)
#define setHigh(port, flags) port |= flags
#define setLow(port, flags) port &= ~(flags)

[[noreturn]] int main() {
    setOutput(LED_COL_DDR, ALL_LED_COLS);
    setOutput(LED_ROW_DDR, ALL_LED_ROWS);

    setHigh(LED_ROW_PORT, LED_ROW_1_G);
    setHigh(LED_ROW_PORT, LED_ROW_2_R);
    setHigh(LED_ROW_PORT, LED_ROW_3_G);

    while (true) {
        setHigh(LED_COL_PORT, LED_COL_1);
        _delay_loop_1(10);
        setLow(LED_COL_PORT, LED_COL_1);
        _delay_loop_1(10);
        setHigh(LED_COL_PORT, LED_COL_2);
        _delay_loop_1(10);
        setLow(LED_COL_PORT, LED_COL_2);
        _delay_loop_1(10);
        setHigh(LED_COL_PORT, LED_COL_3);
        _delay_loop_1(10);
        setLow(LED_COL_PORT, LED_COL_3);
        _delay_loop_1(10);
    }
}