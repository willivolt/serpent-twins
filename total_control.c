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
#define PINCTRL_DRIVE0 (PINCTRL_BASE + 0x200)  /* output current register */
#define PINCTRL_PULL0 (PINCTRL_BASE + 0x400)  /* pull-up enable register */
#define PINCTRL_DOUT0 (PINCTRL_BASE + 0x500)  /* data out register */
#define PINCTRL_DOE0 (PINCTRL_BASE + 0x700)  /* output enable register */

static unsigned int* pinctrl_mem = 0;

void pinctrl_write(unsigned int offset, unsigned int value) {
  int scaled_offset = (offset - (offset & ~0xffff));
  pinctrl_mem[scaled_offset / sizeof(int)] = value;
}

#define PINCTRL_SET(address, value) pinctrl_write((address) + 4, value)
#define PINCTRL_CLR(address, value) pinctrl_write((address) + 8, value)

#define MAX_STRANDS 7

byte ZEROES[MAX_STRANDS] = {0, 0, 0, 0, 0, 0, 0};
unsigned int STRAND_ADDRESS[MAX_STRANDS] = {
  PINCTRL_DOUT0, PINCTRL_DOUT0, PINCTRL_DOUT0, PINCTRL_DOUT0,
  PINCTRL_DOUT0, PINCTRL_DOUT0, PINCTRL_DOUT0,
};
unsigned int STRAND_MASK[MAX_STRANDS] = {2, 4, 8, 16, 32, 64, 128};


int gpio_init() {
  static int fd = 0;

  fd = open("/dev/mem", O_RDWR);
  if (fd < 0)  {
     perror("Unable to open /dev/mem.");
     return -1;
  }

  // Set up a memory map for the PINCTRL registers.
  pinctrl_mem = mmap(0, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED,
             fd, PINCTRL_BASE & ~0xffff);

  // Set pins D0 - D7 (bank 0 pins 0-7) to GPIO.  (2 bits = 0b11 for each pin.)
  PINCTRL_SET(PINCTRL_MUXSEL0, 0x0000ffff);
  
  // Initialize GPIO pins to 0.
  PINCTRL_CLR(PINCTRL_DOUT0, 0x000000ff);
  
  // Enable output for pins D0-D7.
  PINCTRL_SET(PINCTRL_DOE0, 0x000000ff);

  return 0;
}

void spi_clock_tick() {
  PINCTRL_SET(PINCTRL_DOUT0, 1);  // set pin D0
  PINCTRL_CLR(PINCTRL_DOUT0, 1);  // clear pin D0
}

void spi_write(byte value) {
  int i;
  for (i = 0; i < 8; i++) {
    if (value & 0x80) {
      PINCTRL_SET(PINCTRL_DOUT0, 2);  // set pin D1
    } else {
      PINCTRL_CLR(PINCTRL_DOUT0, 2);  // clear pin D1
    }
    value <<= 1;
    spi_clock_tick();
  }
}

void spi_write_multi(byte* values, int num_strands) {
  int i, j, bit;

  for (i = 0, bit = 0x80; i < 8; i++, bit >>= 1) {
    for (j = 0; j < num_strands; j++) {
      if (values[j] & bit) {
        PINCTRL_SET(STRAND_ADDRESS[j], STRAND_MASK[j]);
      } else {
        PINCTRL_CLR(STRAND_ADDRESS[j], STRAND_MASK[j]);
      }
    }
    spi_clock_tick();
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

// Start a new pixel sequence on many strands at once.
void tcl_start_multi(int num_strands) {
  spi_write_multi(ZEROES, num_strands);
  spi_write_multi(ZEROES, num_strands);
  spi_write_multi(ZEROES, num_strands);
  spi_write_multi(ZEROES, num_strands);
}

// Send a single pixel in a sequence.
void tcl_put_pixel(byte red, byte green, byte blue) {
  byte flag = (red & 0xc0) >> 6 | (green & 0xc0) >> 4 | (blue & 0xc0) >> 2;
  spi_write(~flag);
  spi_write(blue);
  spi_write(green);
  spi_write(red);
}

// Send multiple pixels in parallel to separate strands.
void tcl_put_pixel_multi(byte** pixel_ptrs, int num_strands) {
  int j;
  byte flags[MAX_STRANDS];
  byte reds[MAX_STRANDS];
  byte greens[MAX_STRANDS];
  byte blues[MAX_STRANDS];

  if (num_strands > MAX_STRANDS) {
    num_strands = MAX_STRANDS;
  }
  for (j = 0; j < num_strands; j++) {
    reds[j] = pixel_ptrs[j][0];
    greens[j] = pixel_ptrs[j][1];
    blues[j] = pixel_ptrs[j][2];
    flags[j] = ~((reds[j] & 0xc0) >> 6 |
                 (greens[j] & 0xc0) >> 4 |
                 (blues[j] & 0xc0) >> 2);
  }
  spi_write_multi(flags, num_strands);
  spi_write_multi(blues, num_strands);
  spi_write_multi(greens, num_strands);
  spi_write_multi(reds, num_strands);
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

// Send multiple strands of pixels in parallel, n*3 bytes for each strand.
void tcl_put_pixels_multi(byte** pixel_ptrs, int num_strands, int num_pixels) {
  int i, j;
  byte* ptrs[MAX_STRANDS];

  if (num_strands > MAX_STRANDS) {
    num_strands = MAX_STRANDS;
  }
  for (j = 0; j < num_strands; j++) {
    ptrs[j] = pixel_ptrs[j];
  }
  tcl_start_multi(num_strands);
  for (i = 0; i < num_pixels; i++) {
    tcl_put_pixel_multi(ptrs, num_strands);
    for (j = 0; j < num_strands; j++) {
      ptrs[j] += 3;
    }
  }
}
