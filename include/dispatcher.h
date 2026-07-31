#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <stdint.h>
#include "connection.h"

typedef void (*PacketHandler)(
    Connection *c,
    const uint8_t *data
);

/* Initialize packet table */
void DispatcherInit(void);

/* Dispatch packet to handler */
void DispatchPacket(
    Connection *c,
    uint16_t packet_type,
    const uint8_t *data
);

#endif