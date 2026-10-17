#ifndef CONFIG_H
#define CONFIG_H

#include "items.h"
#include <stdbool.h>
#include <stdint.h>

struct Connection;

typedef struct {
	char server_addr[16];
	uint16_t server_port;
	
	uint8_t version_major;
	uint8_t version_minor;
	uint16_t version_patch;
	
	uint8_t language_code;
	char data_path[256];
	
	uint32_t bot_count;
	
	char config_path[256][256];
	
	bool autostart;       // connect bots when the program starts
	
	bool auto_reconnect;
	uint16_t reconnect_delay;
	uint32_t reconnect_max_attempts;
	uint32_t reconnect_attempt;
	
	// Holy shit added
	bool webgui_enabled;
	char webgui_host[16];
	int  webgui_port;
	bool webgui_auth;
	char webgui_user[32];
	char webgui_pass[32];
} ProgramConfig;

// Account fields collected by the Web GUI (all plain text).
typedef struct {
	char igg_id[24];
	char device_uuid[50];
	char access_key[512];
	int  device_type;      // 1 = Android, 9 = Desktop
	char admin_name[32];   // optional
} BotAccount;

bool CreateDefaultConfig(const char *filename);
bool CreateBotConfig(const char *filename, const BotAccount *account);

// include_example: also list configs/bank123.cfg (used by --create-config)
bool CreateDefaultProgramConfig(const char *filename, bool include_example);

// Add or remove one "include = <bot_path>" line in program.cfg.
bool ProgramConfigEditInclude(const char *filename, const char *bot_path, bool add);

bool LoadConfig(struct Connection *c, const char *filename);
ProgramConfig LoadProgramConfig(const char *filepath);

#endif