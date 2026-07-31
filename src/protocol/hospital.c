#include "protocol/hospital.h"
#include "net_rw.h"
#include "packet_enum.h"
#include "connection.h"
#include "log.h"

void WoundedTroopDataLog(Connection *c) {
	// T1 might: 2
	// T2 might: 8
	// T3 might: 24
	// T4 might: 36
	
	printf("Total Wounded: %u\n", c->wounded.troop.total);
	
	for (int i = 3; i >= 0; i--) {
		printf("Wounded T%d Infantry: %u\n", i + 1, c->wounded.troop.infantry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Wounded T%d Ranged: %u\n", i + 1, c->wounded.troop.ranged[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Wounded T%d Cavalry: %u\n", i + 1, c->wounded.troop.cavalry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Wounded T%d Siege: %u\n", i + 1, c->wounded.troop.siege[i]);
	}
	
	
	printf("Healing Total: %u\n", c->wounded.healing.total);
	
	for (int i = 3; i >= 0; i--) {
		printf("Healing T%d Infantry: %u\n", i + 1, c->wounded.healing.infantry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Healing T%d Ranged: %u\n", i + 1, c->wounded.healing.ranged[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Healing T%d Cavalry: %u\n", i + 1, c->wounded.healing.cavalry[i]);
	}
	
	for (int i = 3; i >= 0; i--) {
		printf("Healing T%d Siege: %u\n", i + 1, c->wounded.healing.siege[i]);
	}
	
	printf("num: %lu\n", c->wounded.num);
	printf("total time: %u\n", c->wounded.total_time);
	
	/*
	printf("Wounded Might: %llu\n",
       (unsigned long long)CalculateTroopMight(&c->wounded.troop));
       */
       
   //printf("Healing Might: %llu\n",
     //  (unsigned long long)CalculateTroopMight(&c->wounded.healing));
}

void RecvWoundedTroopData(Connection *c, const uint8_t *data) {
	uint16_t offset = 0;
	
	c->wounded.troop.total = 0;
	c->wounded.healing.total = 0;
	
	// Infantry 
	for (int index = 0; index < 4; index++) {
		c->wounded.troop.infantry[index] = read_u32(data + offset); offset += 4;
		c->wounded.troop.total += c->wounded.troop.infantry[index];
	}
	
	// Ranged 
	for (int index = 0; index < 4; index++) {
		c->wounded.troop.ranged[index] = read_u32(data + offset); offset += 4;
		c->wounded.troop.total += c->wounded.troop.ranged[index];
	}
	
	// Cavalry 
	for (int index = 0; index < 4; index++) {
		c->wounded.troop.cavalry[index] = read_u32(data + offset); offset += 4;
		c->wounded.troop.total += c->wounded.troop.cavalry[index];
	}
	
	// Siege
	for (int index = 0; index < 4; index++) {
		c->wounded.troop.siege[index] = read_u32(data + offset); offset += 4;
		c->wounded.troop.total += c->wounded.troop.siege[index];
	}
	
	
	// Infantry 
	for (int index = 0; index < 4; index++) {
		c->wounded.healing.infantry[index] = read_u32(data + offset); offset += 4;
		c->wounded.healing.total += c->wounded.healing.infantry[index];
	}
	
	// Ranged 
	for (int index = 0; index < 4; index++) {
		c->wounded.healing.ranged[index] = read_u32(data + offset); offset += 4;
		c->wounded.healing.total += c->wounded.healing.ranged[index];
	}
	
	// Cavalry 
	for (int index = 0; index < 4; index++) {
		c->wounded.healing.cavalry[index] = read_u32(data + offset); offset += 4;
		c->wounded.healing.total += c->wounded.healing.cavalry[index];
	}
	
	// Siege
	for (int index = 0; index < 4; index++) {
		c->wounded.healing.siege[index] = read_u32(data + offset); offset += 4;
		c->wounded.healing.total += c->wounded.healing.siege[index];
	}
	
	c->wounded.num = read_u64(data + offset); offset += 8;
	c->wounded.total_time  = read_u32(data + offset); offset += 4;
	
	c->wounded.loaded = true;
	
	// WoundedTroopDataLog(c);
}