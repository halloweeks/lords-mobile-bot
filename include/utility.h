/* utility.h
 *
 * Generic utility functions.
 *
 * This module contains reusable helper functions that are not specific to
 * networking, the Lords Mobile protocol, or any particular subsystem.
 */

#ifndef UTILITY_H
#define UTILITY_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    RESOURCE_FOOD,
    RESOURCE_ROCK,
    RESOURCE_WOOD,
    RESOURCE_ORE,
    RESOURCE_GOLD
} ResourceType;

/*
 * Formats a number using K, M, or B suffixes.
 *
 * Examples:
 *   950      -> "950"
 *   1500     -> "1.50K"
 *   2500000  -> "2.50M"
 *   3200000000 -> "3.20B"
 */
void FormatNumber(uint64_t value, char *buffer, size_t size);

/*
 * Parses a number with an optional K, M, or B suffix.
 *
 * Examples:
 *   "500"    -> 500
 *   "1.5K"   -> 1500
 *   "2M"     -> 2000000
 *   "3.2B"   -> 3200000000
 */
uint64_t ParseNumber(const char *str);

/*
 * Writes a binary buffer to disk.
 *
 * This function is intended primarily for debugging and temporary data dumps.
 */
void DumpData(const char *filename, const void *data, size_t size);

uint8_t GetVIPLevel(uint32_t vipPoints);

const char *FormatTime(uint32_t totalSecs);

void ProcessMemoryUsage(void);
void program_memory();
double GetCPUUsage(void);



const char *GetResourceName(ResourceType type);

#endif /* UTILITY_H */