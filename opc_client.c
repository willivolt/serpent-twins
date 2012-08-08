#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include "opc.h"

/* If connection fails, try again in 0.1 seconds. */
#define OPC_RECONNECT_DELAY_US 100000

/* Internal structure for a sink.  sock >= 0 iff the connection is open. */
typedef struct {
  struct sockaddr_in address;
  int sock;
} opc_sink_info;

static opc_sink_info opc_sinks[OPC_MAX_SINKS];
static opc_sink opc_next_sink = 0;

/* TODO(kpy): Add support for named hosts. */
opc_sink opc_new_sink(char* hostname, u16 port) {
  opc_sink_info* info;

  /* Allocate an opc_sink_info entry. */
  if (opc_next_sink >= OPC_MAX_SINKS) {
    fprintf(stderr, "OPC: No more sinks available\n");
    return -1;
  }
  info = &opc_sinks[opc_next_sink];

  info->sock = -1;
  info->address.sin_family = AF_INET;
  info->address.sin_port = htons(port);
  if (inet_pton(AF_INET, hostname, &(info->address.sin_addr)) != 1) {
    fprintf(stderr, "OPC: Invalid IP address: %s\n", hostname);
    return -1;
  }

  /* Increment opc_next_sink only if we were successful. */
  return opc_next_sink++;
}

void opc_connect(opc_sink sink) {
  int sock;
  struct timeval timeout;
  opc_sink_info* info = &opc_sinks[sink];
  char buffer[64];

  if (sink < 0 || sink >= opc_next_sink) {
    fprintf(stderr, "OPC: Sink %d does not exist\n", sink);
    return;
  }
  if (info->sock < 0) {  /* not connected */
    while (1) {
      /* Create the socket. */
      sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

      /* Set a short send timeout in case we get stuck. */
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
      setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

      /* Connect the socket. */
      if (connect(sock, (struct sockaddr*) &(info->address),
                  sizeof(info->address)) == 0) {
        inet_ntop(AF_INET, &(info->address.sin_addr), buffer, 64);
        fprintf(stderr, "OPC: Connected to %s port %d\n",
                buffer, ntohs(info->address.sin_port));
        info->sock = sock;
        return;
      }

      inet_ntop(AF_INET, &(info->address.sin_addr), buffer, 64);
      fprintf(stderr, "OPC: Failed to connect to %s port %d: ",
              buffer, ntohs(info->address.sin_port));
      perror(NULL);
      close(sock);

      usleep(OPC_RECONNECT_DELAY_US);
    }
  }
}

static void opc_close(opc_sink sink) {
  opc_sink_info* info = &opc_sinks[sink];

  if (sink < 0 || sink >= opc_next_sink) {
    fprintf(stderr, "OPC: Sink %d does not exist\n", sink);
    return;
  }
  if (info->sock >= 0) {
    close(info->sock);
    info->sock = -1;
  }
}

static void opc_send(opc_sink sink, const u8* buffer, ssize_t length) {
  ssize_t total_sent = 0;
  ssize_t sent;
  opc_sink_info* info = &opc_sinks[sink];

  if (sink < 0 || sink >= opc_next_sink) {
    fprintf(stderr, "OPC: Sink %d does not exist\n", sink);
    return;
  }
  opc_connect(sink);
  while (total_sent < length) {
    sent = send(info->sock, buffer + total_sent, length - total_sent, 0);
    if (sent < 0) {
      perror("OPC: Error sending data");
      opc_close(sink);
      break;
    }
    total_sent += sent;
  }
}

void opc_put_pixels(opc_sink sink, u8 address, u16 count, pixel* pixels) {
  u8 header[4];
  ssize_t length;

  if (count > 0xffff / 3) {
    fprintf(stderr, "OPC: Maximum pixel count exceeded (%d > %d)\n",
            count, 0xffff / 3);
  }
  length = count * 3;

  header[0] = address;
  header[1] = OPC_SET_PIXELS;
  header[2] = length >> 8;
  header[3] = length & 0xff;
  opc_send(sink, header, 4);
  opc_send(sink, (u8*) pixels, length);
}
