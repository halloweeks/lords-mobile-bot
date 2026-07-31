#include "protocol/heartbeat.h"
#include "connection.h"
#include "net_rw.h"
#include "packet_enum.h"
#include "log.h"

void RequestHeartBeat(Connection *c)
{
	c->size = 2;
	write_u16(c->data + c->size, _MSG_REQUEST_ACTIVE);  c->size += 2;
	write_u32(c->data + c->size, ++c->protocol.seq_id); c->size += 4;
	write_u16(c->data, c->size);
	send_packet(c, true);
	
	LOGD("called RequestHeartBeat()\n");
}

void RecvHeartBeat(Connection *c, const uint8_t *data) {
	c->server_time = read_u64(data);
	
	LOGD("called RecvHeartBeat()\n");
	LOGD("Server time: %lu\n", c->server_time);
}