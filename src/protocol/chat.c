#include "protocol/chat.h"
#include "net_rw.h"
#include "packet_enum.h"
#include "connection.h"

// #include "bot/command.h"

void RequestSendChat(Connection *c, uint8_t channel, const char *message) {
	uint16_t message_len = (uint16_t)strlen(message);
	
	c->size = 2;
	
	write_u16(c->data + c->size, _MSG_REQUEST_SENDCHAT);
	c->size += 2;
	
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	write_u8 (c->data + c->size, channel);
	c->size += 1;
	
	write_u8 (c->data + c->size, 0);
	c->size += 1;
	
	write_u8 (c->data + c->size, 5);
	c->size += 1;
	
	write_u16(c->data + c->size, message_len);
	c->size += 2;
	
	write_raw(c->data + c->size, message, message_len);
	c->size += message_len;
	
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
}


void RequestViewChat(Connection *c, uint8_t channel, uint8_t prev, int8_t kind, int64_t DataID, int64_t DataTime) {
	c->size = 2;
	
	// packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_VIEWCHAT);
	c->size += 2;
	
	// sequence id
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// channel type 
	write_u8 (c->data + c->size, channel);
	c->size += 1;
	
	// previous 
	write_u8 (c->data + c->size, prev);
	c->size += 1;
	
	if (c->app.version_major != 0) {
		write_u8(c->data + c->size, (kind == -1) ? 0xFF : (uint8_t)kind);
		c->size += 1;
		/*
		if (kind == -1) {
			write_u8 (c->data + c->size, 0xFF);
			c->size += 1;
		} else {
			write_u8 (c->data + c->size, (uint8_t)kind);
			c->size += 1;
		}
		*/
	}
	
	if (channel != 0) {
		write_u64(c->data + c->size, DataID);
		c->size += 8;
		
		write_u64(c->data + c->size, DataTime);
		c->size += 8;
	}
	
	write_u16(c->data, c->size); // update packet size
	send_packet(c, true);
}


void RecvChatMessage(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	memset(&c->chat, 0, sizeof(c->chat));
	
	
	// printf("RecvChatMessage\n");
	
	c->chat.b2 = read_u8(data + offset);
	offset += 1;
	
	// printf("b2: %u\n", c->chat.b2);
	
	if (c->app.version_major != 0) {
		c->chat.num3 = read_u8(data + offset);
		offset += 1;
		
		// printf("num3: %u\n", c->chat.num3);
	}
	
	if (c->chat.b2 == 0 || c->chat.b2 == 1) {
		c->chat.num4 = read_u16(data + offset);
		offset += 2;
		// printf("num4: %u\n", c->chat.num4);
		
		for (uint16_t i = 0; i < c->chat.num4; i++) {
			// talkTime
			c->chat.num5 = read_u64(data + offset);
			offset += 8;
			// playID
			c->chat.num6 = read_u64(data + offset);
			offset += 8;
			// talkID
			c->chat.num7 = read_u64(data + offset);
			offset += 8;
			
			c->chat.alli_or_king = read_u8(data + offset);
			offset += 1;
			c->chat.num8 = read_u8(data + offset);
			offset += 1;
			c->chat.pic_id = read_u16(data + offset);
			offset+= 2;
			read_bytes(c->chat.player_name, data + offset, 13);
			offset += 13;
			c->chat.vip_rank = read_u8(data + offset);
			offset+= 1;
			read_bytes(c->chat.title_name, data + offset, 3);
			offset += 3;
			c->chat.special_block_id = read_u8(data + offset);
			offset += 1;
			c->chat.title_id = read_u8(data + offset);
			offset += 1;
			c->chat.b_have_arabic = read_u8(data + offset);
			offset += 1;
			c->chat.num9 = read_u16(data + offset);
			offset += 2;
			
			/*
			printf("\n\n\n");
			printf("playID: %lu\n", c->chat.num6);
			printf("player_name: %s\n", c->chat.player_name);
			printf("vip_rank: %u\n", c->chat.vip_rank);
			printf("num8: %u\n", c->chat.num8);
			printf("\n\n\n");
			*/
			
			if (c->chat.num8 == 108) {
				c->chat.message_talk_kind = 3;
				c->chat.message_kingdom_id = read_u16(data + offset);
				offset += 2;
				
				read_bytes(c->chat.str1, data + offset, 3);
				offset += 3;
				read_bytes(c->chat.str2, data + offset, 13);
				offset += 13;
				// printf("[%s] %s\n", str1, str2);
			} else if (c->chat.num8 == 109) {
				c->chat.message_talk_kind = 0;
				c->chat.message_emoji_key = read_u16(data + offset);
				offset += 2;
				c->chat.message_num10 = read_u16(data + offset);
				offset += 2;
						
				// printf("message_emoji_key: %u\n", message_emoji_key);
				// printf("message_num10: %u\n", message_num10);
			} else if (c->chat.num8 == 0) {
				read_bytes(c->chat.message, data + offset, c->chat.num9);
				offset += c->chat.num9;
				c->chat.message[c->chat.num9] = '\0';
				
				AllianceMemberMessage(c);
				// printf("[%lu][%s]: %s\n", c->auth.igg_id, c->chat.player_name, c->chat.message);
				
				// command_handler(c, player_name, message);
				
				// memcpy(res.player_name, player_name, 13);
				//p.read_bytes(message, num9);
			} else if (c->chat.num8 == 101) {
				AllianceMemberLeft(c);
			} else if (c->chat.num8 == 102) {
				AllianceMemberKicked(c);
			} else if (c->chat.num8 == 104) {
				read_bytes(c->chat.message, data + offset, c->chat.num9);
				offset += c->chat.num9;
				c->chat.message[c->chat.num9] = '\0';
				AllianceMemberRankChanged(c);
			} else if (c->chat.num8 == 105) {
				AllianceMemberJoined(c);
			} else if (c->chat.num8 == 118) {
				// Inner circle auto rank set
				AllianceMemberRankRestored(c);
			}
		}
	}
	
	return;
}
