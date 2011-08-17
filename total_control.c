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
 * D0 (header P200 pin 2) -> Total Control CLOCK
 * D1 (header P200 pin 4) -> Total Control DATA
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "total_control.h"

#define PINCTRL_BASE (0x80018000)

#define PINCTRL_MUXSEL0 (PINCTRL_BASE + 0x100)
#define PINCTRL_DRIVE0 (PINCTRL_BASE + 0x200)
#define PINCTRL_PULL0 (PINCTRL_BASE + 0x400)
#define PINCTRL_DOUT0 (PINCTRL_BASE + 0x500)
#define PINCTRL_DOE0 (PINCTRL_BASE + 0x700)

static unsigned int* pinctrl_mem = 0;

void pinctrl_write(unsigned int offset, unsigned int value) {
  int scaled_offset = (offset - (offset & ~0xffff));
  pinctrl_mem[scaled_offset / sizeof(int)] = value;
}

#define PINCTRL_SET(address, value) pinctrl_write((address) + 4, value)
#define PINCTRL_CLR(address, value) pinctrl_write((address) + 8, value)

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

  // Set bank 0 pins 0-7 (D0-D7, aka GPMI_D00-GPMI_D07) to GPIO.
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

// Initialize Total Control Lighting.
void tcl_init() {
  if (gpio_init() != 0) {
    fprintf(stderr, "Failed to initialize GPIO.\n");
    exit(1);
  }
}

// Start a new pixel sequence.
void tcl_start() { 
  spi_write(0x00);
  spi_write(0x00);
  spi_write(0x00);
  spi_write(0x00);
}

// Send a single pixel in a sequence.
void tcl_put_pixel(byte red, byte green, byte blue) {
  byte flag = (red & 0xc0) >> 6 | (green & 0xc0) >> 4 | (blue & 0xc0) >> 2;
  spi_write(~flag);
  spi_write(blue);
  spi_write(green);
  spi_write(red);
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
