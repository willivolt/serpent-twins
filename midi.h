#ifndef TYPEDEF_BYTE
#define TYPEDEF_BYTE
typedef unsigned char byte;
#endif

void midi_init();
void midi_poll();
byte midi_get_note(byte note);
void midi_set_note(byte note, byte velocity);
byte midi_get_control(byte control);
void midi_set_control(byte control, byte value);
void midi_set_control_with_pickup(byte control, byte value);
