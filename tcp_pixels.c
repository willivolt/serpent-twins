#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef TYPEDEF_BYTE
#define TYPEDEF_BYTE
typedef unsigned char byte;
#endif

int sock = -1;

int tcp_init(char* ipaddr, int port) {
  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if (inet_pton(AF_INET, ipaddr, &(address.sin_addr)) != 1) {
    fprintf(stderr, "Invalid IP address: %s\n", ipaddr);
    return -1;
  }
  if (connect(sock, (struct sockaddr*) &address, sizeof(address)) != 0) {
    fprintf(stderr, "Could not connect to %s port %d\n", ipaddr, port);
    perror("Error");
    return -1;
  }
  return sock;
}

void tcp_put_pixels(byte address, byte* pixels, int n) {
  byte header[4];
  int length = n*3;
  header[0] = address;
  header[1] = 0;
  header[2] = length >> 8;
  header[3] = length & 0xff;
  write(sock, header, 4);
  write(sock, pixels, length);
}

void tcp_put_pixels_multi(byte** pixel_ptrs, int num_channels, int num_pixels) {
  for (int i = 0; i < num_channels; i++) {
    tcp_put_pixels(i, pixel_ptrs[i], num_pixels);
  }
}
