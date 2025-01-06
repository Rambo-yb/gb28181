#ifndef __RTP_H__
#define __RTP_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
	RTP_TRANSPORT_STREAM_UDP,
	RTP_TRANSPORT_STREAM_TCP_ACTIVE,
	RTP_TRANSPORT_STREAM_TCP_PASSIVE,
};

void* RtpInit(int proto, const char* addr, int port, int ssrc);

void* RtpInitV2(int proto, int sockfd, int ssrc);

void RtpUnInit(void* hander);

int RtpAddDest(void* hander, const char* addr, int port);

int RtpTransportPort(void* hander);

int RtpPush(void* hander, unsigned char* packet, int size);

#ifdef __cplusplus
}
#endif

#endif