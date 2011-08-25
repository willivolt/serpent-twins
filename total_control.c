/*
 * total_control.c - Total Control Lighting library
 * Copyright 2011 by Tanio Klyce and Ka-Ping Yee
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/*
 * Pinouts are:
 * D0 (header P200 pin 2) -> Total Control CLOCK (green)
 * D1 (header P200 pin 4) -> Total Control DATA (white)
 * +5V (header P200 pin 1) -> Total Control +5V (red)
 * GND (header P200 pin 7) -> Total Control GND (blue)
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "total_control.h"

#define PINCTRL_BASE (0x80018000)

#define PINCTRL_MUXSEL0 (PINCTRL_BASE + 0x100)  /* function select register */
#define PINCTRL_MUXSEL1 (PINCTRL_BASE + 0x110)  /* function select register */
#define PINCTRL_MUXSEL2 (PINCTRL_BASE + 0x120)  /* function select register */
#define PINCTRL_MUXSEL3 (PINCTRL_BASE + 0x130)  /* function select register */
#define PINCTRL_DRIVE0  (PINCTRL_BASE + 0x200)  /* output current register */
#define PINCTRL_PULL0   (PINCTRL_BASE + 0x400)  /* pull-up enable register */
#define PINCTRL_DOUT0   (PINCTRL_BASE + 0x500)  /* data out register */
#define PINCTRL_DOUT1   (PINCTRL_BASE + 0x510)  /* data out register */
#define PINCTRL_DIN0    (PINCTRL_BASE + 0x600)  /* data in register */
#define PINCTRL_DIN1    (PINCTRL_BASE + 0x610)  /* data in register */
#define PINCTRL_DOE0    (PINCTRL_BASE + 0x700)  /* output enable register */
#define PINCTRL_DOE1    (PINCTRL_BASE + 0x710)  /* output enable register */


// On the Chumby board, P200 is a 13x2 header; P401 is a 22x2 header.

// SPI output bits       GPIO bank/bit  CPU pin    Header  Pin   Onboard label
#define CLK  (1<<0)   // bank 0 bit 0   GPMI_D00   P200    2     (D0)
#define D1   (1<<1)   // bank 0 bit 1   GPMI_D01   P200    4     (D1)
#define D2   (1<<2)   // bank 0 bit 2   GPMI_D02   P200    6     (D2)
#define D3   (1<<3)   // bank 0 bit 3   GPMI_D03   P200    8     (D3)
#define D4   (1<<4)   // bank 0 bit 4   GPMI_D04   P200    10    (D4)
#define D5   (1<<5)   // bank 0 bit 5   GPMI_D05   P200    12    (D5)
#define D6   (1<<7)   // bank 0 bit 7   GPMI_D06   P200    14    (D7)
#define D7   (1<<6)   // bank 0 bit 6   GPMI_D07   P200    16    (D6)
#define D8   (1<<28)  // bank 1 bit 28  PWM2       P200    20    (PM2)
#define D9   (1<<30)  // bank 1 bit 30  PWM4       P200    24    (PM4)
#define D10  (1<<14)  // bank 0 bit 14  GPMI_D14   P200    26    (Rx)
#define D11  (1<<15)  // bank 0 bit 15  GPMI_D15   P200    25    (Tx)

// Button input bits     GPIO bank/bit  CPU pin    Header  Pin   Onboard label
#define BTN1 (1<<8)   // bank 1 bit 8   LCD_D08    P401    11    G2
#define BTN2 (1<<9)   // bank 1 bit 9   LCD_D09    P401    12    G3
#define BTN3 (1<<15)  // bank 1 bit 15  LCD_D15    P401    19    R3
#define BTN4 (1<<16)  // bank 1 bit 16  LCD_D16    P401    20    R4

#define MAX_CHANNELS 11
byte ZEROES[MAX_CHANNELS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

typedef struct {
  unsigned int address;
  unsigned int mask;
} channel_pin;
channel_pin CHANNEL_PINS[MAX_CHANNELS] = {
  {PINCTRL_DOUT0, D1},
  {PINCTRL_DOUT0, D2},
  {PINCTRL_DOUT0, D3},
  {PINCTRL_DOUT0, D4},
  {PINCTRL_DOUT0, D5},
  {PINCTRL_DOUT0, D6},
  {PINCTRL_DOUT0, D7},
  {PINCTRL_DOUT1, D8},
  {PINCTRL_DOUT1, D9},
  {PINCTRL_DOUT0, D10},
  {PINCTRL_DOUT0, D11}
};

// Pins in use: bank 0, bits 0-7, 14, and 15 (two bits, 11, for each pin)
#define BANK0_MUXSEL0 (0xf000ffff) // 1111 0000 0000 0000 1111 1111 1111 1111
// Pins in use: bank 0, none of bits 16-31
#define BANK0_MUXSEL1 (0x00000000) // 0000 0000 0000 0000 0000 0000 0000 0000
// Pins in use: bank 1, pins 6, 7, and 15
#define BANK1_MUXSEL2 (0xc000f000) // 1100 0000 0000 0000 1111 0000 0000 0000
// Pins in use: bank 1, pins 16, 28, and 30
#define BANK1_MUXSEL3 (0x330000f3) // 0011 0011 0000 0000 0000 0000 0000 0011

// Pins to enable for output
#define BANK0_OUT_MASK (CLK | D1 | D2 | D3 | D4 | D5 | D6 | D7 | D10 | D11)
#define BANK1_OUT_MASK (D8 | D9)

// Pins to enable for input
#define BANK0_IN_MASK (0)
#define BANK1_IN_MASK (BTN1 | BTN2 | BTN3 | BTN4)


static unsigned int* pinctrl_mem = 0;

void pinctrl_write(unsigned int offset, unsigned int value) {
  int scaled_offset = (offset - (offset & ~0xffff));
  pinctrl_mem[scaled_offset / sizeof(int)] = value;
}

unsigned int pinctrl_read(unsigned int offset) {
  int scaled_offset = (offset - (offset & ~0xffff));
  return pinctrl_mem[scaled_offset / sizeof(int)];
}

#define PINCTRL_SET(address, value) pinctrl_write((address) + 4, value)
#define PINCTRL_CLR(address, value) pinctrl_write((address) + 8, value)


int gpio_init() {
  static int fd = 0;

  fd = open("/dev/mem", O_RDWR);
  if (fd < 0)  {
     perror("Unable to open /dev/mem");
     return -1;
  }

  // Set up a memory map for the PINCTRL registers.
  pinctrl_mem = mmap(0, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED,
             fd, PINCTRL_BASE & ~0xffff);

  // Select the pins we want to use for GPIO.
  PINCTRL_SET(PINCTRL_MUXSEL0, BANK0_MUXSEL0);
  PINCTRL_SET(PINCTRL_MUXSEL1, BANK0_MUXSEL1);
  PINCTRL_SET(PINCTRL_MUXSEL2, BANK1_MUXSEL2);
  PINCTRL_SET(PINCTRL_MUXSEL3, BANK1_MUXSEL3);

  // Initialize the pin values to zero.
  PINCTRL_CLR(PINCTRL_DOUT0, BANK0_OUT_MASK);
  PINCTRL_CLR(PINCTRL_DOUT1, BANK1_OUT_MASK);

  // Enable pins for input and output.
  PINCTRL_SET(PINCTRL_DOE0, BANK0_OUT_MASK);
  PINCTRL_SET(PINCTRL_DOE1, BANK1_OUT_MASK);
  PINCTRL_CLR(PINCTRL_DOE0, BANK0_IN_MASK);
  PINCTRL_CLR(PINCTRL_DOE1, BANK1_IN_MASK);

  return 0;
}

void spi_clock_high() {
  PINCTRL_SET(PINCTRL_DOUT0, CLK);
}

void spi_clock_low() {
  PINCTRL_CLR(PINCTRL_DOUT0, CLK);
}

void spi_write(byte value) {
  byte bit, c;

  spi_clock_high();
  for (bit = 0x80; bit; bit >>= 1) {
    spi_clock_low();
    if (value & bit) {
      PINCTRL_SET(PINCTRL_DOUT0, D1);
    } else {
      PINCTRL_CLR(PINCTRL_DOUT0, D1);
    }
    value <<= 1;
    spi_clock_high();
  }
}

void spi_write_multi(byte* values, int num_channels) {
  byte bit, c;

  spi_clock_high();
  for (bit = 0x80; bit; bit >>= 1) {
    spi_clock_low();
    for (c = 0; c < num_channels; c++) {
      if (values[c] & bit) {
        PINCTRL_SET(CHANNEL_PINS[c].address, CHANNEL_PINS[c].mask);
      } else {
        PINCTRL_CLR(CHANNEL_PINS[c].address, CHANNEL_PINS[c].mask);
      }
    }
    spi_clock_high();
  }
}

// Initialize Total Control Lighting.
void tcl_init() {
  if (gpio_init() != 0) {
    fprintf(stderr, "Failed to initialize GPIO.\n");
    exit(1);
  }
}

// Start a new pixel sequence.
void tcl_start() { 
  spi_write(0);
  spi_write(0);
  spi_write(0);
  spi_write(0);
}

// Start a new pixel sequence on many channels at once.
void tcl_start_multi(int num_channels) {
  spi_write_multi(ZEROES, num_channels);
  spi_write_multi(ZEROES, num_channels);
  spi_write_multi(ZEROES, num_channels);
  spi_write_multi(ZEROES, num_channels);
}

// Send a single pixel in a sequence.
void tcl_put_pixel(byte red, byte green, byte blue) {
  byte flag = (red & 0xc0) >> 6 | (green & 0xc0) >> 4 | (blue & 0xc0) >> 2;
  spi_write(~flag);
  spi_write(blue);
  spi_write(green);
  spi_write(red);
}

// Send multiple pixels in parallel to separate channels.
void tcl_put_pixel_multi(byte** pixel_ptrs, int num_channels) {
  int j;
  byte flags[MAX_CHANNELS];
  byte reds[MAX_CHANNELS];
  byte greens[MAX_CHANNELS];
  byte blues[MAX_CHANNELS];

  if (num_channels > MAX_CHANNELS) {
    num_channels = MAX_CHANNELS;
  }
  for (j = 0; j < num_channels; j++) {
    reds[j] = pixel_ptrs[j][0];
    greens[j] = pixel_ptrs[j][1];
    blues[j] = pixel_ptrs[j][2];
    flags[j] = ~((reds[j] & 0xc0) >> 6 |
                 (greens[j] & 0xc0) >> 4 |
                 (blues[j] & 0xc0) >> 2);
  }
  spi_write_multi(flags, num_channels);
  spi_write_multi(blues, num_channels);
  spi_write_multi(greens, num_channels);
  spi_write_multi(reds, num_channels);
}

// Send an entire sequence of n pixels, given n*3 bytes of colour data.
void tcl_put_pixels(byte* pixels, int n) {
  int i;

  tcl_start();
  for (i = 0; i < n; i++) {
    tcl_put_pixel(pixels[0], pixels[1], pixels[2]);
    pixels += 3;
  }
}

// Send multiple channels of pixels in parallel, n*3 bytes for each channel.
void tcl_put_pixels_multi(byte** pixel_ptrs, int num_channels, int num_pixels) {
  int i, j;
  byte* ptrs[MAX_CHANNELS];

  if (num_channels > MAX_CHANNELS) {
    num_channels = MAX_CHANNELS;
  }
  for (j = 0; j < num_channels; j++) {
    ptrs[j] = pixel_ptrs[j];
  }
  tcl_start_multi(num_channels);
  for (i = 0; i < num_pixels; i++) {
    tcl_put_pixel_multi(ptrs, num_channels);
    for (j = 0; j < num_channels; j++) {
      ptrs[j] += 3;
    }
  }
}

// Read a button (b = 1, 2, 3, or 4).
byte tcl_read_button(byte b) {
  switch (b) {
    case 1:
      return (pinctrl_read(PINCTRL_DIN1) & BTN1) ? 1 : 0;
    case 2:
      return (pinctrl_read(PINCTRL_DIN1) & BTN2) ? 1 : 0;
    case 3:
      return (pinctrl_read(PINCTRL_DIN1) & BTN3) ? 1 : 0;
    case 4:
      return (pinctrl_read(PINCTRL_DIN1) & BTN4) ? 1 : 0;
  }
}
