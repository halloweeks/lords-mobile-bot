#ifndef CHAT_H
#define CHAT_H

#include <stdint.h>
#include <stdbool.h>

struct Connection;


typedef struct {
    char player_name[13];
    char title_name[3];
    char str1[20];
	char str2[20];
    char message[1024];
    uint8_t num3;
    uint8_t b2;
    uint16_t num4;
    // talkTime
	int64_t num5;
	// playID
	int64_t num6;
	// talkID
	int64_t num7;
	uint8_t alli_or_king;
	uint8_t num8;
	uint16_t pic_id;
	uint8_t vip_rank;
	uint8_t special_block_id;
	uint8_t title_id;
	uint8_t b_have_arabic;
	uint16_t num9;
	int message_talk_kind;
	uint16_t message_kingdom_id;
	uint16_t message_emoji_key;
	uint16_t message_num10;
} ChatState;


void RequestSendChat(struct Connection*, uint8_t, const char*);
void RequestViewChat(struct Connection*, uint8_t, uint8_t, int8_t, int64_t, int64_t);

void RecvChatMessage(struct Connection*, const uint8_t*);

#endif