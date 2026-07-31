#ifndef CARGO_SHIP_H
#define CARGO_SHIP_H

#include <stdint.h>
#include <stdbool.h>

struct Connection;
// struct SmartUseList;

typedef struct {
    uint16_t item_id;
    uint16_t item_count;
    uint8_t resource_kind;
    uint32_t resource_count;
    uint8_t rare;
} CargoShipItem;


typedef struct {
	bool blazing_ember;
	bool archaic_tome;
	bool bright_talent_orb;
	bool exp_elixir;
	bool speed_up;
	bool speed_up_research;
	bool speed_up_merging;
	bool speed_up_training;
	
	bool star_scroll;
	bool wolfpack_sigil;
	
	bool anima;
	bool food;
	bool rock;
	bool wood;
	bool ore;
	bool gold;
} CargoShipBuyOptions;

typedef struct {
	bool auto_trade;
	bool use_bag_rss;
	
	bool spend[5]; 
	
	bool spend_food;
	bool spend_rock;
	bool spend_wood;
	bool spend_ore;
	bool spend_gold;
} CargoShipSettings;

typedef struct {
	bool loaded;
	bool buy_pending;
	uint8_t trade_locks;
	uint8_t trade_status;
	uint64_t refresh_time;
	CargoShipItem items[4];
	uint32_t reserve[5];
	CargoShipSettings settings;
	
	CargoShipBuyOptions trade_for;
} CargoShip;

// Request 
void RequestCargoShipData(struct Connection*);
void RequestBuyCargoShipItem(struct Connection*, uint8_t mIdx);
// void RequestBuyCargoShipItemSmartUse(struct Connection*, struct SmartUseList *smart_use, uint8_t mIdx);

// Response 
void RecvCargoShipData(struct Connection*, const uint8_t*);
void RecvBuyCargoShipItem(struct Connection*, const uint8_t*);


void EvaluateCargoShipTrade(struct Connection*);

void CargoShipTick(struct Connection*);

#endif