#pragma once

#include <avr/io.h>

void tone(uint8_t level);

void readButtons();
void buttonPressed(uint8_t row, uint8_t col);
void playWinSequence(uint16_t pattern, uint8_t cellState);
void playDrawSequence();
void resetGame();
bool checkWin();
void beep();

void setCell(uint8_t cells[3], uint8_t row, uint8_t col, uint8_t state);
uint8_t getCell(const uint8_t cells[3], uint8_t row, uint8_t col);
void clearCells(const uint8_t cells[3]);