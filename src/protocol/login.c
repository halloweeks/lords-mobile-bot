#include "protocol/login.h"
#include "net_rw.h"
#include "packet_enum.h"
#include "connection.h"
#include "log.h"

/* Gateway login */
void RequestGuestLogIn(Connection *c)
{
	// reserve space for packet length
    c->size = 2;
    
    // write packet type 
    write_u16(c->data + c->size, _MSG_NEWLOGIN_LOGINTOL);
    c->size += 2;
    
    // write igg id
    write_u64(c->data + c->size, c->auth.igg_id);
    c->size += 8;
    
    // write game minor version 
    write_u8(c->data + c->size, c->app.version_minor);
    c->size += 1;
    
    // write game major version
    write_u8(c->data + c->size, c->app.version_major);
    c->size += 1;
    
    // write game patch version
    write_u16(c->data + c->size, c->app.version_patch);
    c->size += 2;

    write_u8(c->data + c->size, 1);
    c->size += 1;
    
    // write language code
    write_u8(c->data + c->size, c->app.language_code);
    c->size += 1;
    
    // write Device Universally Unique Identifier.
    write_raw(c->data + c->size, c->auth.device_uuid, 50);
    c->size += 50;
    
    // write session length
    write_u16(c->data + c->size, c->auth.session_len);
    c->size += 2;
    
    // write session token
    write_raw(c->data + c->size, c->auth.session, 512);
    c->size += 512;
    
    // rewrite total packet length 
    write_u16(c->data, c->size);
    
    // call send packet
    send_packet(c, false);
}


/* Game login */
void RequestLogIn(Connection *c) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_NEWLOGIN_LOGINTOP);
	c->size += 2;
	
	// write igg id
	write_u64(c->data + c->size, c->auth.igg_id);
	c->size += 8;
	
	// write 50 byte zero 
	for (int i = 0; i < 25; i++) {
		write_u16(c->data + c->size, 0);
		c->size += 2;
	}
	
	// write version 
	write_u32(c->data + c->size, 0);
	c->size += 4;
	
	// battle_is_oul
	write_u8(c->data + c->size, 0);
	c->size += 1;
	
	// b_recv_kingdom
	write_u8(c->data + c->size, 0);
	c->size += 1;
	
	// session len
	write_u16(c->data + c->size, c->auth.session_len);
	c->size += 2;
	
	// session 
	write_raw(c->data + c->size, c->auth.session, 512);
    c->size += 512;

    write_u16(c->data, c->size);
    
    send_packet(c, false);
}

/* initialization over after game login */
void RequestClientInitOver(Connection *c) {
	c->size = 2; // reserve space for packet length
	
	// write packet type 
	write_u16(c->data + c->size, _MSG_REQUEST_CLIENTINITOVER);
	c->size += 2;
	
	// write sequence 
	write_u32(c->data + c->size, ++c->protocol.seq_id);
	c->size += 4;
	
	// write igg id
	write_u64(c->data + c->size, c->auth.igg_id);
	c->size += 8;
	
	write_u16(c->data, c->size);
    
    send_packet(c, true);
}


void RecvLoginValidate(Connection *c, const uint8_t *data)
{
	uint16_t offset = 0;
	
	c->game_server.port = read_i32(data + offset);      offset += 4;
	c->auth.igg_id      = read_i64(data + offset);      offset += 8;
	read_bytes(c->game_server.addr, data + offset, 16); offset += 16;
	
	// c->state = CONN_CONNECTING_GAME;
	c->state = CONN_GATEWAY_LOGIN_SUCCESS;
	LOGI("Gateway Login success! Switching to game server\n");
}


void RecvLoginError(Connection *c, const uint8_t *data) {
	uint8_t kind = read_u8(data);
	
	if (kind == 110) {
		// [ERROR] Client version outdated. Required: 2.197.309
		LOGE("UPDATE CLIENT VERSION\n");
	} else if (kind == 9) {
		LOGE("LOGGING FROM ANOTHER DEVICE errorCode: %u\n", kind);
	} else {
		LOGE("Bootstrap Login failed: %u\n", kind);
	}
	
	c->state = CONN_GATEWAY_LOGIN_FAILED;
}

void RecvLoginError2(Connection *c, const uint8_t *data) {
	int32_t kind = read_i32(data);
	
	LOGE("Bootstrap Login failed session expired: %d\n", kind);
	
	c->state = CONN_GATEWAY_LOGIN_FAILED;
} 

void RecvGameLogin(Connection *c, const uint8_t *data) {
	printf("[GAME] LOGIN OK\n");
	
	return;
}