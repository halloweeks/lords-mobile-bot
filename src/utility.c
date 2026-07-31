/* utility.c */

#include "utility.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include "log.h"

void FormatNumber(uint64_t value, char *buffer, size_t size)
{
    if (value >= 1000000000ULL)
        snprintf(buffer, size, "%.2fB", value / 1000000000.0);
    else if (value >= 1000000ULL)
        snprintf(buffer, size, "%.2fM", value / 1000000.0);
    else if (value >= 1000ULL)
        snprintf(buffer, size, "%.2fK", value / 1000.0);
    else
        snprintf(buffer, size, "%" PRIu64, value);
}

uint64_t ParseNumber(const char *str)
{
    double value = 0.0;
    char suffix = '\0';

    if (sscanf(str, "%lf %c", &value, &suffix) < 1)
        return 0;

    switch (tolower((unsigned char)suffix))
    {
        case 'k':
            value *= 1000.0;
            break;

        case 'm':
            value *= 1000000.0;
            break;

        case 'b':
            value *= 1000000000.0;
            break;

        default:
            break;
    }

    if (value < 0.0)
        value = 0.0;

    return (uint64_t)value;
}

void DumpData(const char *filename, const void *data, size_t size)
{
    char path[1024];

    snprintf(path, sizeof(path), "%s.bin", filename);

    FILE *fp = fopen(path, "wb");
    if (!fp)
    {
        perror("fopen");
        return;
    }

    if (fwrite(data, 1, size, fp) != size)
        perror("fwrite");

    fclose(fp);

    printf("Saved %zu bytes to %s\n", size, path);
}

uint8_t GetVIPLevel(uint32_t vipPoints)
{
	if (vipPoints >= 1500000) return 15;
	if (vipPoints >= 730000)  return 14;
	if (vipPoints >= 350000)  return 13;
	if (vipPoints >= 175000)  return 12;
	if (vipPoints >= 90000)   return 11;
	if (vipPoints >= 50000)   return 10;
	if (vipPoints >= 20000)   return 9;
	if (vipPoints >= 8000)	return 8;
	if (vipPoints >= 4000)	return 7;
	if (vipPoints >= 1600)	return 6;
	if (vipPoints >= 800)	 return 5;
	if (vipPoints >= 400)	 return 4;
	if (vipPoints >= 300)	 return 3;
	if (vipPoints >= 100)	 return 2;
	
	return 1;
}


const char *FormatTime(uint32_t totalSecs) {
    static char buffer[64];
    uint32_t secs = totalSecs;

    uint32_t days    = secs / 86400; secs %= 86400;
    uint32_t hours   = secs / 3600;  secs %= 3600;
    uint32_t minutes = secs / 60;    secs %= 60;

    if (days > 0 && hours > 0)
        snprintf(buffer, sizeof(buffer), "%ud %uh", days, hours);
    else if (days > 0)
        snprintf(buffer, sizeof(buffer), "%ud", days);
    else if (hours > 0 && minutes > 0)
        snprintf(buffer, sizeof(buffer), "%uh %um", hours, minutes);
    else if (hours > 0)
        snprintf(buffer, sizeof(buffer), "%uh", hours);
    else if (minutes > 0 && secs > 0)
        snprintf(buffer, sizeof(buffer), "%um %us", minutes, secs);
    else if (minutes > 0)
        snprintf(buffer, sizeof(buffer), "%um", minutes);
    else
        snprintf(buffer, sizeof(buffer), "%us", secs);

    return buffer;
}



/*
void program_memory() {
	FILE *fp = fopen("/proc/self/status", "r");
	
	if (fp == NULL) return;
	
	char line[256];
	
	while (fgets(line, sizeof(line), fp)) {
		if (strncmp(line, "VmRSS:", 6) == 0) {
			LOGI("Real memory usage: %s", line);
			break;
		}
	}
	
	fclose(fp);
}*/

void ProcessMemoryUsage(void)
{
	FILE *fp = fopen("/proc/self/status", "r");

	if (fp == NULL)
		return;

	char line[256];

	while (fgets(line, sizeof(line), fp)) {
		if (strncmp(line, "VmRSS:", 6) == 0) {
			long kb;

			sscanf(line, "VmRSS: %ld kB", &kb);

			size_t bytes = (size_t)kb * 1024;
			double mb = (double)bytes / (1024 * 1024);

			LOGI("Real memory usage: %zu bytes (%.2f MB)\n",
			     bytes,
			     mb);
			break;
		}
	}

	fclose(fp);
}

#include <time.h>
#include <sys/resource.h>

double GetCPUUsage(void)
{
	static double prev_cpu = 0.0;
	static double prev_wall = 0.0;
	
	struct rusage usage;
	struct timespec ts;
	
	getrusage(RUSAGE_SELF, &usage);
	clock_gettime(CLOCK_MONOTONIC, &ts);
	
	double cpu =
		usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0 +
		usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0;
	
	double wall =
		ts.tv_sec + ts.tv_nsec / 1000000000.0;
	
	if (prev_wall == 0.0) {
		prev_cpu = cpu;
		prev_wall = wall;
		return 0.0;
	}
	
	double cpu_usage = (cpu - prev_cpu) / (wall - prev_wall) * 100.0;
	
	prev_cpu = cpu;
	prev_wall = wall;
	
	return cpu_usage;
}

const char *GetResourceName(ResourceType type)
{
    switch (type) {
        case RESOURCE_FOOD:
            return "food";

        case RESOURCE_ROCK:
            return "rock";

        case RESOURCE_WOOD:
            return "wood";

        case RESOURCE_ORE:
            return "ore";

        case RESOURCE_GOLD:
            return "gold";

        default:
            return "unknown";
    }
}