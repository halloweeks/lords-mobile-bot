#ifndef ALLIANCE_H
#define ALLIANCE_H

#include <stdint.h>

struct Connection;

typedef enum {
    Research = 0,
    Building,
    Max,
} HelpKind;

typedef struct {
	uint32_t record_sn;
	uint16_t head;
	uint8_t rank;
	char player_name[13];
	HelpKind help_kind;
	uint16_t event_id;
	uint8_t event_data_lv;
	uint8_t already_helped;
	uint8_t help_max;
	uint32_t record_sn_arr[100];
} AllianceHelp;


#define MAX_ALLIANCE_MEMBER 100

typedef struct {
    int64_t user_id;
    uint16_t head;
    char name[13];
    uint8_t rank;
    uint64_t power;
    uint64_t troop_kill_num;
    int64_t logout_time;
    uint8_t white_list_flag;
} AllianceMember;

typedef struct {
    AllianceMember member[MAX_ALLIANCE_MEMBER];
    uint16_t recv_index;
    uint8_t data_finished;
    uint8_t count;
} AllianceMemberList;


typedef struct {
    uint32_t sn;
    uint8_t status;
    int64_t rcv_time;
    uint16_t box_item_id;
    uint16_t item_id;
    uint16_t num;
    uint8_t item_rank;
    // Option
    uint32_t diamond;
    uint32_t money;
    char player[13];
} AllianceGift;

void RequestAllianceMemberInfo(struct Connection*);

void RequestAllianceGiftInfo(struct Connection*);


void RequestHelpAllianceMember(struct Connection*, uint16_t, const uint32_t*);

void RequestOpenAllianceGift(struct Connection*, uint32_t);

void RecvAllianceInfo(struct Connection*, const uint8_t*);
void RecvAllianceMemberNeedsHelp(struct Connection*, const uint8_t*);
void RecvPendingAllianceMembersNeedHelp(struct Connection*, const uint8_t*);

void RecvAllianceMemberInfo(struct Connection*, const uint8_t*);

void RecvAllianceAttr(struct Connection*, const uint8_t*);


// alliance.h
void AllianceMemberJoined(struct Connection*);
void AllianceMemberLeft(struct Connection*);
void AllianceMemberKicked(struct Connection*);
void AllianceMemberRankChanged(struct Connection*);
void AllianceMemberRankRestored(struct Connection*);
void AllianceMemberMessage(struct Connection*);

AllianceMember *FindMemberById(struct Connection*, uint64_t);
AllianceMember *FindMemberByName(struct Connection*, const char*);

#endif