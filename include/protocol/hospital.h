#ifndef HOSPITAL_H
#define HOSPITAL_H

/*
 * Hospital protocol module.
 *
 * Handles wounded troop and hospital information packets.
 */
 
#include <stdint.h>
#include <stdbool.h>

struct Connection;

/*
typedef struct {
    uint32_t total;

    uint32_t soldier_hospital[16];

    uint32_t treatment_qty;
    uint32_t treatment_soldier[16];

    uint64_t queue_id;
    uint32_t total_time;

} HospitalState;
*/


typedef struct {
	uint32_t total;
	uint32_t infantry[4];
	uint32_t cavalry[4];
	uint32_t ranged[4];
	uint32_t siege[4];
} HospitalTroopData;

/*
typedef struct {
	uint32_t total;
	uint32_t soldiers[16];
} HospitalTroopData;
*/

typedef struct {
	bool loaded;
	HospitalTroopData troop;
	HospitalTroopData healing;
	long num;
	uint32_t total_time;
} WoundedTroopData;

void RecvWoundedTroopData(struct Connection*, const uint8_t*);

#endif