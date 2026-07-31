#include "dispatcher.h"

// Network protocol packet type definitions.
#include "packet_enum.h"

/* Protocol modules */
#include "protocol/login.h"
#include "protocol/chat.h"
#include "protocol/cargo_ship.h"
#include "protocol/resource.h"
#include "protocol/heartbeat.h"
#include "protocol/alliance.h"
#include "protocol/hospital.h"
#include "protocol/social.h"

/*
 * Global packet dispatcher.
 *
 * Uses a direct lookup table (O(1)) instead of a large switch statement.
 * The packet ID is used as the array index, allowing the handler to be
 * found in constant time with a single memory lookup.
 *
 * Benefits:
 * - Constant-time dispatch regardless of the number of packet types.
 * - No large switch-case to maintain.
 * - Easy to register, remove, or replace handlers.
 * - Cleaner modular design (each protocol registers its own handlers).
 *
 * Memory cost is small (~512 KiB on 64-bit systems, ~256 KiB on 32-bit
 * systems) and is acceptable for the performance gained.
 */
static PacketHandler dispatcher[65536];


/**
 * Initializes the global packet dispatcher.
 *
 * Registers packet handlers by mapping each packet ID to its corresponding
 * handler function. This function is called once during program startup
 * before any packets are processed.
 *
 * Unregistered packet IDs remain NULL and are ignored by DispatchPacket().
 */
void DispatcherInit(void)
{
	dispatcher[_MSG_RESP_LOGINVALIDATE]          = RecvLoginValidate;
	dispatcher[_MSG_GAMESERVER_LOGINLOG]         = RecvGameLogin;
	dispatcher[_MSG_LOGIN_LOGINERRORRESP]        = RecvLoginError;
	dispatcher[_MSG_CLIENT_LOGINTOLRESP]         = RecvLoginError2;
	dispatcher[_MSG_RESP_CHATMESSAGE]            = RecvChatMessage;
	dispatcher[_MSG_RESP_ACTIVE]                 = RecvHeartBeat;
	dispatcher[_MSG_RESP_RESOURCEINFO]           = RecvResources;
	dispatcher[_MSG_RESP_UPDATE_RESOURCEAMOUNT]  = RecvRefreshResources;
	dispatcher[_MSG_RESP_BLACKMARKET_DATA]       = RecvCargoShipData;
	dispatcher[_MSG_RESP_BLACKMARKET_BUY]        = RecvBuyCargoShipItem;
	dispatcher[_MSG_RESP_ALLIANCE_NEEDHELP_INFO] = RecvPendingAllianceMembersNeedHelp;
	dispatcher[_MSG_RESP_RESHELPREPORTINFO]      = RecvResourceHelpReportInfo;
	dispatcher[_MSG_RESP_ALLIANCE_MEMBERINFO]    = RecvAllianceMemberInfo;
	
	// RecvHospitalInfo
	dispatcher[_MSG_HOSPITAL_HOSPITALINFO]       = RecvWoundedTroopData;
	// dispatcher[_MSG_RESP_ARMYGROUPINFO_]         = RecvArmyGroupInfo;
	
	// dispatcher[_MSG_RESP_BROCAST_NPC_WAR_BEGIN]  = RecvDarknestBroadcast;
	
	// dispatcher[_MSG_RESP_ADDCONFLICT_LINE] = RecvAddConflictLine;
	
	// dispatcher[_MSG_RESP_ITEMINFO] = RecvItemInfo;
	// dispatcher[_MSG_RESP_USEITEM] = RecvUseItem;
	// dispatcher[_MSG_RESP_BUYITEM] = RecvBuyItem;
	
	dispatcher[_MSG_RESP_ALLIANCE_SOMEBODY_NEEDHELP] = RecvAllianceMemberNeedsHelp;
	
	
	
	dispatcher[_MSG_RESP_ALLIANCE_MEMBERINFO] = RecvAllianceMemberInfo;
	
	dispatcher[_MSG_RESP_SOCIAL_DATA] = RecvSocialData;
	
	dispatcher[_MSG_RESP_ALLIANCE_UPDATEINFO] = RecvAllianceAttr;
	// dispatcher[_MSG_RESP_ALLIANCE_HELP] = RecvAllianceHelp;
	
	// dispatcher[0x0B2F] = RecvAllianceGiftInfo;
	
	// dispatcher[_MSG_MARCH_MARCHEVENTDATA] = RecvMarchData;
	
	// dispatcher[_MSG_RESP_BUILDINGINFO] = RecvAllBuildData;
	// dispatcher[_MSG_RESP_MAILINFO] = RecvMailInfo;
	
	// dispatcher[_MSG_RESP_RESEARCHINFO] = RecvTechnologyInfo;
	
	// dispatcher[_MSG_RESP_WARHALL_INITLIST] = RecvRallyCountData;
	
	// dispatcher[_MSG_RESP_ARMYGROUPINFO_] = RecvArmygroupInfo;
	// RecvAllianceNeedHelp
	
	
}


/**
 * Dispatches an incoming packet to its registered handler.
 *
 * Performs an O(1) lookup using the packet ID to retrieve the corresponding
 * handler from the global dispatch table. If no handler is registered for
 * the packet type, the packet is silently ignored.
 */
void DispatchPacket(Connection *c, uint16_t type, const uint8_t *data)
{
	PacketHandler handler = dispatcher[type];
	
	if (handler)
		handler(c, data);
	
}