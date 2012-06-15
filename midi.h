#ifndef TYPEDEF_BYTE
#define TYPEDEF_BYTE
typedef unsigned char byte;
#endif

extern byte midi_note[256];
extern byte midi_control[256];

void midi_init();
void midi_poll();
#define midi_get_note(note) midi_note[note]
void midi_set_note(byte note, byte velocity);
#define midi_get_control(control) midi_control[control]
void midi_set_control(byte control, byte value);
void midi_set_control_with_pickup(byte control, byte value);
