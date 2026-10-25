// light_sound_helper.h

#ifndef LIGHT_SOUND_HELPER_H
#define LIGHT_SOUND_HELPER_H


// When soundEnabled is false the speaker stays silent but the note timing still runs,
// so anything relying on the tune's duration behaves the same with sound on or off.
void playBonusTune(int speakerPin, bool soundEnabled);

#endif