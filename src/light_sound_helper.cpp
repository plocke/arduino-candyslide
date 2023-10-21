#include "light_sound_helper.h"
#include <Arduino.h>


const int HAPPY_NOTES[] = {262, 294, 330, 349}; // C D E F
const int HAPPY_TEMPO = 100;

void playHappyTune(int speakerPin)
{

    for (int i = 0; i < 4; i++)
    {
        // Play each note
        tone(speakerPin, HAPPY_NOTES[i]);
        delay(HAPPY_TEMPO);
        noTone(speakerPin);
        delay(HAPPY_TEMPO);
    }

    // Repeat
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            tone(speakerPin, HAPPY_NOTES[j]);
            delay(HAPPY_TEMPO);
            noTone(speakerPin);
            delay(HAPPY_TEMPO);
        }
    }
}


void playBonusTune(int speakerPin)
{

    playHappyTune(speakerPin);
}