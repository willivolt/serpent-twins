#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#ifndef TYPEDEF_BYTE
#define TYPEDEF_BYTE
typedef unsigned char byte;
#endif

int sock = -1;

int tcp_connect(char* ipaddr, int port) {
  char buffer[100];
  int sock = -1;
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if (inet_pton(AF_INET, ipaddr, &(address.sin_addr)) != 1) {
    fprintf(stderr, "Invalid IP address: %s\n", ipaddr);
    return -1;
  }
  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
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
}

void tcp_send_sync(const byte* buf, int len) {
  int sent = 0;
  int result;
  while (sock >= 0 && sent < len) {
    result = send(sock, buf + sent, len - sent, MSG_DONTWAIT);
    if (result < 0) {
      close(sock);
      sock = -1;
    }
    sent += result;
  }
}

void tcp_put_pixels(byte address, byte* pixels, int n) {
  byte header[4];
  int length = n*3;
  header[0] = address;
  header[1] = 0;
  header[2] = length >> 8;
  header[3] = length & 0xff;
  tcp_send_sync(header, 4);
  tcp_send_sync(pixels, length);
}

void tcp_put_pixels_multi(byte** pixel_ptrs, int num_channels, int num_pixels) {
  for (int i = 0; i < num_channels; i++) {
    tcp_put_pixels(i, pixel_ptrs[i], num_pixels);
  }
}
