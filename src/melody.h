#pragma once

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "millis.h"

#define BUZZER PA7

namespace Melody {

    void init();

    void tryPlayNextNote();

    void play(const uint16_t *melody, uint8_t length);

    void tone(uint8_t level);

}