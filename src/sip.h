#ifndef __SIP_H__
#define __SIP_H__

typedef struct {
    void* buff;
    int size;
    unsigned long long timestamp;
}SipStreamFrame;

int SipInit();

void SipUnInit();

int SipPushStream(SipStreamFrame* frame);

#endif