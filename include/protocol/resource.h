#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdint.h>
#include <stdbool.h>

struct Connection;

typedef struct Resources {
	bool loaded;
	uint32_t stock[5];
	uint64_t bag[5];
	uint64_t production[5];
} Resources;

void RecvResources(struct Connection*, const uint8_t*);
void RecvRefreshResources(struct Connection*, const uint8_t*);

void RecvResourceHelpReportInfo(struct Connection*, const uint8_t*);


#endif