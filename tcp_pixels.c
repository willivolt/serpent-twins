#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "midi.h"
#include "tcp_pixels.h"

#ifndef TYPEDEF_BYTE
#define TYPEDEF_BYTE
typedef unsigned char byte;
#endif

#define ADDRESS_BROADCAST 0

enum psoc_channel {
  CHANNEL_LED = 0,
  CHANNEL_COMMAND = 1
};

enum psoc_command {
  CMD_NONE = 0,
  CMD_BOOTLOAD = 1,
  CMD_FORGET = 2,
  CMD_PREPARE = 3,
  CMD_COMMIT = 4,
  CMD_TESTPATTERN = 5
};

int sock = -1;

int tcp_connect(char* ipaddr, int port) {
  char buffer[100];
  int sock = -1;
  struct sockaddr_in address;
  struct timeval timeout;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if (inet_pton(AF_INET, ipaddr, &(address.sin_addr)) != 1) {
    fprintf(stderr, "Invalid IP address: %s\n", ipaddr);
    return -1;
  }
  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  /* Set a short send timeout in case we get stuck. */
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

  /* Connect to the master PSoC board. */
  if (connect(sock, (struct sockaddr*) &address, sizeof(address)) != 0) {
    close(sock);
    sprintf(buffer, "Could not connect to %s port %d", ipaddr, port);
    perror(buffer);
    return -1;
  }
  fprintf(stderr, "Connected to %s port %d\n", ipaddr, port);
  return sock;
}

void tcp_init() {
  FILE* fp;
  int count;
  char ipaddr[20];
  int port;

  /* Ignore the SIGPIPE that occurs when we write to a closed socket. */
  signal(SIGPIPE, SIG_IGN);

  while (sock < 0) {
    usleep(50000); /* 0.05 sec */
    midi_set_note(16, 127);
    fp = fopen("/tmp/serpent", "r");
    if (!fp) {
      perror("Cannot read /tmp/serpent");
      continue;
    }
    count = fscanf(fp, "%16s %d", ipaddr, &port);
    fclose(fp);
    if (count != 2) {
      fprintf(stderr, "Could not parse /tmp/serpent\n");
      continue;
    }
    sock = tcp_connect(ipaddr, port);
  }
  tcp_init_addresses();
}

void tcp_send_sync(const byte* buf, int len) {
  int sent = 0;
  int result;
  if (sock < 0) {
    tcp_init();
  }
  while (sock >= 0 && sent < len) {
    result = send(sock, buf + sent, len - sent, 0);
    if (result < 0) {
      perror("Write error");
      close(sock);
      sock = -1;
    }
    sent += result;
  }
}

void tcp_init_addresses() {
  byte command[6];
  byte address;

  // Tell all nodes to forget their addresses.
  command[0] = ADDRESS_BROADCAST;
  command[1] = CHANNEL_COMMAND;
  command[2] = 0;  // data length = 1 byte
  command[3] = 1;  // data length = 1 byte
  command[4] = CMD_FORGET;
  tcp_send_sync(command, 5);

  // Assign addresses to each node.
  command[0] = ADDRESS_BROADCAST;
  command[1] = CHANNEL_COMMAND;
  command[2] = 0;  // data length = 2 bytes
  command[3] = 2;  // data length = 2 bytes
  for (address = 1; address < 63; address++) {
    command[4] = CMD_PREPARE;
    command[5] = address;
    tcp_send_sync(command, 6);

    command[4] = CMD_COMMIT;
    command[5] = address;
    tcp_send_sync(command, 6);
  }
}

byte tcp_buffer[1000*3];

void tcp_put_pixels(byte address, byte* pixels, int n) {
  byte header[4];
  int length = n*3 + 3;
  byte* dest = tcp_buffer;
  int i;
  for (i = 0; i < n; i++) {
    *dest++ = pixels[2];  // blue
    *dest++ = pixels[1];  // green
    *dest++ = pixels[0];  // red
    pixels += 3;
  }
  // Sometimes the last pixel doesn't get set; add three bytes for reliability.
  *dest++ = 0;
  *dest++ = 0;
  *dest++ = 0;
  header[0] = address;
  header[1] = CHANNEL_LED;
  header[2] = length >> 8;
  header[3] = length & 0xff;
  tcp_send_sync(header, 4);
  tcp_send_sync(tcp_buffer, length);
}
