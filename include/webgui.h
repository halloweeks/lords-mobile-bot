#ifndef WEBGUI_H
#define WEBGUI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bots.h"

typedef struct {
	BotList *bots;

	char     host[64];
	uint16_t port;

	bool auth_enabled;
	char username[64];
	char password[128];
} WebGuiContext;

/* Returns 1 on success, 0 on failure. */
int  WebGuiStart(WebGuiContext *ctx);
void WebGuiStop(void);

#endif
