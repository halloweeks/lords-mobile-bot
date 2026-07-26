#ifndef CONFIG_H
#define CONFIG_H

#include "connection.h"
#include "items.h"

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
} ProgramConfig;

bool CreateDefaultConfig(const char *filename);
bool CreateDefaultProgramConfig(const char *filename);

bool LoadConfig(Connection *c, const char *filename);
ProgramConfig LoadProgramConfig(const char *filepath);

#endif