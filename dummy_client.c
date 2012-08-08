#include <stdio.h>
#include <string.h>
#include "opc.h"

int main(int argc, char** argv) {
  int address;
  char buffer[1000];
  char* token;
  pixel pixels[1000];
  int c, chars;
  u32 hex;
  u16 count;
  u16 port;
  opc_sink s;
  int i;
  char* sep;

  port = argc > 2 ? atoi(argv[2]) : 0;
  if (!port) {
    fprintf(stderr, "Usage: %s <server-ip-address> <port>\n", argv[0]);
    return 1;
  }

  s = opc_new_sink(argv[1], port);
  while (s >= 0 && fgets(buffer, 1000, stdin)) {
    sscanf(buffer, "%d%n", &address, &c);
    count = 0;
    token = strtok(buffer + c, " \t\r\n");
    while (token) {
      c += chars;
      if (strlen(token) == 3) {
        hex = strtol(token, NULL, 16);
        pixels[count].r = ((hex & 0xf00) >> 8) * 0x11;
        pixels[count].g = ((hex & 0x0f0) >> 4) * 0x11;
        pixels[count].b = ((hex & 0x00f) >> 0) * 0x11;
      } else if (strlen(token) == 6) {
        hex = strtol(token, NULL, 16);
        pixels[count].r = (hex & 0xff0000) >> 16;
        pixels[count].g = (hex & 0x00ff00) >> 8;
        pixels[count].b = (hex & 0x0000ff) >> 0;
      } else {
      }
      count++;
      token = strtok(NULL, " \t\r\n");
    }
    printf("<- address %d: %d pixel%s", address, count, count == 1 ? "" : "s");
    sep = ":";
    for (i = 0; i < count; i++) {
      if (i >= 4) {
        printf(", ...");
        break;
      }
      printf("%s %02x %02x %02x", sep, pixels[i].r, pixels[i].g, pixels[i].b);
      sep = ",";
    }
    printf("\n");
    opc_put_pixels(s, address, count, pixels);
  }
}
