#pragma once

#include "main.h"
#include <avr/io.h>
#include <util/atomic.h>
#include <avr/interrupt.h>

#define EVERY_N_MS(interval, run) {\
  static uint32_t lastStep = 0;\
  if (millis() - lastStep > interval) {\
    run\
    lastStep = millis();\
  }\
}

ISR(TIMER1_COMPA_vect);

void initMillis(uint32_t fcpu);

uint32_t millis();

void resetMillis();