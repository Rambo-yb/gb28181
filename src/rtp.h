#ifndef __RTP_H__
#define __RTP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RTP_PAYLOAD_H264 (98)
#define RTP_PAYLOAD_PS (96)

int RtpInit(const char* addr, int port, int ssrc);

void RtpUnInit();

int RtpAddDest(const char* addr, int port);

int RtpPush(unsigned char* packet, int size);

#ifdef __cplusplus
}
#endif

#endif