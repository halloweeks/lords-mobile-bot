#include "protocol/login.h"
#include "net_rw.h"
#include "packet_enum.h"
#include "connection.h"
#include "log.h"

/* Gateway login */
void RequestGuestLogIn(Connection *c)
{
	// reserve space for packet length
    c->size = 2;
    
    // write packet type 
    write_u16(c->data + c->size, _MSG_NEWLOGIN_LOGINTOL);
    c->size += 2;
    
    // write igg id
    write_u64(c->data + c->size, c->auth.igg_id);
    c->size += 8;
    
    // write game minor version 
    write_u8(c->data + c->size, c->app.version_minor);
    c->size += 1;
    
    // write game major version
    write_u8(c->data + c->size, c->app.version_major);
    c->size += 1;
    
    // write game patch version
    write_u16(c->data + c->size, c->app.version_patch);
    c->size += 2;
    
    // Device type
    // Android    = 1 
    // Desktop    = 9 
    write_u8(c->data + c->size, c->auth.device_type);
    c->size += 1;
    
    // write language code
    write_u8(c->data + c->size, c->app.language_code);
    c->size += 1;
    
    // write Device Universally Unique Identifier.
    write_raw(c->data + c->size, c->auth.device_uuid, 50);
    c->size += 50;
    
    // write session length
    write_u16(c->data + c->size, c->auth.session_len);
    c->size += 2;
    
    // write session token
    write_raw(c->data + c->size, c->auth.session, 512);
    c->size += 512;
    
    // rewrite total packet length 
    write_u16(c->data, c->size);
    
    // call send packet
    send_packet(c, false);
}


/* Game login */
void RequestLogIn(Connection *c) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_NEWLOGIN_LOGINTOP);
	c->size += 2;
	
	// write igg id
	write_u64(c->data + c->size, c->auth.igg_id);
	c->size += 8;
	
	// write 50 byte zero 
	for (int i = 0; i < 25; i++) {
		write_u16(c->data + c->size, 0);
		c->size += 2;
	}
	
	// write version 
	write_u32(c->data + c->size, 0);
	c->size += 4;
	
	// battle_is_oul
	write_u8(c->data + c->size, 0);
	c->size += 1;
	
	// b_recv_kingdom
	write_u8(c->data + c->size, 0);
	c->size += 1;
	
	// session len
	write_u16(c->data + c->size, c->auth.session_len);
	c->size += 2;
	
	// session 
	write_raw(c->data + c->size, c->auth.session, 512);
    c->size += 512;

    write_u16(c->data, c->size);
    
    send_packet(c, false);
}

/* initialization over after game login */
void RequestClientInitOver(Connection *c) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_CLIENTINITOVER);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write igg id
	write_u64(c->data + c->size, c->auth.igg_id);
	c->size += 8;
	
	write_u16(c->data, c->size);
    
    send_packet(c, true);
}


void RecvLoginValidate(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	
	c->game_server.port = read_i32(data + offset);      offset += 4;
	c->auth.igg_id      = read_i64(data + offset);      offset += 8;
	read_bytes(c->game_server.addr, data + offset, 16); offset += 16;
	
	// c->state = CONN_CONNECTING_GAME;
	c->state = CONN_GATEWAY_LOGIN_SUCCESS;
	LOGI("Gateway Login success! Switching to game server\n");
}


void RecvLoginError(Connection *c, const uint8_t *data) {
	uint8_t kind = read_u8(data);
	
	if (kind == 8) {
		c->state = CONN_DISCONNECTED;
		LOGE("HUHHHHHHHHHHHHH\n");
		return;
	}
	
	if (kind == 110) {
		// [ERROR] Client version outdated. Required: 2.197.309
		LOGE("UPDATE CLIENT VERSION\n");
	} else if (kind == 9) {
		LOGE("LOGGING FROM ANOTHER DEVICE errorCode: %u\n", kind);
	} else {
		LOGE("Bootstrap Login failed: %u\n", kind);
	}
	
	c->state = CONN_GATEWAY_LOGIN_FAILED;
}

void RecvLoginError2(Connection *c, const uint8_t *data) {
	int32_t kind = read_i32(data);
	
	LOGE("Bootstrap Login failed session expired: %d\n", kind);
	
	c->state = CONN_GATEWAY_LOGIN_FAILED;
} 

void RecvGameLogin(Connection *c, const uint8_t *data) {
	printf("[GAME] LOGIN OK\n");
	
	return;
}

uint16_t RoleAttrLevelUp(const uint8_t *data, int UpdateFlag) {
	uint16_t offset = 0;
	
	if ((UpdateFlag & 1) != 0) {
		read_u8(data + offset); offset += 1;
		
		// this.UpdateRoleAttrLevel(MP.ReadByte(-1));
	}
	
	if ((UpdateFlag & 2) != 0) {
		read_u32(data + offset); offset += 4;
		// this.UpdateRoleAttrExp(MP.ReadUInt(-1));
	}
	
	if ((UpdateFlag & 4) != 0) {
		read_u32(data + offset); offset += 4;
		// DataManager.Instance.Resource[4].Stock = MP.ReadUInt(-1);
	}
	
	if ((UpdateFlag & 8) != 0) {
		read_u16(data + offset); offset += 2;
		// this.UpdateRoleAttrMorale(MP.ReadUShort(-1));
	}
	
	if ((UpdateFlag & 16) != 0) {
		read_u64(data + offset); offset += 8;
		// DataManager.Instance.RoleAttr.LastMoraleRecoverTime = MP.ReadLong(-1);
	}
	
	if ((UpdateFlag & 32) != 0) {
		read_u16(data + offset); offset += 2;
		// this.UpdateRoleTalentPoint(MP.ReadUShort(-1));
	}
	
	return offset;
}

void RecvLoginRoleInfo(Connection *c, const uint8_t *data) 
{
	uint16_t offset = 0;
	
	// printf("\n\n\n");
	
	uint32_t ReadPackNum = read_u32(data + offset); offset += 4;
	uint64_t UserId      = read_u64(data + offset); offset += 8;
	read_raw(c->player.name, data + offset, 13); offset += 13;
	uint16_t Head = read_u16(data + offset); offset += 2;
	
	offset += RoleAttrLevelUp(data + offset, 27);
	// p.read_bytes(d, 27);
		
	uint64_t ServerTime = read_u64(data + offset); offset += 8;
	uint64_t LogoutTime = read_u64(data + offset); offset += 8;
	uint64_t Guide = (unsigned long)read_u32(data + offset); offset += 4;
	uint32_t Diamond = read_u32(data + offset); offset += 4;
	
	c->player.gems = Diamond;
	
	uint8_t HeroSkillPoint = read_u8(data + offset); offset += 1;
	uint64_t LastHeroSPRecoverTime = read_u64(data + offset); offset += 8;
	uint16_t EnhanceEventHeroID = read_u16(data + offset); offset += 2;
	uint64_t HeroEnhanceEventTime_BeginTime = read_u64(data + offset); offset += 8;
	uint32_t HeroEnhanceEventTime_RequireTime = read_u32(data + offset); offset += 4;
	
	uint16_t StarUpEventHeroID = read_u16(data + offset); offset += 2;
	uint64_t HeroStarUpEventTime_BeginTime = read_u64(data + offset); offset += 8;
	uint32_t HeroStarUpEventTime_RequireTime = read_u32(data + offset); offset += 4;
	
	uint8_t temp[100];
	read_raw(temp, data + offset, 12); offset += 12;
	read_raw(temp, data + offset, 48); offset += 48;
	
	uint16_t abb = read_u16(data + offset); offset += 2;
	read_u16(data + offset); offset += 2;
	// printf("abb: %u\n", abb);
	
	// printf("Head: %u\n", Head);
	uint64_t BattleID = read_u64(data + offset); offset += 8;
	
	c->player.zone_id  = read_u16(data + offset); offset += 2;
	c->player.point_id = read_u8 (data + offset); offset += 1;
	/*
	uint16_t newZoneID = read_u16(data + offset); offset += 2;
	uint8_t newPointID = read_u8(data + offset); offset += 1;
	*/
	// bank_zoneId = newZoneID;
	// bank_pointId = newPointID;
	
	// map_pos_t pos = getTileMapPosbyPointCode(newZoneID, newPointID);
	// printf("Here: X:%u Y:%u\n", pos.x, pos.y);
	
	
	printf("name: %s\n", c->player.name);
	printf("userid: %lu\n", UserId);
	printf("ServerTime: %lu\n", ServerTime);
	
	printf("Diamond: %u\n", c->player.gems);
	
	printf("\n");
	
	// strcpy(login_name, name);
	// gems = Diamond;
	
	uint64_t LastChatterTime = read_u64(data + offset); offset += 8;
	uint32_t AllianceChatID  = read_u32(data + offset); offset += 4;
	c->player.power = read_u64(data + offset); offset += 8;
	c->player.kills = read_u64(data + offset); offset += 8;
	c->player.vip_point = read_u32(data + offset); offset += 4;
	uint64_t FirstTimer = read_u64(data + offset); offset += 8;
	uint32_t PrizeFlag = read_u32(data + offset); offset += 4;
	
	char PowerStr[30];
	char KillsStr[30];
				
	// format_number2(Power, PowerStr, 30);
	// format_number2(Kills, KillsStr, 30);
	
	/*
	printf("LastChatterTime: %lu\n", LastChatterTime);
	printf("AllianceChatID: %u\n", AllianceChatID);
	
	printf("Power: %s (%lu)\n", PowerStr, Power);
	printf("Kills: %s (%lu)\n", KillsStr, Kills);
	
	printf("VipPoint: %u\n", VipPoint);
	printf("FirstTimer: %lu\n", FirstTimer);
	printf("PrizeFlag: %u\n", PrizeFlag);
	*/
	
	uint64_t BookmarkTime = read_u64(data + offset); offset += 8;
	uint16_t BookmarkLimit = read_u16(data + offset); offset += 2;
	uint16_t BookmarkNum = read_u16(data + offset); offset += 2;
	
	/*
	printf("BookmarkTime: %ld\n", (int64_t)BookmarkTime);
	printf("BookmarkLimit: %u\n", BookmarkLimit);
	printf("BookmarkNum: %u\n", BookmarkNum);
	*/
	
	// skirmishes
	// read UpdateCorpsStageInfo data
	uint8_t skirmishes = read_u8(data + offset); offset += 1;
	
	printf("skirmishes: %u\n", skirmishes);
	
	for (int i = 0; i < 10; i++) {
		/*
		NowCombatStageInfo[i].SoldierTableID = read_u8(data + offset); offset += 1;
		NowCombatStageInfo[i].Amount = read_u32(data + offset); offset += 4;
		
		printf("NowCombatStageInfo[%d].SoldierTableID: %u\n", i, NowCombatStageInfo[i].SoldierTableID);
		printf("NowCombatStageInfo[%d].Amount: %u\n", i, NowCombatStageInfo[i].Amount);
		*/
		
		uint8_t SoldierTableID = read_u8(data + offset); offset += 1;
		uint32_t Amount = read_u32(data + offset); offset += 4;
		
		// printf("NowCombatStageInfo[%d].SoldierTableID: %u\n", i, SoldierTableID);
		// printf("NowCombatStageInfo[%d].Amount: %u\n", i, Amount);
		
	}
	
	uint32_t CorpsStageWallDefence = read_u32(data + offset); offset += 4;
	// printf("CorpsStageWallDefence: %u\n", CorpsStageWallDefence);
	
	uint16_t SuccessiveLoginDays = read_u16(data + offset); offset += 2;
	// printf("SuccessiveLoginDays: %u\n", SuccessiveLoginDays);
	
	uint8_t TodayUseMoraleItemTimes = read_u8(data + offset); offset += 1;
	// printf("TodayUseMoraleItemTimes: %u\n", TodayUseMoraleItemTimes);
	
	uint8_t LordEquipBagSize = read_u8(data + offset); offset += 1;
	// printf("LordEquipBagSize: %u\n", LordEquipBagSize);
	
	uint64_t NextOnlineGiftOpenTime = read_u64(data + offset); offset += 8;
	// printf("NextOnlineGiftOpenTime: %ld\n", (int64_t)NextOnlineGiftOpenTime);
	
	uint8_t OnlineGiftOpenTimes = read_u8(data + offset); offset += 1;
	// printf("OnlineGiftOpenTimes: %u\n", OnlineGiftOpenTimes);
	
	uint16_t OnlineGiftItemID_ItemID = read_u16(data + offset); offset += 2;
	// printf("OnlineGiftItemID_ItemID: %u\n", OnlineGiftItemID_ItemID);
	
	uint16_t OnlineGiftItemID_Quantity = read_u16(data + offset); offset += 2;
	// printf("OnlineGiftItemID_Quantity: %u\n", OnlineGiftItemID_Quantity);
	
	int64_t LastLordEquipUpdateTime = (int64_t)read_u64(data + offset); offset += 8;
	int64_t LastItemMatUpdateTime = (int64_t)read_u64(data + offset); offset += 8;
	int64_t LastItemGemUpdateTime = (int64_t)read_u64(data + offset); offset += 8;
	uint16_t LordEquipEventData_ItemID = read_u16(data + offset); offset += 2;
	int8_t LordEquipEventData_Color = (int8_t)read_u8(data + offset); offset += 1;
	
	/*
	printf("LastLordEquipUpdateTime: %ld\n", LastLordEquipUpdateTime);
	printf("LastItemMatUpdateTime: %ld\n", LastItemMatUpdateTime);
	printf("LastItemGemUpdateTime: %ld\n", LastItemGemUpdateTime);
	printf("LordEquipEventData_ItemID: %u\n", LordEquipEventData_ItemID);
	printf("LordEquipEventData_Color: %d\n", LordEquipEventData_Color);
	*/
	
	int8_t LordEquipEventData_GemColor;
	
	for (int i = 0; i < 4; i++)
	{
		LordEquipEventData_GemColor = (int8_t)read_u8(data + offset); offset += 1;
		// printf("LordEquipEventData.GemColor[%d]: %d\n", i, LordEquipEventData_GemColor);
	}
	
	uint16_t LordEquipEventData_Gem;
	
	for (int j = 0; j < 4; j++) {
		LordEquipEventData_Gem = read_u16(data + offset); offset += 2;
		// printf("LordEquipEventData.Gem[%d]: %u\n", j, LordEquipEventData_Gem);
	}
	
	uint32_t LordEquipEventData_SerialNO = read_u32(data + offset); offset += 4;
	int64_t LordEquipEventTime_BeginTime = (int64_t)read_u64(data + offset); offset += 8;
	uint32_t LordEquipEventTime_RequireTime = read_u32(data + offset); offset += 4;
	int8_t VipLevelUp = (int8_t)read_u8(data + offset); offset += 1;
	
	/*
	printf("LordEquipEventData_SerialNO: %u\n", LordEquipEventData_SerialNO);
	printf("LordEquipEventTime_BeginTime: %ld\n", LordEquipEventTime_BeginTime);
	printf("LordEquipEventTime_RequireTime: %u\n", LordEquipEventTime_RequireTime);
	printf("VipLevelUp: %u\n", VipLevelUp);
	*/
	
	uint16_t nowKingdomID  = read_u16(data + offset); offset += 2;
	uint16_t homeKingdomID = read_u16(data + offset); offset += 2;
	
	c->player.current_kingdom_id = nowKingdomID;
	c->player.home_kingdom_id = homeKingdomID;
	// current_kingdom = nowKingdomID;
	
	printf("nowKingdomID: %u\n", nowKingdomID);
	printf("homeKingdomID: %u\n", homeKingdomID);
	
	/*
	DataManager.MapDataController.updateMyKingdom(MP.ReadUShort(-1), MP.ReadUShort(-1));
	DataManager.MapDataController.updateCapitalPoint(newZoneID, newPointID, DataManager.MapDataController.OtherKingdomData.kingdomID, false);
	*/
	
	// exit(1);
}

void RecvMagicGateDoEvent(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t b = read_u8(data + offset); offset += 1;
	
	printf("b: %u\n", b);
	
	
}


void RecvTDInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
}

void RecvTDTriggerInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
}
