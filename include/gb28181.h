#ifndef __GB28181_H__
#define __GB28181_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "gb28181_common.h"

typedef enum {
	GB28181_OPERATION_CONTROL,
}Gb28181OperationType;

void Gb28181OperationRegister(int type, void* cb);

int Gb28181Init();

void Gb28181UnInit();

typedef struct {
	char sip_id[64];
	char sip_domain[64];
	char sip_addr[16];
	int sip_port;
	char sip_username[64];
	char sip_password[32];
	int register_valid;
	int keeplive_interval;
	int max_timeout;
	int protocol_type;
	int register_inretval;
}Gb28181Info;

int Gb28181Register(Gb28181Info* info);

int Gb28181Unregister(Gb28181Info* info);

typedef enum {
	GB28181_PUSH_REAL_TIME_STREAM,
	GB28181_PUSH_RECORD_STREAM,
}Gb28181PushStreamType;

int Gb28181PushStream(int type, unsigned char* stream, unsigned int size, int end);

int Gb28181PushAlarm(Gb28181AlarmInfo* info);

#ifdef __cplusplus
}
#endif

#endif