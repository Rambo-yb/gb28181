#ifndef __SIP_H__
#define __SIP_H__

#ifdef __cplusplus
extern "C" {
#endif

int SipInit();

void SipUnInit();

int SipRegister(const char* rem_addr, int rem_port, const char* user, const char* password, const char* domain);

void SipUnRegister();

int SipPushStream(void* buff, int size);

#ifdef __cplusplus
}
#endif

#endif