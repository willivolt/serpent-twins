#include <sys/fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "midi.h"

int midi_in = -1, midi_out = -1;
byte midi_note[256];
byte midi_control[256];
byte midi_control_position[256];
byte midi_control_awaiting_pickup[256];

void midi_init() {
  int i;
  for (i = 0; i < 256; i++) {
    midi_note[i] = 0;
    midi_control[i] = 0;
    midi_control_position[i] = 255;
    midi_control_awaiting_pickup[i] = 0;
  }
}

void midi_close() {
  if (midi_in >= 0) {
    close(midi_in);
    midi_in = -1;
    printf("\nclosed /tmp/midi.in\n");
  }
  if (midi_out >= 0) {
    close(midi_out);
    midi_out = -1;
    printf("\nclosed /tmp/midi.out\n");
  }
}

void midi_poll() {
  fd_set rfds;
  struct timeval tv;
  int result;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  byte command;
  byte index;
  byte value;

  if (midi_in < 0) {
    midi_in = open("/tmp/midi.in", O_RDONLY | O_NONBLOCK);
    if (midi_in > 0) printf("\nopened /tmp/midi.in\n");
  }
  if (midi_out < 0) {
    midi_out = open("/tmp/midi.out", O_WRONLY | O_NONBLOCK);
    if (midi_out > 0) printf("\nopened /tmp/midi.out\n");
  }
  FD_ZERO(&rfds);
  while (midi_in >= 0) {
    FD_SET(midi_in, &rfds);
    result = select(midi_in + 1, &rfds, NULL, NULL, &tv);
    if (result <= 0) {
      break;
    }
    if (read(midi_in, &command, 1) == 1) {
      if (command >= 0x80 && command <= 0xbf ||
          command >= 0xe0 && command <= 0xef ||
          command == 0xf2) {
        if (read(midi_in, &index, 1) == 1 &&
            read(midi_in, &value, 1) == 1) {
          if ((command & 0xf0) == 0x80) {
            midi_note[index] = 0;
          } else if ((command & 0xf0) == 0x90) {
            midi_note[index] = value;
          } else if ((command & 0xf0) == 0xb0) {
            if (midi_control_awaiting_pickup[index]) {
              byte last_position = midi_control_position[index];
              byte pickup_value = midi_control[index];
              if (last_position != 255) {
                if (last_position <= pickup_value && value >= pickup_value ||
                    last_position >= pickup_value && value <= pickup_value) {
                  midi_control_awaiting_pickup[index] = 0;
                }
              }
            }
            if (!midi_control_awaiting_pickup[index]) {
              midi_control[index] = value;
            }
            midi_control_position[index] = value;
          }
        } else midi_close();
      } else if (command >= 0xc0 && command <= 0xdf ||
                 command == 0xf1 || command == 0xf3) {
        if (read(midi_in, &value, 1)) {
          ;
        } else midi_close();
      } else if (command == 0xf0) {  // system exclusive
        while (value != 0xf7) {
          if (read(midi_in, &value, 1)) {
            continue;
          } else midi_close();
        }
      }
    } else midi_close();
  }
  if (result < 0) {
    midi_close();
  }
}

byte midi_get_note(byte note) {
  return midi_note[note];
}

void midi_set_note(byte note, byte velocity) {
  byte message[3];
  message[0] = (velocity == 0) ? 0x80 : 0x90;
  message[1] = note;
  message[2] = (velocity == 0) ? 0x7f : velocity;
  write(midi_out, message, 3);
  midi_note[note] = velocity;
}

byte midi_get_control(byte control) {
  return midi_control[control];
}

void midi_set_control(byte control, byte value) {
  byte message[3];
  message[0] = 0xb0;
  message[1] = control;
  message[2] = value;
  write(midi_out, message, 3);
  midi_control[control] = value;
}

void midi_set_control_with_pickup(byte control, byte value) {
  midi_set_control(control, value);
  midi_control_awaiting_pickup[control] = 1;
}
