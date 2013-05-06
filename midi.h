#ifndef TYPEDEF_BYTE
#define TYPEDEF_BYTE
typedef unsigned char byte;
#endif

// Initializes the MIDI subsystem.  Call this before any of the other functions.
void midi_init();

// Checks for and processes incoming MIDI messages.  Call this periodically.
// Messages are read from /tmp/midi.in and sent to /tmp/midi.out; these files
// are checked on every call to midi_poll() and reopened if necessary.
void midi_poll();

// Returns 0 if the note is off, or a velocity from 1 to 127 if the note is on.
// Notes can be turned on/off by incoming MIDI messages or by midi_set_note().
byte midi_get_note(byte note);

// Sets the velocity of a note and sends a MIDI Note On or Note Off message.
void midi_set_note(byte note, byte velocity);

// Returns 1 if corresponding Note Off messages have been received for all Note
// On messages since the last time midi_clear_clicks() was called.  To detect
// chords, wait for midi_clicks_finished(), then check midi_get_click() for
// each note, and finally call midi_clear_clicks().
byte midi_clicks_finished();

// Returns 1 if a Note On message has been received for this note since the
// last time midi_clear_clicks() has been called.  Only incoming messages cause
// clicks to be registered; midi_set_note() does not.
byte midi_get_click(byte note);

// Clears "clicked" flags for all notes, causing midi_get_click() to return 0.
void midi_clear_clicks();

// Returns the current value of a MIDI control, a value from 0 to 127.
byte midi_get_control(byte control);

#define midi_get_control_linear(control, min, max) \
    ((min) + (midi_get_control(control)/127.0)*(max - min))

#define midi_get_control_exp(control, min, max) \
    ((min) * pow((max)/(min), midi_get_control(control)/127.0))

// Sets the value of a MIDI control, and sends a MIDI Control Change message.
// Any further incoming MIDI Control Change messages for this control will
// immediately affect the value returned by midi_get_control().
void midi_set_control(byte control, byte value);

// Sets the value of a MIDI control, and sends a MIDI Control Change message.
// Any further incoming MIDI Control Change messages for this control will not
// affect the value returned by midi_get_control() until the incoming control
// value sweeps past this set value.
void midi_set_control_with_pickup(byte control, byte value);
