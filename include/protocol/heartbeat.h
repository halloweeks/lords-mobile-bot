#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <stdint.h>

struct Connection;

void RequestHeartBeat(struct Connection*);


void RecvHeartBeat(struct Connection*, const uint8_t*);

#endif