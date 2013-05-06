#include <sys/fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "midi.h"

#define MIDI_FD_MAX 4
int midi_ins[MIDI_FD_MAX] = {-1, -1, -1, -1};
int midi_outs[MIDI_FD_MAX] = {-1, -1, -1, -1};
char midi_in_filenames[MIDI_FD_MAX][20] = {
  "/tmp/midi1.in", "/tmp/midi2.in", "/tmp/midi3.in", "/tmp/midi4.in"
};
char midi_out_filenames[MIDI_FD_MAX][20] = {
  "/tmp/midi1.out", "/tmp/midi2.out", "/tmp/midi3.out", "/tmp/midi4.out"
};
byte midi_note[256];
byte midi_clicked[256];
byte midi_click_held[256];
byte midi_control[256];
byte midi_control_position[256];
byte midi_control_awaiting_pickup[256];

void midi_init() {
  int i;
  for (i = 0; i < MIDI_FD_MAX; i++) {
    midi_ins[i] = -1;
    midi_outs[i] = -1;
  }
  for (i = 0; i < 256; i++) {
    midi_note[i] = 0;
    midi_clicked[i] = 0;
    midi_click_held[i] = 0;
    midi_control[i] = 0;
    midi_control_position[i] = 255;
    midi_control_awaiting_pickup[i] = 0;
  }
}

void midi_close_device(int d) {
  int closed = 0;

  if (midi_ins[d] >= 0) {
    close(midi_ins[d]);
    midi_ins[d] = -1;
  }
  if (midi_outs[d] >= 0) {
    close(midi_outs[d]);
    midi_outs[d] = -1;
  }
}

void midi_poll_device(int d) {
  fd_set rfds;
  struct timeval tv;
  int result;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  byte command;
  byte index;
  byte value;

  FD_ZERO(&rfds);
  while (midi_ins[d] >= 0) {
    FD_SET(midi_ins[d], &rfds);
    result = select(midi_ins[d] + 1, &rfds, NULL, NULL, &tv);
    if (result <= 0) {
      break;
    }
    if (read(midi_ins[d], &command, 1) == 1) {
      if (command >= 0x80 && command <= 0xbf ||
          command >= 0xe0 && command <= 0xef ||
          command == 0xf2) {
        printf("midi: [%02x]\n", command);
        if (read(midi_ins[d], &index, 1) == 1 &&
            read(midi_ins[d], &value, 1) == 1) {
          printf("midi: [index %02x value %02x]\n", index, value);
          if ((command & 0xf0) == 0x80) {
            midi_note[index] = 0;
            midi_click_held[index] = 0;
          } else if ((command & 0xf0) == 0x90) {
            midi_note[index] = value;
            midi_clicked[index] = value;
            midi_click_held[index] = value;
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
        } else midi_close_device(d);
      } else if (command >= 0xc0 && command <= 0xdf ||
                 command == 0xf1 || command == 0xf3) {
        if (read(midi_ins[d], &value, 1)) {
          ;
        } else midi_close_device(d);
      } else if (command == 0xf0) {  // system exclusive
        while (value != 0xf7) {
          if (read(midi_ins[d], &value, 1)) {
            continue;
          } else midi_close_device(d);
        }
      }
    } else midi_close_device(d);
  }
  if (result < 0) {
    midi_close_device(d);
  }
}

void midi_poll() {
  int d;

  for (d = 0; d < MIDI_FD_MAX; d++) {
    if (midi_ins[d] < 0) {
      midi_ins[d] = open(midi_in_filenames[d], O_RDONLY | O_NONBLOCK);
    }
    if (midi_outs[d] < 0) {
      midi_outs[d] = open(midi_out_filenames[d], O_WRONLY | O_NONBLOCK);
    }
    midi_poll_device(d);
  }
}

byte midi_get_note(byte note) {
  return midi_note[note];
}

void midi_set_note(byte note, byte velocity) {
  int d;
  byte message[3] = {
    (velocity == 0) ? 0x80 : 0x90,
    note,
    (velocity == 0) ? 0x7f : velocity
  };

  for (d = 0; d < MIDI_FD_MAX; d++) {
    if (midi_outs[d] >= 0) {
      write(midi_outs[d], message, 3);
    }
  }
  midi_note[note] = velocity;
}

byte midi_clicks_finished() {
  int i;
  for (i = 0; i < 256; i++) {
    if (midi_click_held[i]) {
      return 0;
    }
  }
  return 1;
}

byte midi_get_click(byte note) {
  return midi_clicked[note];
}

void midi_clear_clicks() {
  int i;
  for (i = 0; i < 256; i++) {
    midi_clicked[i] = 0;
  }
}

byte midi_get_control(byte control) {
  return midi_control[control];
}

void midi_set_control(byte control, byte value) {
  int d;
  byte message[3] = {0xb0, control, value};

  for (d = 0; d < MIDI_FD_MAX; d++) {
    if (midi_outs[d] >= 0) {
      write(midi_outs[d], message, 3);
    }
  }
  midi_control[control] = value;
}

void midi_set_control_with_pickup(byte control, byte value) {
  midi_set_control(control, value);
  midi_control_awaiting_pickup[control] = 1;
}
