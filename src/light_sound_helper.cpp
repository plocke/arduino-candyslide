#include "light_sound_helper.h"
#include <Arduino.h>

const int NOTE_DELAY = 200;

// In loop()
void playBonusTune(int speakerPin) {

    // Play tune on speaker  
    tone(speakerPin, 440); // A4 note
    delay(NOTE_DELAY);
    tone(speakerPin, 494); // B4 note
    delay(NOTE_DELAY);  
    tone(speakerPin, 523); // C5 note 
    delay(NOTE_DELAY);

    noTone(speakerPin); // Stop sound
  

}
