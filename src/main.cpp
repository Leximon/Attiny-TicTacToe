#define F_CPU 1000000L

#include "main.h"

static const uint16_t MELODY_MARKER_PLACED[] PROGMEM = {
        100, 100,
        100, 50
};
static const uint16_t MELODY_DRAW[] PROGMEM = {
        100, 100,
        50, 0,
        100, 150,
        50, 0,
        100, 200,
        50, 0,
        300, 215
};
static const uint16_t MELODY_WIN[] PROGMEM = {
        100, 100,
        50, 0,
        100, 75,
        50, 0,
        100, 50,
        50, 0,
        100, 75,
        50, 0,
        100, 50
};

static uint8_t lights[COLUMNS] = {0, 0, 0};
static uint32_t lastInteractionTime = 0;

int main() {
    initMillis(F_CPU);
    Melody::init();
    DDRA |= _BV(BUTTON_GROUNDING_ENABLE); // set button ground enable pin as output
    sei();

    GIMSK |= _BV(PCIE0) | _BV(PCIE1); // enable pin change interrupt for wake-up
    PCMSK0 |= _BV(PCINT0) | _BV(PCINT1) | _BV(PCINT2) | _BV(PCINT3) | _BV(PCINT4) | _BV(PCINT5);
    PCMSK1 |= _BV(PCINT8) | _BV(PCINT9) | _BV(PCINT10);

    enterSleep();

    while (true) {
        readButtons();
        PORTA &= ~_BV(BUTTON_GROUNDING_ENABLE); // disable button grounding !IMPORTANT! to prevent short circuit
        _delay_loop_1(5);

        animateMask();
        lightUpLeds();
        Melody::tryPlayNextNote();

        if (millis() - lastInteractionTime > ENTER_SLEEP_TIMEOUT) {
            restartGame();
        }
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

        lightUpLedsInColumn(column, lights[column]);
        _delay_ms(3);

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
            uint8_t pin;
            if (row == 0) {
                pin = red ? ROW_0_RED : ROW_0_GREEN;
            } else if (row == 1) {
                pin = red ? ROW_1_RED : ROW_1_GREEN;
            } else {
                pin = red ? ROW_2_RED : ROW_2_GREEN;
            }
            PORTA |= _BV(pin);
        }
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

    _delay_loop_1(5);
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

    _delay_loop_1(5);
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

static uint8_t board[COLUMNS] = {0, 0, 0};
static bool isRedsTurn = false;
static uint8_t markersPlaced = 0;

static bool maskAnimationPlaying = false;
static uint32_t maskAnimationStartTime = 0;
static uint8_t animationMask[COLUMNS] = {0, 0, 0};

void setCellState(uint8_t* grid, uint8_t column, uint8_t row, uint8_t twoBitState) {
    uint8_t bitShift = row * 2;
    grid[column] &= ~(0b11 << bitShift);
    grid[column] |= twoBitState << bitShift;
}

uint8_t getCellState(const uint8_t* grid, uint8_t column, uint8_t row) {
    uint8_t bitShift = row * 2;
    return (grid[column] >> bitShift) & 0b11;
}

void onButtonPressed(uint8_t column, uint8_t row) {
    lastInteractionTime = millis();

    if (board[column] >> (row * 2) & 0x1 || maskAnimationPlaying) {
        return;
    }

    uint8_t state = isRedsTurn ? 0b11 : 0b01;
    setCellState(board, column, row, state);
    setCellState(lights, column, row, state);

    isRedsTurn = !isRedsTurn;
    markersPlaced++;

    if (checkWin()) {
        Melody::play(MELODY_WIN, sizeof(MELODY_WIN) / sizeof(uint16_t) / 2);
        playMaskAnimation();
        return;
    }

    if (markersPlaced == ROWS * COLUMNS) {
        Melody::play(MELODY_DRAW, sizeof(MELODY_DRAW) / sizeof(uint16_t) / 2);
        memcpy(animationMask, board, sizeof(animationMask));
        playMaskAnimation();
        return;
    }

    Melody::play(MELODY_MARKER_PLACED, sizeof(MELODY_MARKER_PLACED) / sizeof(uint16_t) / 2);
}

bool winCheckCells(
        uint8_t col1, uint8_t row1,
        uint8_t col2, uint8_t row2,
        uint8_t col3, uint8_t row3
) {
    uint8_t state1 = getCellState(board, col1, row1);
    uint8_t state2 = getCellState(board, col2, row2);
    uint8_t state3 = getCellState(board, col3, row3);
    if (state1 != 0b00 && state1 == state2 && state2 == state3) {
        memset(animationMask, 0, sizeof(animationMask));
        setCellState(animationMask, col1, row1, state1);
        setCellState(animationMask, col2, row2, state1);
        setCellState(animationMask, col3, row3, state1);
        return true;
    }

    return false;
}

bool checkWin() {
    // columns
    for (uint8_t column = 0; column < COLUMNS; column++) {
        if (board[column] == 0b010101 || board[column] == 0b111111) {
            memset(animationMask, 0, sizeof(animationMask));
            animationMask[column] = board[column];
            return true;
        }
    }

    // rows
    if (winCheckCells(0, 0,1, 0,2, 0))
        return true;
    if (winCheckCells(0, 1,1, 1,2, 1))
        return true;
    if (winCheckCells(0, 2,1, 2,2, 2))
        return true;

    // diagonals
    if (winCheckCells(0, 0,1, 1,2, 2))
        return true;
    if (winCheckCells(0, 2, 1, 1,2, 0))
        return true;
    return false;
}

void playMaskAnimation() {
    maskAnimationPlaying = true;
    maskAnimationStartTime = millis();
}

void animateMask() {
    if (!maskAnimationPlaying) {
        return;
    }

    uint32_t timePassed = millis() - maskAnimationStartTime;

    if (timePassed < 250*12) {
        bool turnOn = timePassed % 500 < 250;
        turnLightsByMarkerMask(turnOn);
    } else if (timePassed < 250*12 + 100 * ROWS * COLUMNS) {
        turnLightsByMarkerMask(true);
        for (uint8_t row = 0; row < ROWS; row++) {
            for (uint8_t column = 0; column < COLUMNS; column++) {
                if (timePassed > 250*12 + 100 * (row * COLUMNS + column)) {
                    setCellState(lights, column, row, 0b00);
                }
            }
        }
    } else {
        restartGame();
    }
}

void turnLightsByMarkerMask(bool turnOn) {
    for (uint8_t row = 0; row < ROWS; row++) {
        for (uint8_t column = 0; column < COLUMNS; column++) {
            uint8_t winMarkerState = getCellState(animationMask, column, row);
            if (winMarkerState == 0) {
                continue;
            }

            setCellState(lights, column, row, turnOn ? winMarkerState : 0b00);
        }
    }
}

void restartGame() {
    memset(board, 0, sizeof(board));
    memset(lights, 0, sizeof(lights));
    markersPlaced = 0;
    maskAnimationPlaying = false;

    enterSleep();
}

void enterSleep() {
    DDRB &= ~ALL_BUTTON_PB_MASK; // PORT-B set as input
    PORTB |= ALL_BUTTON_PB_MASK; // PORT-B enable pull-up resistor
    DDRA &= ~ALL_BUTTON_PA_MASK; // PORT-A set as input
    PORTA |= ALL_BUTTON_PA_MASK; // PORT-A enable pull-up resistor

    PORTA |= _BV(BUTTON_GROUNDING_ENABLE);

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_mode();

    PORTA &= ~_BV(BUTTON_GROUNDING_ENABLE);

    TCNT1 = 0;
    resetMillis();
}

ISR(PCINT0_vect) {
}
ISR(PCINT1_vect) {
}

void memset(uint8_t* buffer, uint8_t value, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = value;
    }
}

void memcpy(uint8_t* dest, const uint8_t* src, size_t size) {
    for (size_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }
}