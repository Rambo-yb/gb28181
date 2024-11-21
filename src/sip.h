#ifndef __SIP_H__
#define __SIP_H__

#ifdef __cplusplus
extern "C" {
#endif

int SipInit();

void SipUnInit();

int SipPushStream(void* buff, int size);

#ifdef __cplusplus
}
#endif

#endif