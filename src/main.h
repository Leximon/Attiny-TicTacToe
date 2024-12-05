#pragma once

#include <avr/io.h>

void setCell(uint8_t cells[3], uint8_t row, uint8_t col, uint8_t state);

uint8_t getCell(const uint8_t cells[3], uint8_t row, uint8_t col);