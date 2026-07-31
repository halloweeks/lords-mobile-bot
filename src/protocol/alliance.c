#include "protocol/alliance.h"
#include "net_rw.h"
#include "packet_enum.h"
#include "connection.h"
#include "log.h"

void RequestAllianceMemberInfo(Connection *c) {
	c->size = 2;
	write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_MEMBERINFO); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);   c->size += 4;
	write_u16(c->data, c->size);
	send_packet(c, true);
}

void RequestAllianceGiftInfo(Connection *c)
{
	c->size = 2;
	write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_GIFT_INFO); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id);            c->size += 4;
	write_u16(c->data, c->size);
	send_packet(c, true);
}

void RequestHelpAllianceMember(Connection *c, uint16_t record_sn_count, const uint32_t *record_sn)
{
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_HELP_SOMEBODY); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
	write_u16(c->data + c->size, record_sn_count); c->size += 2;
	
	for (uint16_t i = 0; i < record_sn_count; i++) {
		write_u32(c->data + c->size, record_sn[i]); c->size += 4;
	}
	
	write_u16(c->data, c->size);
	send_packet(c, true);
}

void RequestOpenAllianceGift(Connection *c, uint32_t SN) {
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_ALLIANCE_GIFT_OPENBOX); c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
	write_u32(c->data + c->size, SN); c->size += 4;
	write_u16(c->data, c->size);
	send_packet(c, true);
}


void RecvAllianceInfo(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	
	c->RoleAlliance.Channel = read_u32(data + offset); offset += 4;
	c->RoleAlliance.Rank = (AllianceRank)read_u8(data + offset); offset += 1;
	c->RoleAlliance.Apply = read_u8(data + offset); offset += 1;
	c->RoleAlliance.Money = read_u32(data + offset); offset += 4;
	
	return;
}


void RecvAllianceMemberNeedsHelp(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->help.record_sn      = read_u32(data + offset);          offset += 4;
	c->help.head           = read_u16(data + offset);          offset += 2;
	c->help.rank           = read_u8(data + offset);           offset += 1;
	read_raw(c->help.player_name, data + offset, 13);                  offset += 13;
	c->help.help_kind      = (HelpKind)read_u8(data + offset); offset += 1;
	c->help.event_id       = read_u16(data + offset);          offset += 2;
	c->help.event_data_lv  = read_u8(data + offset);           offset += 1;
	c->help.already_helped = read_u8(data + offset);           offset += 1;
	c->help.help_max       = read_u8(data + offset);           offset += 1;
	
	printf("[GUILD] record=%u event=%u player=%s\n",
       c->help.record_sn,
       c->help.event_id,
       c->help.player_name);
       
	if (c->alliance.auto_help) {
		RequestHelpAllianceMember(c, 1, &c->help.record_sn);
		printf("[GUILD] Sent help to %s\n", c->help.player_name);
	}
}

void RecvPendingAllianceMembersNeedHelp(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	bool do_not_update_ui = read_u8(data + offset); offset += 1;
	uint8_t count = read_u8(data + offset); offset += 1;
	
	for (int i = 0; i < count; i++) {
		c->help.record_sn = read_u32(data + offset); offset += 4;
		c->help.head      = read_u16(data + offset); offset += 2;
		c->help.rank      = read_u8(data + offset); offset += 1;
		read_raw(c->help.player_name, data + offset, 13); offset += 13;
		c->help.help_kind = (HelpKind)read_u8(data + offset); offset += 1;
		c->help.event_id = read_u16(data + offset); offset += 2;
		c->help.event_data_lv = read_u8(data + offset); offset += 1;
		c->help.already_helped = read_u8(data + offset); offset += 1;
		c->help.help_max = read_u8(data + offset); offset += 1;
		c->help.record_sn_arr[i] = c->help.record_sn;
	}
	
	if (c->alliance.auto_help) {
		RequestHelpAllianceMember(c, count, c->help.record_sn_arr);
		printf("[GUILD] Sent help to %u guild members\n", count);
	}
}



void RecvAllianceMemberInfo(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	printf("RecvAllianceMember()\n");
	
	
	uint8_t b  = read_u8(data + offset);  offset += 1;
	uint8_t b2 = read_u8(data + offset);  offset += 1;
	uint8_t b3 = read_u8(data + offset);  offset += 1;
	
	// printf("RecvAllianceMember(type=%u, finished=%u, count=%u)\n", b, b2, b3);
	
	if (b != 0 && b != 2)
	{
		// printf("RecvAllianceMember: unknown type %u\n", b);
		return;
	}
	
	if (b == 0) 
	{
		/* Start of a new full member list */
		if (c->alliance_member.data_finished) {
			c->alliance_member.recv_index = 0;
			c->alliance_member.count = 0;
		}
		
		c->alliance_member.count += b3;
		
		for (uint8_t i = 0; i < b3 && c->alliance_member.recv_index < MAX_ALLIANCE_MEMBER; i++)
		{
			
			AllianceMember *m = &c->alliance_member.member[c->alliance_member.recv_index];
			
			m->user_id         = read_i64(data + offset);  offset += 8;
			m->head            = read_u16(data + offset);  offset += 2;
			
			read_raw(m->name, data + offset, 13); offset += 13;
			m->rank            = read_u8(data + offset);   offset += 1;
			m->power           = read_u64(data + offset);  offset += 8;
			m->troop_kill_num  = read_u64(data + offset);  offset += 8;
			m->logout_time     = read_i64(data + offset);  offset += 8;
			m->white_list_flag = read_u8(data + offset);   offset += 1;
			c->alliance_member.recv_index++;
			
			// if (m->white_list_flag == 0) continue;
			
			printf("user_id: %ld\n", m->user_id);
			// printf("head: %u\n",     m->head);
			printf("name: %s\n",     m->name);
			// printf("rank: %u\n",     m->rank);
			// printf("power: %lu\n",   m->power);
			// printf("troop_kill_num: %lu\n", m->troop_kill_num);
			// printf("logout_time: %ld\n", m->logout_time);
			printf("white_list_flag: %u\n", m->white_list_flag);
			printf("\n");
		}
	} else if (b == 2) {
		for (int i = 0; i < (int)b3; i++) 
		{
			AllianceMember tmp;
			
			tmp.user_id         = read_i64(data + offset); offset += 8;
			tmp.head            = read_u16(data + offset); offset += 2;
			read_raw(tmp.name, data + offset, 13);         offset += 13;
			tmp.rank            = read_u8(data + offset);  offset += 1;
			tmp.power           = read_u64(data + offset); offset += 8;
			tmp.troop_kill_num  = read_u64(data + offset); offset += 8;
			tmp.logout_time     = read_i64(data + offset); offset += 8;
			tmp.white_list_flag = read_u8(data + offset);  offset += 1;
			
			for (int j = 0; j < MAX_ALLIANCE_MEMBER; j++)
			{
				if (c->alliance_member.member[j].user_id == tmp.user_id)
				{
					c->alliance_member.member[j] = tmp;
					
					printf("Updated member: %s (%lld)\n", tmp.name, (long long)tmp.user_id);
					
					break;
				}
			}
		}
	}
	
	
	c->alliance_member.data_finished = b2;
	
	if (b2 == 1)
	{
		c->alliance_member.recv_index = 0;
	}
	
}


void RecvAllianceAttr(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	uint8_t attr_type = read_u8(data + offset); offset += 1;
	
	// printf("\n\nRecvAllianceAttr\n");
	// printf("attr_type: %u\n\n", attr_type);
	
	if (attr_type == 19) {
		if (c->alliance.auto_open_gifts) {
			AllianceGift gift;
			gift.sn           = read_u32(data + offset); offset += 4;
			gift.status       = read_u8(data + offset);  offset += 1;
			gift.rcv_time     = read_u64(data + offset); offset += 8;
			gift.box_item_id  = read_u16(data + offset); offset += 2;
			gift.item_id      = read_u16(data + offset); offset += 2;
			gift.num          = read_u16(data + offset); offset += 2;
			gift.item_rank    = read_u8(data + offset);  offset += 1;
			read_raw(gift.player, data + offset, 13);    offset += 13;
			
			uint32_t gift_update_sn = read_u32(data + offset); offset += 4;
			
			RequestOpenAllianceGift(c, gift.sn);
		}
		return;
	}
	
	// Triggered when the alliance member count changes
	// (a player joins or leaves the alliance).
	// Payload:
	//   uint64_t alliance_power
	if (attr_type == 2) {
		uint64_t AlliancePower = read_u64(data + offset); offset += 8;
		return;
		printf("AlliancePower: %lu\n", AlliancePower);
		return;
	}
	
	// Triggered when the alliance member count changes
	// (a player joins or leaves the alliance).
	// Payload:
	//   uint8_t  member_count
	//   uint64_t alliance_power
	if (attr_type == 8) {
		uint8_t  AllianceMember = read_u8(data + offset); offset += 1;
		uint64_t AlliancePower  = read_u64(data + offset); offset += 8;
		return;
		printf("AllianceMember: %u\n", AllianceMember);
		printf("AlliancePower: %lu\n", AlliancePower);
		return;
	}
	
	
}


void AllianceMemberJoined(Connection *c)
{
	return; // exit
	
	// printf("[GUILD] %s joined the Guild!\n", c->chat.player_name);
	char msg[128];
	snprintf(msg, sizeof(msg), 
		"Glad to have you here, %s!",
		c->chat.player_name
	);
	
	RequestSendChat(c, 1, msg);
}

void AllianceMemberLeft(Connection *c)
{
	// printf("[GUILD] %s left the Guild.\n", c->chat.player_name);
}

void AllianceMemberKicked(Connection *c)
{
	// printf("[GUILD] %s was kicked out of thr Guild.\n", c->chat.player_name);
}

void AllianceMemberRankChanged(Connection *c)
{
	// printf("[GUILD] %s changed %s's Guild Rank to %u!\n", c->chat.message, c->chat.player_name, c->chat.pic_id);
}

void AllianceMemberRankRestored(Connection *c)
{
	// printf("[GUILD] %s's Guild Rank has automatically changed to R%u!\n", c->chat.player_name, c->chat.pic_id);
}

void AllianceMemberMessage(Connection *c) {
	// Perform command 
	// printf("[GUILD] [%s] %s\n", c->chat.player_name, c->chat.message);
}



/*
AllianceMember *FindMemberById(Connection *c, uint64_t user_id)
{
    for (uint16_t i = 0; i < c->alliance_member.count; i++) {
        if (c->alliance_member[i].user_id == user_id)
            return &c->alliance_member[i];
    }

    return NULL;
}

AllianceMember *FindMemberByName(Connection *c, const char *name)
{
    for (uint16_t i = 0; i < c->alliance_member.count; i++) {
        if (strcmp(c->alliance_member[i].name, name) == 0)
            return &c->alliance_member[i];
    }

    return NULL;
}
*/