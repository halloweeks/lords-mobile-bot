#ifndef BOTS_H
#define BOTS_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#include "config.h"
#include "connection.h"

#define MAX_BOTS 256

/*
 * All bots. Used by the main loop and by the Web GUI thread, so every access
 * must happen between BotsLock() and BotsUnlock().
 *
 * Each Connection is allocated separately: epoll keeps a pointer to it, so
 * its address must never change while the bot is registered.
 */
typedef struct {
	ProgramConfig *program;        /* global settings (server, version, ...) */
	const char    *program_path;   /* program.cfg, updated on add/remove     */

	Connection *bot[MAX_BOTS];
	char        path[MAX_BOTS][256];   /* config file of each bot */
	size_t      count;

	/* Removed bots wait here until the main loop is done with its current
	 * epoll events; BotsReap() frees them. */
	Connection *dead[MAX_BOTS];
	size_t      dead_count;

	pthread_mutex_t lock;
} BotList;

static inline void BotsLock(BotList *l)   { pthread_mutex_lock(&l->lock); }
static inline void BotsUnlock(BotList *l) { pthread_mutex_unlock(&l->lock); }

void BotsInit(BotList *l, ProgramConfig *program, const char *program_path);
void BotsDestroy(BotList *l);

/* Startup: load an existing bot config file. */
bool BotsLoad(BotList *l, const char *config_path);

/*
 * Web GUI actions. They return NULL on success or a short error message
 * (plain text, no quotes) that is shown to the user. Call with the lock held.
 */
const char *BotsAdd(BotList *l, const BotAccount *account);
const char *BotsRemove(BotList *l, const char *igg_id);
const char *BotsStart(BotList *l, const char *igg_id);   /* NULL = all bots */
const char *BotsStop(BotList *l, const char *igg_id);    /* NULL = all bots */

/* Main loop: call (locked) after processing a batch of epoll events. */
void BotsReap(BotList *l);

#endif
