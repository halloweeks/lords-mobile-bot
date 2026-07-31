#ifndef SOCIAL_H
#define SOCIAL_H

#include <stdint.h>

struct Connection;

void RecvSocialData(struct Connection*, const uint8_t*);

#endif