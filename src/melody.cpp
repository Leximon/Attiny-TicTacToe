#include "melody.h"


namespace Melody {

    static const uint16_t *currentMelody = nullptr;
    static uint8_t currentMelodyLength = 0;
    static uint8_t currentMelodyIndex = 0;
    static uint32_t lastNoteTime = 0;

    void init() {
        DDRA |= _BV(BUZZER); // set buzzer as output
        TCCR0A |= _BV(WGM01); // timer0 CTC mode
        TCCR0B |= _BV(CS01); // timer0 prescaler div 8
    }

    void play(const uint16_t *melody, uint8_t length) {
        currentMelody = melody;
        currentMelodyLength = length;
        currentMelodyIndex = 0;

        uint16_t level = pgm_read_word(&melody[1]);
        tone(level);

        lastNoteTime = millis();
    }

    void tryPlayNextNote() {
        if (currentMelody == nullptr) {
            return;
        }

        uint32_t now = millis();
        uint16_t duration = pgm_read_word(&currentMelody[currentMelodyIndex * 2]);
        uint32_t timeSinceLastNote = now - lastNoteTime;

        if (timeSinceLastNote >= duration) {
            currentMelodyIndex++;
            if (currentMelodyIndex >= currentMelodyLength) {
                currentMelody = nullptr;
                currentMelodyLength = 0;
                currentMelodyIndex = 0;
                tone(0);
                return;
            }

            uint16_t level = pgm_read_word(&currentMelody[currentMelodyIndex * 2 + 1]);
            tone(level);

            lastNoteTime = now;
        }
    }

    void tone(uint8_t level) {
        if (level == 0) {
            DDRA &= ~_BV(BUZZER);
            TCCR0A &= ~_BV(COM0B0);
            return;
        }

        DDRA |= _BV(BUZZER);
        TCCR0A |= _BV(COM0B0);
        OCR0A = level;
    }

}