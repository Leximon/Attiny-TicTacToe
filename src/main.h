#pragma once

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include "millis.h"
#include "melody.h"

#define ROWS 3
#define COLUMNS 3

#define ROW_0_GREEN PA5
#define ROW_0_RED PA4
#define ROW_1_GREEN PA3
#define ROW_1_RED PA2
#define ROW_2_GREEN PA1
#define ROW_2_RED PA0
#define ALL_ROW_MASK (_BV(ROW_0_GREEN) | _BV(ROW_0_RED) | _BV(ROW_1_GREEN) | _BV(ROW_1_RED) | _BV(ROW_2_GREEN) | _BV(ROW_2_RED))

#define COL_0 PB2
#define COL_1 PB1
#define COL_2 PB0
#define ALL_COL_MASK (_BV(COL_0) | _BV(COL_1) | _BV(COL_2))

#define BUTTON_0_0 PB0
#define BUTTON_0_1 PB1
#define BUTTON_0_2 PB2

#define BUTTON_1_0 PA2
#define BUTTON_1_1 PA1
#define BUTTON_1_2 PA0

#define BUTTON_2_0 PA5
#define BUTTON_2_1 PA4
#define BUTTON_2_2 PA3
#define ALL_BUTTON_PB_MASK (_BV(BUTTON_0_0) | _BV(BUTTON_0_1) | _BV(BUTTON_0_2))
#define ALL_BUTTON_PA_MASK (_BV(BUTTON_1_0) | _BV(BUTTON_1_1) | _BV(BUTTON_1_2) | _BV(BUTTON_2_0) | _BV(BUTTON_2_1) | _BV(BUTTON_2_2))

#define BUTTON_GROUNDING_ENABLE PA6

#define ANIMATION_NONE 0
#define ANIMATION_WIN 1
#define ANIMATION_DRAW 2


void lightUpLeds();

void lightUpLedsInColumn(uint8_t column, uint8_t data);

int getLedRowPin(uint8_t row, bool red);

void readButtons();

void onButtonPressed(uint8_t column, uint8_t row);

void playAnimation(uint8_t animationId);

void animate();

bool checkWin();

void restartGame();

void turnLightsByMarkersThatWin(bool turnOn);