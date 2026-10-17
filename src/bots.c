#include "bots.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/stat.h>

#include "log.h"

extern int epoll_fd;   /* main.c */

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static Connection *BotCreate(BotList *l, const char *config_path)
{
	const ProgramConfig *p = l->program;

	Connection *c = calloc(1, sizeof(*c));

	if (!c) {
		return NULL;
	}

	if (!LoadConfig(c, config_path)) {
		free(c);
		return NULL;
	}

	c->sock = -1;   /* calloc gives 0, which would be stdin */

	snprintf(c->bot.data_path, sizeof(c->bot.data_path), "%s", p->data_path);

	c->app.version_major = p->version_major;
	c->app.version_minor = p->version_minor;
	c->app.version_patch = p->version_patch;
	c->app.language_code = p->language_code;

	snprintf(c->gateway_server.addr, sizeof(c->gateway_server.addr), "%s", p->server_addr);
	c->gateway_server.port = p->server_port;

	c->reconnect_max_attempts = p->reconnect_max_attempts;
	c->reconnect_attempt      = 0;

	c->state = CONN_DISCONNECTED;

	return c;
}

static void BotAppend(BotList *l, Connection *c, const char *config_path)
{
	l->bot[l->count] = c;
	snprintf(l->path[l->count], sizeof(l->path[0]), "%s", config_path);
	l->count++;
}

static void BotStart(Connection *c)
{
	c->enabled = true;
	c->reconnect_attempt = 0;
	c->reconnect_time = 0;

	/* A bot that failed to log in stays in that state until restarted. */
	if (c->sock < 0 &&
		(c->state == CONN_GATEWAY_LOGIN_FAILED || c->state == CONN_CLOSED)) {
		c->state = CONN_DISCONNECTED;
	}
}

static void BotStop(Connection *c)
{
	c->enabled = false;

	if (c->sock >= 0) {
		if (epoll_fd >= 0) {
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c->sock, NULL);
		}

		close(c->sock);
		c->sock = -1;
	}

	memset(&c->stream, 0, sizeof(c->stream));
	c->state = CONN_DISCONNECTED;
}

static bool IsDigits(const char *s)
{
	if (!*s) {
		return false;
	}

	for (; *s; s++) {
		if (!isdigit((unsigned char)*s)) {
			return false;
		}
	}

	return true;
}

/* Printable ASCII, no spaces or control characters (so it can never break
 * the line-based config format). */
static bool IsToken(const char *s)
{
	for (; *s; s++) {
		if ((unsigned char)*s < 0x21 || (unsigned char)*s > 0x7e) {
			return false;
		}
	}

	return true;
}

/* Index of the bot with this IGG ID, or -1. */
static int FindBot(const BotList *l, const char *igg_id)
{
	if (strlen(igg_id) > 18 || !IsDigits(igg_id)) {
		return -1;
	}

	int64_t id = strtoll(igg_id, NULL, 10);

	for (size_t i = 0; i < l->count; i++) {
		if (l->bot[i]->auth.igg_id == id) {
			return (int)i;
		}
	}

	return -1;
}

static const char *ValidateAccount(const BotAccount *a)
{
	size_t n = strlen(a->igg_id);

	if (n < 1 || n > 18 || !IsDigits(a->igg_id)) {
		return "IGG ID must be 1 to 18 digits";
	}

	n = strlen(a->device_uuid);

	if (n < 1 || n > 49 || !IsToken(a->device_uuid)) {
		return "Device ID is required (up to 49 characters, no spaces)";
	}

	n = strlen(a->access_key);

	if (n < 1 || n > 500 || !IsToken(a->access_key)) {
		return "Access key is required (up to 500 characters, no spaces)";
	}

	if (a->device_type < 1 || a->device_type > 255) {
		return "Device type must be 1 to 255";
	}

	n = strlen(a->admin_name);

	if (n > 12) {
		return "Admin name must be 12 characters or less";
	}

	for (size_t i = 0; i < n; i++) {
		if ((unsigned char)a->admin_name[i] < 0x20 || a->admin_name[i] == 0x7f) {
			return "Admin name has invalid characters";
		}
	}

	if (n > 0 && (a->admin_name[0] == ' ' || a->admin_name[n - 1] == ' ')) {
		return "Admin name cannot start or end with a space";
	}

	return NULL;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void BotsInit(BotList *l, ProgramConfig *program, const char *program_path)
{
	memset(l, 0, sizeof(*l));
	l->program = program;
	l->program_path = program_path;
	pthread_mutex_init(&l->lock, NULL);
}

void BotsDestroy(BotList *l)
{
	for (size_t i = 0; i < l->count; i++) {
		BotStop(l->bot[i]);
		free(l->bot[i]);
	}

	l->count = 0;
	BotsReap(l);
}

bool BotsLoad(BotList *l, const char *config_path)
{
	if (l->count >= MAX_BOTS) {
		return false;
	}

	Connection *c = BotCreate(l, config_path);

	if (!c) {
		return false;
	}

	c->enabled = l->program->autostart;
	BotAppend(l, c, config_path);
	return true;
}

const char *BotsAdd(BotList *l, const BotAccount *a)
{
	const char *err = ValidateAccount(a);

	if (err) {
		return err;
	}

	if (l->count >= MAX_BOTS) {
		return "Too many accounts";
	}

	if (FindBot(l, a->igg_id) >= 0) {
		return "This IGG ID is already added";
	}

	char path[64];
	snprintf(path, sizeof(path), "configs/%s.cfg", a->igg_id);

	mkdir("configs", 0775);

	if (access(path, F_OK) == 0) {
		return "A config file for this IGG ID already exists in configs/";
	}

	if (!CreateBotConfig(path, a)) {
		unlink(path);
		return "Could not write the bot config file";
	}

	/* Load it back: the bot is built from exactly what a restart would read. */
	Connection *c = BotCreate(l, path);

	if (!c) {
		unlink(path);
		return "The generated config file could not be loaded";
	}

	if (!ProgramConfigEditInclude(l->program_path, path, true)) {
		unlink(path);
		free(c);
		return "Could not update program.cfg";
	}

	/* Stays stopped until the user presses Start. */
	BotAppend(l, c, path);
	return NULL;
}

const char *BotsRemove(BotList *l, const char *igg_id)
{
	int i = FindBot(l, igg_id);

	if (i < 0) {
		return "No such account";
	}

	if (l->dead_count >= MAX_BOTS) {
		return "Busy, try again";
	}

	if (!ProgramConfigEditInclude(l->program_path, l->path[i], false)) {
		return "Could not update program.cfg";
	}

	Connection *c = l->bot[i];
	BotStop(c);

	/* Keep the file in case it was removed by mistake. */
	char removed[300];
	snprintf(removed, sizeof(removed), "%s.removed", l->path[i]);
	rename(l->path[i], removed);

	l->dead[l->dead_count++] = c;

	size_t tail = l->count - (size_t)i - 1;
	memmove(&l->bot[i], &l->bot[i + 1], tail * sizeof(l->bot[0]));
	memmove(l->path[i], l->path[i + 1], tail * sizeof(l->path[0]));
	l->count--;

	return NULL;
}

const char *BotsStart(BotList *l, const char *igg_id)
{
	if (!igg_id) {
		for (size_t i = 0; i < l->count; i++) {
			BotStart(l->bot[i]);
		}

		return NULL;
	}

	int i = FindBot(l, igg_id);

	if (i < 0) {
		return "No such account";
	}

	BotStart(l->bot[i]);
	return NULL;
}

const char *BotsStop(BotList *l, const char *igg_id)
{
	if (!igg_id) {
		for (size_t i = 0; i < l->count; i++) {
			BotStop(l->bot[i]);
		}

		return NULL;
	}

	int i = FindBot(l, igg_id);

	if (i < 0) {
		return "No such account";
	}

	BotStop(l->bot[i]);
	return NULL;
}

void BotsReap(BotList *l)
{
	while (l->dead_count > 0) {
		free(l->dead[--l->dead_count]);
	}
}
