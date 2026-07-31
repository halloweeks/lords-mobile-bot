#include "protocol/resource.h"
#include "net_rw.h"
#include "packet_enum.h"
#include "connection.h"
#include "log.h"

// Cargo ship
#include "protocol/cargo_ship.h"

#include "utility.h"

void ResourcesLog(Connection *c) {
	char food_str[20];
	char rock_str[20];
	char wood_str[20];
	char  ore_str[20];
	char gold_str[20];
	
	FormatNumber(c->resource.stock[0], food_str, 20);
	FormatNumber(c->resource.stock[1], rock_str, 20);
	FormatNumber(c->resource.stock[2], wood_str, 20);
	FormatNumber(c->resource.stock[3], ore_str, 20);
	FormatNumber(c->resource.stock[4], gold_str, 20);
	
	LOGD("[RESOURCE][%lu] FOOD = %s\n", c->auth.igg_id, food_str);
	LOGD("[RESOURCE][%lu] ROCK = %s\n", c->auth.igg_id, rock_str);
	LOGD("[RESOURCE][%lu] WOOD = %s\n", c->auth.igg_id, wood_str);
	LOGD("[RESOURCE][%lu] ORE  = %s\n", c->auth.igg_id, ore_str);
	LOGD("[RESOURCE][%lu] GOLD = %s\n", c->auth.igg_id, gold_str);
	return;
}

void RecvResources(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	// Food
	c->resource.stock[0]      = read_u32(data + offset); offset += 4;
	c->resource.production[0] = read_i64(data + offset); offset += 8;
	
	/*
	int64_t prod = c->resource.production[0];
	
	if (prod >= 0) {
		printf("Food production: +%ld/hour\n", prod);
	} else {
		printf("Food consumption: %ld/hour\n", prod);
	}*/
	
	// Rock
	c->resource.stock[1]      = read_u32(data + offset); offset += 4;
	c->resource.production[1] = read_i64(data + offset); offset += 8;
	
	// Wood
	c->resource.stock[2]      = read_u32(data + offset); offset += 4;
	c->resource.production[2] = read_i64(data + offset); offset += 8;
	
	// Ore
	c->resource.stock[3]      = read_u32(data + offset); offset += 4;
	c->resource.production[3] = read_i64(data + offset); offset += 8;
	
	// Gold
	c->resource.stock[4]      = read_u32(data + offset); offset += 4;
	c->resource.production[4] = read_i64(data + offset); offset += 8;
	
	c->resource.loaded = true;
	
	// this will must inside tick instead of this module 
	if (c->cargo_ship.loaded) {
		EvaluateCargoShipTrade(c);
	}
	
	#ifdef _DEBUG_
		ResourcesLog(c);
	#endif
	return;
}

void RecvRefreshResources(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->resource.stock[0]  = read_u32(data + offset); offset += 4;
	c->resource.stock[1]  = read_u32(data + offset); offset += 4;
	c->resource.stock[2]  = read_u32(data + offset); offset += 4;
	c->resource.stock[3]  = read_u32(data + offset); offset += 4;
	c->resource.stock[4]  = read_u32(data + offset); offset += 4;
	
	c->resource.loaded = true;
	
	#ifdef _DEBUG_
		ResourcesLog(c);
	#endif
	return;
}


void RecvResourceHelpReportInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint32_t aa = read_u32(data + offset); offset += 4;
	uint8_t bb  = read_u8(data + offset); offset += 1;
	// received date
	uint64_t cc = read_u64(data + offset); offset += 8;
	uint8_t result = read_u8(data + offset); offset += 1;// it's results 1 means received from player! and 0 means bank sending the resource
	
	printf("aa: %u\n", aa);
	printf("bb: %u\n", bb);
	printf("cc: %lu\n", cc);
	
	char player_name[13] = {0};
	read_raw(player_name, data + offset, 13); offset += 13;
	
	uint32_t stock[5];
	
	for (int i = 0; i < 5; i++) {
		stock[i] = read_u32(data + offset); offset += 4;
	}
	
	/*if (dd == 1) {
		update_balance(player_name, res, dd);
	}*/
	
	printf("Result: %u\n", result);
	printf("player: %s\n", player_name);
				
	printf("food: %u\n",   stock[0]);
	printf("rock: %u\n",   stock[1]);
	printf("wood: %u\n",   stock[2]);
	printf("ore:  %u\n",   stock[3]);
	printf("gold: %u\n\n", stock[4]);
}
