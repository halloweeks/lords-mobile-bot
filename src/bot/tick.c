#include "bot/tick.h"
#include "connection.h"
#include <time.h>

#include "utility.h"

#include "log.h"

void BotTick(Connection *c)
{
	if (c->server_time == 0) return;
	
	time_t now = time(NULL);
	
	if (now - c->last_tick_time < 15) return;
	
	c->last_tick_time = now;
	
	RequestHeartBeat(c);
	
	
	#ifdef _DEBUG_
		static time_t last_cpu_log = 0;
		
		if (now != last_cpu_log) {
			last_cpu_log = now;
			// ProcessMemoryUsage();
			LOGI("CPU Usage: %.2f%%\n", GetCPUUsage());
		}
	#endif
	
	// restart scheduler
	if (c->action_state >= ACTION_MAX) {
		c->action_state = 1;
	}
	
	switch (c->action_state) {
		case ACTION_CARGO: 
			CargoShipTick(c);
			LOGD("Called CargoShipTick()\n");
			break;
		case ACTION_SHIELD:
			// ShieldTick(c);
			LOGD("Called ShieldTick()\n");
			break;
		case ACTION_HEAL:
			// HealTick(c);
			LOGD("Called HealTick()\n");
			break;
		case ACTION_GATHER:
			// GatherTick(c);
			LOGD("Called GatherTick()\n");
			break;
		default: 
			break;
	}
	
	c->action_state++;
}