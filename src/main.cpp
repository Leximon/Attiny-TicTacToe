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
        100, 200
};
static const uint16_t MELODY_WIN[] PROGMEM = {
        100, 100,
        50, 0,
        100, 75,
        50, 0,
        100, 50,
        50, 0,
        100, 75
};

static uint8_t lights[COLUMNS] = {0, 0, 0};
static bool lightsOn = true;

int main() {
    initMillis(F_CPU);
    Melody::init();
    DDRA |= _BV(BUTTON_GROUNDING_ENABLE); // set switch pull-down enable pin as output
    sei();

    while (true) {
        animate();
        if (lightsOn) {
            lightUpLeds();
        }
        readButtons();
        Melody::tryPlayNextNote();
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
        _delay_ms(2);

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

static uint8_t board[COLUMNS] = {0, 0, 0};
static bool isRedsTurn = false;
static uint8_t markersPlaced = 0;
static uint8_t markersThatWin[COLUMNS] = {0, 0, 0};

static int animation = ANIMATION_NONE;
static uint32_t animationStartTime = 0;

void setCellState(uint8_t* grid, uint8_t column, uint8_t row, uint8_t state) {
    uint8_t bitShift = row * 2;
    grid[column] &= ~(0b11 << bitShift);
    grid[column] |= state << bitShift;
}

uint8_t getCellState(const uint8_t* grid, uint8_t column, uint8_t row) {
    uint8_t bitShift = row * 2;
    return (grid[column] >> bitShift) & 0b11;
}

void onButtonPressed(uint8_t column, uint8_t row) {
    if (board[column] >> (row * 2) & 0x1 || animation != ANIMATION_NONE) {
        return;
    }

    uint8_t state = isRedsTurn ? 0b11 : 0b01;
    setCellState(board, column, row, state);
    setCellState(lights, column, row, state);

    isRedsTurn = !isRedsTurn;
    markersPlaced++;

    if (checkWin()) {
        Melody::play(MELODY_WIN, sizeof(MELODY_WIN) / sizeof(uint16_t) / 2);
        playAnimation(ANIMATION_WIN);
        return;
    }

    if (markersPlaced == ROWS * COLUMNS) {
        Melody::play(MELODY_DRAW, sizeof(MELODY_DRAW) / sizeof(uint16_t) / 2);
        playAnimation(ANIMATION_DRAW);
        return;
    }

    Melody::play(MELODY_MARKER_PLACED, sizeof(MELODY_MARKER_PLACED) / sizeof(uint16_t) / 2);
}

bool checkWin() {
    // columns
    for (uint8_t column = 0; column < COLUMNS; column++) {
        if (board[column] == 0b010101 || board[column] == 0b111111) {
            memset(markersThatWin, 0, sizeof(markersThatWin));
            markersThatWin[column] = board[column];
            return true;
        }
    }

    // rows
    for (uint8_t row = 0; row < ROWS; row++) {
        memset(markersThatWin, 0, sizeof(markersThatWin));

        uint8_t state = getCellState(board, 0, row);
        if (state == 0b00) {
            continue;
        }
        setCellState(markersThatWin, 0, row, state);

        bool validStreak = true;
        for (uint8_t column = 1; column < COLUMNS; column++) {
            uint8_t nextState = getCellState(board, column, row);

            if (state == nextState) {
                setCellState(markersThatWin, column, row, state);
            } else {
                validStreak = false;
                break;
            }
        }

        if (validStreak) {
            return true;
        }
    }

    // diagonals
    uint8_t state1 = getCellState(board, 0, 0);
    uint8_t state2 = getCellState(board, 1, 1);
    uint8_t state3 = getCellState(board, 2, 2);
    if (state1 != 0b00 && state1 == state2 && state2 == state3) {
        memset(markersThatWin, 0, sizeof(markersThatWin));
        setCellState(markersThatWin, 0, 0, state1);
        setCellState(markersThatWin, 1, 1, state1);
        setCellState(markersThatWin, 2, 2, state1);
        return true;
    }

    state1 = getCellState(board, 2, 0);
    state2 = getCellState(board, 1, 1);
    state3 = getCellState(board, 0, 2);
    if (state1 != 0b00 && state1 == state2 && state2 == state3) {
        memset(markersThatWin, 0, sizeof(markersThatWin));
        setCellState(markersThatWin, 2, 0, state1);
        setCellState(markersThatWin, 1, 1, state1);
        setCellState(markersThatWin, 0, 2, state1);
        return true;
    }

    return false;
}

void playAnimation(uint8_t animationId) {
    animation = animationId;
    animationStartTime = millis();
}

void animate() {
    if (animation == ANIMATION_NONE) {
        return;
    }

    uint32_t timePassed = millis() - animationStartTime;

    if (timePassed < 250*8) {
        bool turnOn = timePassed % 500 < 250;
        if (animation == ANIMATION_WIN) {
            turnLightsByMarkersThatWin(turnOn);
        } else {
            lightsOn = turnOn;
        }
    } else if (timePassed < 250*8 + 100 * ROWS * COLUMNS) {
        if (animation == ANIMATION_WIN) {
            turnLightsByMarkersThatWin(true);
        } else {
            lightsOn = true;
        }
        for (uint8_t row = 0; row < ROWS; row++) {
            for (uint8_t column = 0; column < COLUMNS; column++) {
                if (timePassed > 250*8 + 100 * (row * COLUMNS + column)) {
                    setCellState(lights, column, row, 0b00);
                }
            }
        }
    } else {
        restartGame();
    }

    return;
}

void turnLightsByMarkersThatWin(bool turnOn) {
    for (uint8_t row = 0; row < ROWS; row++) {
        for (uint8_t column = 0; column < COLUMNS; column++) {
            uint8_t winMarkerState = getCellState(markersThatWin, column, row);
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
    animation = ANIMATION_NONE;
    lightsOn = true;
}