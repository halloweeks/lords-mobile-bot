#include "protocol/cargo_ship.h"
#include "net_rw.h"
#include "packet_enum.h"
#include "connection.h"
#include "log.h"
#include "items.h"
#include <stdbool.h>

void RequestCargoShipData(Connection *c) {
	c->size = 2;
	write_u16(c->data + c->size, _MSG_REQUSET_BLACKMARKET_DATA); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
	write_u8 (c->data + c->size, 1); c->size += 1;
	write_u16(c->data,  c->size);
	send_packet(c, true);
	
	LOGD("Called RequestCargoShipData()\n");
}

void RequestBuyCargoShipItem(Connection *c, uint8_t mIdx) {
	c->size = 2;
	write_u16(c->data + c->size, _MSG_REQUEST_BLACKMARKET_BUY); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
	write_u8(c->data + c->size, mIdx);    c->size += 1;
	write_u16(c->data, c->size);
	send_packet(c, true);
	
	LOGD("Called RequestBuyCargoShipItem()\n");
}

void RequestBuyCargoShipItemSmartUse(Connection *c, SmartUseList smart_use, uint8_t mIdx) {
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_SMARTUSE_FOR_BLACKMARKET); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
	write_u8(c->data + c->size, mIdx); c->size += 1;
	write_u16(c->data + c->size, c->smart_use.count); c->size += 2;
	for (int i = 0; i < c->smart_use.count; i++) {
		write_u16(c->data + c->size, c->smart_use.items[i].id); c->size += 2;
		write_u16(c->data + c->size, c->smart_use.items[i].qty); c->size += 2;
	}
	write_u16(c->data, c->size);
	send_packet(c, true);
	
	LOGD("Called RequestBuyCargoShipItemSmartUse()\n");
}

bool ShouldBuyItem(const Connection *c, const CargoShipItem *item)
{
	switch (item->item_id) {
		case BLAZING_EMBER: 
			return c->cargo_ship.trade_for.blazing_ember;
		case ARCHAIC_TOME:
			return c->cargo_ship.trade_for.archaic_tome;
		case BRIGHT_TALENT_ORB: 
			return c->cargo_ship.trade_for.bright_talent_orb;
		case SPEED_UP_30_MINUTE:
		case SPEED_UP_60_MINUTE:
		case SPEED_UP_3_HOUR:
			return c->cargo_ship.trade_for.speed_up;
		case SPEED_UP_MERGING_60_MINUTE:
		case SPEED_UP_MERGING_3_HOUR: 
			return c->cargo_ship.trade_for.speed_up_merging;
		case SPEED_UP_RESEARCH_3_HOUR:
		case SPEED_UP_RESEARCH_8H:
			return c->cargo_ship.trade_for.speed_up_research;
		case ANIMA:
			return c->cargo_ship.trade_for.anima;
		case FOOD_5K:
		case FOOD_30K:
		case FOOD_150K:
		case FOOD_500K:
		case FOOD_2M:
		case FOOD_6M:
		case FOOD_20M:
		case FOOD_60M:
			return c->cargo_ship.trade_for.food;
		case STONE_3K:
		case STONE_10K:
		case STONE_50K:
		case STONE_150K:
		case STONE_500K:
		case STONE_1_5M:
		case STONE_5M:
		case STONE_15M:
			return c->cargo_ship.trade_for.rock;
		case TIMBER_3K:
		case TIMBER_10K:
		case TIMBER_50K:
		case TIMBER_150K:
		case TIMBER_500K:
		case TIMBER_1_5M:
		case TIMBER_5M:
		case TIMBER_15M:
			return c->cargo_ship.trade_for.wood;
		case ORE_3K:
		case ORE_10K:
		case ORE_50K:
		case ORE_150K:
		case ORE_500K:
		case ORE_1_5M:
		case ORE_5M:
		case ORE_15M:
			return c->cargo_ship.trade_for.ore;
		case GOLD_3K:
		case GOLD_15K:
		case GOLD_50K:
		case GOLD_200K:
		case GOLD_600K:
		case GOLD_2M:
		case GOLD_6M:
			return c->cargo_ship.trade_for.gold;
		default: 
			return false;
	}
	
	return false;
}

bool CanSpendResource(Connection *c, uint8_t type)
{
	return c->cargo_ship.settings.spend[type];
}

bool CanAffordCargoShipItem(Connection *c, const CargoShipItem *item)
{
	return c->resource.stock[item->resource_kind] >= 
		c->cargo_ship.reserve[item->resource_kind] + 
		item->resource_count;
}

void EvaluateCargoShipTrade(Connection *c)
{
	if (!c->cargo_ship.settings.auto_trade)
		return;
	
	if (!c->cargo_ship.loaded)
		return;
	
	if (c->cargo_ship.buy_pending)
		return;
		
	for (int i = 0; i < 4; i++) 
	{
		// already purchased
		if (c->cargo_ship.trade_status & (1 << i))
			continue;
		
		CargoShipItem *item = &c->cargo_ship.items[i];
		
		if (!CanSpendResource(c, item->resource_kind)) {
			LOGI("Cannot spend resource kind: %u, skipping item %u\n", item->resource_kind, item->item_id);
			continue;
		}
		
		if (!ShouldBuyItem(c, item)) {
			printf("[MARKET] Slot %d -> SKIP (unwanted item %u)\n", i, item->item_id);
			continue;
		}
		
		if (!CanAffordCargoShipItem(c, item)) {
			LOGI("Cannot afford cargo item: id=%u count=%u resource=%u amount=%u\n",
				item->item_id,
				item->item_count,
				item->resource_kind,
				item->resource_count
			);
			continue;
		}
		
		RequestBuyCargoShipItem(c, i);
		
		c->cargo_ship.buy_pending = true;
		break;
    }
}



void TryEvaluateCargoShipTrade(Connection *c)
{
	if (!c->cargo_ship.settings.auto_trade)
		return;
	
	if (!c->resource.loaded) 
		return;
		
	if (!c->cargo_ship.loaded)
		return;
	
	EvaluateCargoShipTrade(c);
}

void RecvBuyCargoShipItem(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t result = read_u8(data + offset); offset++;
	
	if (result != 0) {
		c->cargo_ship.buy_pending = false;
		printf("BlackMarket buy failed: %u\n", result);
		return;
	}
	
	uint8_t new_trade_status = read_u8(data + offset); offset++;
	uint8_t changed = new_trade_status ^ c->cargo_ship.trade_status;
	
	for (int i = 0; i < 4; i++) 
	{
		if (((changed >> i) & 1) == 1) 
		{
			printf("Purchased slot %d\n", i);
			
			read_u16(data + offset); offset += 2;
			read_u16(data + offset); offset += 2;
			
			uint8_t resource_kind = read_u8(data + offset);
			offset += 1;
			
			uint32_t stock = read_u32(data + offset);
			offset += 4;
			
			printf("resource=%u stock=%u\n", resource_kind, stock);
		}
	}
	
	c->cargo_ship.trade_status = new_trade_status;
	c->cargo_ship.buy_pending = false;
	
	EvaluateCargoShipTrade(c);
}

void CargoShipDataLog(Connection *c) {
	for (int i = 0; i < 4; i++) {
		LOGD("item_id:        %u\n", c->cargo_ship.items[i].item_id);
		LOGD("item_count:     %u\n", c->cargo_ship.items[i].item_count);
		LOGD("resource_kind:  %u\n", c->cargo_ship.items[i].resource_kind);
		LOGD("resource_count: %u\n", c->cargo_ship.items[i].resource_count);
		LOGD("rare:           %u\n", c->cargo_ship.items[i].rare);
		LOGD("\n\n");
	}
}

#include "log.h"
#include "utility.h"

void RecvCargoShipData(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->cargo_ship.refresh_time = read_u64(data + offset); offset += 8;
	c->cargo_ship.trade_locks  = read_i8( data + offset); offset += 1;
	c->cargo_ship.trade_status = read_u8( data + offset); offset += 1;
	
	for (int i = 0; i < 4; i++) {
		c->cargo_ship.items[i].item_id        = read_u16(data + offset);  offset += 2;
		c->cargo_ship.items[i].item_count     = read_u16(data + offset);  offset += 2;
		c->cargo_ship.items[i].resource_kind  = read_u8( data + offset);  offset += 1;
		c->cargo_ship.items[i].resource_count = read_u32(data + offset);  offset += 4;
		c->cargo_ship.items[i].rare           = read_u8( data + offset);  offset += 1;
	}
	
	c->cargo_ship.loaded = true;
	
	#ifdef _DEBUG_
		CargoShipDataLog(c);
	#endif
	
	// printf("[MARKET RESET] ");
	
	LOGI("[%lu] Cargo Ship resets in %s\n", c->auth.igg_id, FormatTime(c->cargo_ship.refresh_time - c->server_time));
	
	TryEvaluateCargoShipTrade(c);
}


// void RequestMissionInfo(Connection *c, uint8_t missionType) {
static void RequestCargoShipInfo(Connection *c,  uint8_t missionType) {
	if (missionType != 0 && missionType != 1) return;
	
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_MISSIONINFO);   c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);       c->size += 4;
	write_u8 (c->data + c->size, (missionType + 1)); c->size += 1;
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
	return;
}

void CargoShipTick(Connection *c) 
{
	if (!c->cargo_ship.loaded)
		return;
	
	if (c->server_time > c->cargo_ship.refresh_time) {
		printf("[MARKET] Refresh expired, requesting new data\n");
		RequestCargoShipInfo(c, 0);
		c->cargo_ship.loaded = false;
	}
}