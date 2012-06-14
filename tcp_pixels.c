#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/time.h>
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
    result = send(sock, buf + sent, len - sent, 0);
    if (result < 0) {
      perror("Write error");
      close(sock);
      sock = -1;
    }
    sent += result;
  }
}

byte tcp_buffer[1000*3];

void tcp_put_pixels(byte address, byte* pixels, int n) {
  byte header[4];
  int length = n*3;
  byte* dest;
  int i;
  for (i = 0; i < n; i++) {
    *dest++ = pixels[2];  // blue
    *dest++ = pixels[1];  // green
    *dest++ = pixels[0];  // red
    pixels += 3;
  }
  header[0] = address;
  header[1] = 0;
  header[2] = length >> 8;
  header[3] = length & 0xff;
  tcp_send_sync(header, 4);
  tcp_send_sync(tcp_buffer, length);
}
