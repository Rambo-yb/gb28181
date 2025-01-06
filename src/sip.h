#ifndef __SIP_H__
#define __SIP_H__

#include "libmxml4/mxml.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char device_id[64];
	char name[32];
	char manu_facturer[16];
	char model[16];
	char owner[16];
	char civil_code[64];
	char block[16];
	char address[16];
	int parental;
	char parent_id[64];
	int safety_way;
	int register_way;
	char cert_num[16];
	int certifiable;
	int err_code;
	int end_time;
	int secrecy;
	char ip_addr[16];
	int port;
	int password;
	float lon;
	float lat;
}SipCatalogInfo;

int SipMsgCatalog(void* st, mxml_node_t* tree, char** resp, void* cb);

int SipMsgDeviceStatus(void* st, mxml_node_t* tree, char** resp, void* cb);

int SipMsgDeviceInfo(void* st, mxml_node_t* tree, char** resp, void* cb);

int SipMsgRecordInfo(void* st, mxml_node_t* tree, char** resp, void* cb);

int SipMsgAlarm(void* st, mxml_node_t* tree, char** resp, void* cb);


int SipMsgPtzCmd(void* st, mxml_node_t* tree, char** resp, void* cb);

int SipMsgRecordCmd(void* st, mxml_node_t* tree, char** resp, void* cb);

int SipMsgGuardCmd(void* st, mxml_node_t* tree, char** resp, void* cb);


int SipNotifyKeepalive(int sn, char* device_id, char** resp);

int SipNotifyMediaStatus(int sn, char* device_id, char** resp);

int SipNotifyAlarm(int sn, char* device_id, void* st, char** resp);

#ifdef __cplusplus
}
#endif

#endif