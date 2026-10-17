#ifndef LOGIN_H
#define LOGIN_H

#include <stdint.h>

typedef struct AuthInfo {
    int64_t igg_id;
    char device_uuid[50];
    uint8_t device_type;   /* 1 = Android, 9 = Desktop */
    uint16_t session_len;
    char session[512];
} ClientAuth;

struct Connection;

void RequestGuestLogIn(struct Connection*);
void RequestLogIn(struct Connection*);
void RequestClientInitOver(struct Connection*);

void RecvLoginValidate(struct Connection*, const uint8_t*);
void RecvLoginError(struct Connection*, const uint8_t*);
void RecvLoginError2(struct Connection*, const uint8_t*);
void RecvGameLogin(struct Connection*, const uint8_t*);

void RecvLoginRoleInfo(struct Connection*, const uint8_t*);

// These are temporarily 
void RecvMagicGateDoEvent(struct Connection*, const uint8_t*);

void RecvTDInfo(struct Connection*, const uint8_t*);
void RecvTDTriggerInfo(struct Connection*, const uint8_t*);

#endif