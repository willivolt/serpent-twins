// Open Pixel Control, a protocol for controlling arrays of RGB lights.
#ifndef OPC_H
#define OPC_H

#include <stdint.h>

#ifndef TYPEDEF_U8
#define TYPEDEF_U8
typedef uint8_t u8;
#endif

#ifndef TYPEDEF_U16
#define TYPEDEF_U16
typedef uint16_t u16;
#endif

#ifndef TYPEDEF_U32
#define TYPEDEF_U32
typedef uint32_t u32;
#endif

#ifndef TYPEDEF_PIXEL
#define TYPEDEF_PIXEL
typedef struct { u8 r, g, b; } pixel;
#endif

/* OPC addresses */
#define OPC_BROADCAST 0

/* OPC command codes */
#define OPC_SET_PIXELS 0

#define OPC_MAX_SINKS 64
#define OPC_MAX_SOURCES 64

// OPC client functions ----------------------------------------------------

/* Handle for an OPC sink created by opc_new_sink. */
typedef int opc_sink;

/* Creates a new OPC sink.  'hostname' must be a valid numeric IPv4 address. */
/* No TCP connection is attempted; the connection will be automatically */
/* opened as necessary (and reopened if it closes). */
opc_sink opc_new_sink(char* hostname, u16 port);

/* Ensures that the connection for a sink is open, retrying until success. */
void opc_connect(opc_sink sink);

/* Sends RGB data for 'count' pixels to address 'address'. */
void opc_put_pixels(opc_sink sink, u8 address, u16 count, pixel* pixels);

// OPC server functions ----------------------------------------------------

/* Handle for an OPC source created by opc_new_source. */
typedef int opc_source;

/* Handler called by opc_receive when pixel data is received. */
typedef void opc_handler(u8 address, u16 count, pixel* pixels);

/* Creates a new OPC source by listening on the specified TCP port.  At most */
/* one incoming connection is accepted at a time; if the connection closes, */
/* we will begin listening for another connection. */
opc_source opc_new_source(u16 port);

/* Handles incoming connections and pixel data on a given OPC source. */
void opc_receive(opc_source source, opc_handler* handler, u32 timeout_ms);

#endif  /* OPC_H */
