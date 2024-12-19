#ifndef __RTP_H__
#define __RTP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RTP_PAYLOAD_H264 (98)
#define RTP_PAYLOAD_PS (96)

void* RtpInit(const char* addr, int port, int ssrc);

void RtpUnInit(void* hander);

int RtpAddDest(void* hander, const char* addr, int port);

int RtpTransportPort(void* hander);

int RtpPush(void* hander, unsigned char* packet, int size);

#ifdef __cplusplus
}
#endif

#endif