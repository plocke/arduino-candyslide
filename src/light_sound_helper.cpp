#include "light_sound_helper.h"
#include <Arduino.h>

#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880

int melody[] = {
    NOTE_C5, NOTE_D5, NOTE_E5, NOTE_G5, NOTE_G5,
    NOTE_A5, NOTE_G5, NOTE_F5, NOTE_E5, NOTE_C5
};

int noteDurations[] = {
    4, 4, 4, 4, 8,
    8, 8, 8, 4, 4
};

void playHappyTune(int speakerPin)
{
    for (int thisNote = 0; thisNote < sizeof(melody) / sizeof(int); thisNote++) {
        int noteDuration = 1000 / noteDurations[thisNote];
        tone(speakerPin, melody[thisNote], noteDuration);

        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);

        noTone(speakerPin);
    }
}

void playBonusTune(int speakerPin)
{
    playHappyTune(speakerPin);
}