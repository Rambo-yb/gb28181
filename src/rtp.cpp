#include <stdlib.h>
#include <unistd.h>
#include "rtp.h"
#include "check_common.h"

#include "rtpsessionparams.h"
#include "rtpudpv4transmitter.h"
#include "rtpsession.h"
#include "rtptcptransmitter.h"
#include "rtptcpaddress.h"

using namespace jrtplib;

#define RTP_PAYLOAD_H264 (98)
#define RTP_PAYLOAD_PS (96)

typedef struct {
	int proto;
	int tcp_sockfd;
    int rtp_port;
    RTPSession sess;
    RTPSessionParams sess_param;
    RTPUDPv4TransmissionParams udp_trans_param;
	RTPTCPTransmitter* tcp_trans;
}RtpInfo;

#define RTP_TRANSPORT_PORT (4000)
#define RTP_TRANSPORT_MAX_PORT (4010)
#define RTP_MAX_PACKET_SIZE (1200)

static int RtpCreateTcpClient(RtpInfo* info, const char* addr, int port) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	CHECK_LE(sockfd, 0, return -1);

	struct sockaddr_in ser_addr;
	memset(&ser_addr, 0, sizeof(struct sockaddr_in));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(port);
	inet_aton(addr, &ser_addr.sin_addr);
	
	int ret = connect(sockfd, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
	CHECK_LT(ret, 0, close(sockfd);return -1);
	
	struct sockaddr_in loc_addr;
	socklen_t len = sizeof(loc_addr);
	getsockname(sockfd, (struct sockaddr*)&loc_addr, &len);

	info->tcp_sockfd = sockfd;
	info->rtp_port = ntohs(loc_addr.sin_port);
	LOG_INFO("sockfd:%d, loc port:%d", info->tcp_sockfd, info->rtp_port);

	return 0;
}

#define MAX_CLIENT_NUM 5
static int RtpCreateTcpServer(int port) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("Failed to create socket");
        return -1;
    }
	
	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));

	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr_in)) < 0) {
        perror("Failed to bind address");
		close(sockfd);
        return -1;
    }
	
    if (listen(sockfd, MAX_CLIENT_NUM) < 0) {
        perror("Failed to listen");
		close(sockfd);
        return -1;
    }
	
	return sockfd;
}

void* RtpInit(int proto, const char* addr, int port, int ssrc) {
    RtpInfo* info = new RtpInfo;
    CHECK_POINTER(info, return NULL);

	info->proto = proto;

    info->sess_param.SetOwnTimestampUnit(1.0/90000.0);
    info->sess_param.SetUsePredefinedSSRC(true);
    info->sess_param.SetPredefinedSSRC(ssrc);

	if (proto == RTP_TRANSPORT_STREAM_UDP) {
		info->rtp_port = RTP_TRANSPORT_PORT;
		do {
			info->udp_trans_param.SetPortbase(info->rtp_port);
			int ret = info->sess.Create(info->sess_param, &info->udp_trans_param);
			if (ret == 0) {
				break;
			}

			if (info->rtp_port >= RTP_TRANSPORT_MAX_PORT) {
				delete info;
				LOG_ERR("rtp create fail !");
				return NULL;
			}
			info->rtp_port += 2;
			usleep(1000*100);
		}while (1);

		int ret = info->sess.AddDestination(RTPIPv4Address(ntohl(inet_addr(addr)), port));
		CHECK_LT(ret, 0, delete info; return NULL);
	} else {
		int ret = RtpCreateTcpClient(info, addr, port);
		CHECK_LT(ret, 0, delete info; return NULL);

		info->tcp_trans = new RTPTCPTransmitter(NULL);
		info->tcp_trans->Init(true);
		info->tcp_trans->Create(65535, 0);
		ret = info->sess.Create(info->sess_param, info->tcp_trans);
		CHECK_LT(ret, 0, close(info->tcp_sockfd); delete info; return NULL);

		ret = info->sess.AddDestination(RTPTCPAddress(info->tcp_sockfd));
		CHECK_LT(ret, 0, close(info->tcp_sockfd); delete info; return NULL);
	}

    info->sess.SetDefaultPayloadType(RTP_PAYLOAD_PS);
    info->sess.SetDefaultMark(false);
    info->sess.SetDefaultTimestampIncrement(0);

    return (void*)info;
}

void* RtpInitV2(int proto, int sockfd, int ssrc) {
    RtpInfo* info = new RtpInfo;
    CHECK_POINTER(info, return NULL);

	info->proto = proto;
	info->tcp_sockfd = sockfd;

    info->sess_param.SetOwnTimestampUnit(1.0/90000.0);
    info->sess_param.SetUsePredefinedSSRC(true);
    info->sess_param.SetPredefinedSSRC(ssrc);

	info->tcp_trans = new RTPTCPTransmitter(NULL);
	info->tcp_trans->Init(true);
	info->tcp_trans->Create(65535, 0);
	int ret = info->sess.Create(info->sess_param, info->tcp_trans);
	CHECK_LT(ret, 0, delete info; return NULL);

	ret = info->sess.AddDestination(RTPTCPAddress(sockfd));
	CHECK_LT(ret, 0, delete info; return NULL);

    info->sess.SetDefaultPayloadType(RTP_PAYLOAD_PS);
    info->sess.SetDefaultMark(false);
    info->sess.SetDefaultTimestampIncrement(0);

    return (void*)info;
}

void RtpUnInit(void* hander) {
    CHECK_POINTER(hander, return);

    RtpInfo* info = (RtpInfo* )hander;

	RTPTime delay = RTPTime(1.0);
    info->sess.BYEDestroy(delay, "28181 stop play", strlen("28181 stop play"));
    
	if (info->proto != RTP_TRANSPORT_STREAM_UDP) {
		close(info->tcp_sockfd);
	}

    delete info;
}

int RtpAddDest(void* hander, const char* addr, int port) {
    CHECK_POINTER(hander, return -1);
    CHECK_POINTER(addr, return -1);

    RTPIPv4Address dest_addr(ntohl(inet_addr(addr)), port);
    int ret = ((RtpInfo*)hander)->sess.AddDestination(dest_addr);
    CHECK_LT(ret, 0, return -1);

    return 0;
}

static unsigned char* RtpFindNalu(const unsigned char* buff, int len, int* size) {
	unsigned char *s = NULL;
	while (len >= 3) {
		if (buff[0] == 0 && buff[1] == 0 && buff[2] == 1) {
			if (!s) {
				if (len < 4)
					return NULL;
				s = (unsigned char *)buff;
			} else {
				*size = (buff - s);
				return s;
			}
			buff += 3;
			len  -= 3;
			continue;
		}
		if (len >= 4 && buff[0] == 0 && buff[1] == 0 && buff[2] == 0 && buff[3] == 1) {
			if (!s) {
				if (len < 5)
					return NULL;
				s = (unsigned char *)buff;
			} else {
				*size = (buff - s);
				return s;
			}
			buff += 4;
			len  -= 4;
			continue;
		}
		buff ++;
		len --;
	}
	if (!s)
		return NULL;
	*size = (buff - s + len);
	return s;
}

static int RtpSendH264Packet(RtpInfo* info, unsigned char* packet, int size) {
    CHECK_BOOL(packet, return -1);
    CHECK_LE(size, 0, return -1);

    if (packet[0] == 0 && packet[1] == 0 && packet[2] == 1) {
        packet += 3;
        size -= 3;
    }

    if (packet[0] == 0 && packet[1] == 0 && packet[2] == 0 && packet[3] == 1) {
        packet += 4;
        size -= 4;
    }

    unsigned int ts = 90000/25;

    if (size <= RTP_MAX_PACKET_SIZE) {
        info->sess.SendPacket((const void *)packet, size, RTP_PAYLOAD_H264, true, ts);
    } else {
        int payload_len = RTP_MAX_PACKET_SIZE - 2;
        int offset = 1;

        unsigned char ualhdr = packet[0];
        unsigned char pkt[RTP_MAX_PACKET_SIZE] = {0};
        pkt[0] = (ualhdr & 0xe0) | 28;
        pkt[1] = (ualhdr & 0x1f) | 0x80;
        memcpy(pkt+2, packet+offset, RTP_MAX_PACKET_SIZE - 2);
        info->sess.SendPacket((const void *)pkt, RTP_MAX_PACKET_SIZE, RTP_PAYLOAD_H264, false, 0);
        offset += (RTP_MAX_PACKET_SIZE - 2);

        pkt[1] = packet[4] & 0x1f;
        while(offset + (RTP_MAX_PACKET_SIZE - 2) < size) {
            memcmp(pkt+2, packet+offset, RTP_MAX_PACKET_SIZE - 2);
            info->sess.SendPacket((const void *)pkt, RTP_MAX_PACKET_SIZE, RTP_PAYLOAD_H264, false, 0);
            offset += (RTP_MAX_PACKET_SIZE - 2);
        }

        pkt[1] = 0x40 | (packet[4] & 0x1F);
        memcmp(pkt+2, packet+offset, size - offset);
        info->sess.SendPacket((const void *)pkt, size - offset, RTP_PAYLOAD_H264, true, ts);
    }

    return 0;
}

static int RtpTransH264(RtpInfo* info, unsigned char* packet, int size) {
    int start = 0;
    while(start < size) {
        int len = 0;
        unsigned char* p = RtpFindNalu(packet + start, size - start, &len);
        if (p == NULL) {
            break;
        }

        RtpSendH264Packet(info, p, len);
        start = p - packet + len;
    }

    return 0;
}

static unsigned char kPsHead[] = {
    0x00, 0x00, 0x01, 0xba, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x01, 0x47, 0xb3, 
    0xf8
};

static unsigned char kSysMapHead[] = {
    0x00, 0x00, 0x01, 0xbb, 
    0x00, 0x0c, 
    0x80, 0xa3, 0xd9, 
    0x04, 0xe1, 
    0xff, 
    0xb9, 0xe0, 0x00, 0xb8, 0xc0, 0x40,
    0x00, 0x00, 0x01, 0xbc,
    0x00, 0x12,
    0xe1, 0xff,
    0x00, 0x00, 0x00, 0x08,
    0x1b, 0xe0, 0x00, 0x00,
    0x90, 0xc0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static unsigned char kPesHead[] = {
    0x00, 0x00, 0x01, 0xe0,
    0x00, 0x00,
    0x80, 0xc0,
    0x0a,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void SetPsHeaderTimeStamp(unsigned char* dest, unsigned long long ts)
{
    unsigned char *scr_buf = dest + 4;
    scr_buf[0] = 0x40 | (((unsigned char)(ts >> 30) & 0x07) << 3) | 0x04 | ((unsigned char)(ts >> 28) & 0x03);
    scr_buf[1] = (unsigned char)((ts >> 20) & 0xff);
    scr_buf[2] = (((unsigned char)(ts >> 15) & 0x1f) << 3) | 0x04 | ((unsigned char)(ts >> 13) & 0x03);
    scr_buf[3] = (unsigned char)((ts >> 5) & 0xff);
    scr_buf[4] = (((unsigned char)ts & 0x1f) << 3) | 0x04;
    scr_buf[5] = 1;
}

void RtpSetPesTimeStamp(unsigned char* buff, unsigned long long ts) {
    buff += 9;
    // PTS
    buff[0] = (unsigned char)(((ts >> 30) & 0x07) << 1) | 0x30 | 0x01;
    buff[1] = (unsigned char)((ts >> 22) & 0xff);
    buff[2] = (unsigned char)(((ts >> 15) & 0xff) << 1) | 0x01;
    buff[3] = (unsigned char)((ts >> 7) & 0xff);
    buff[4] = (unsigned char)((ts & 0xff) << 1) | 0x01;
    // DTS
    buff[5] = (unsigned char)(((ts >> 30) & 0x07) << 1) | 0x10 | 0x01;
    buff[6] = (unsigned char)((ts >> 22) & 0xff);
    buff[7] = (unsigned char)(((ts >> 15) & 0xff) << 1) | 0x01;
    buff[8] = (unsigned char)((ts >> 7) & 0xff);
    buff[9] = (unsigned char)((ts & 0xff) << 1) | 0x01;
}

int RtpGetSinglePesHeader(unsigned char* header, unsigned long long ts, int len) {
    // len += 13;
    memcpy(header, kPesHead, sizeof(kPesHead));
    *(header+4) = (unsigned char)(len>>8);
    *(header+5) = (unsigned char)len;
 
    RtpSetPesTimeStamp(header, ts);
    return sizeof(kPesHead);
}

int RtpGetPsHeader(unsigned char* header, int farme_type, unsigned long long ts, int farme_len) {
    int ret = 0;

    memcpy(header, kPsHead, sizeof(kPsHead));
    SetPsHeaderTimeStamp(header, ts);
    header += sizeof(kPsHead);
    ret += sizeof(kPsHead);

    if (farme_type == 1) {
        memcpy(header, kSysMapHead, sizeof(kSysMapHead));
        header += sizeof(kSysMapHead);
        ret += sizeof(kSysMapHead);
    }

    ret += RtpGetSinglePesHeader(header, ts, farme_len);

    return ret;
}

#define RTP_MAX_PS_SIZE (64*1024)
#define RTP_MAX_PES_SIZE (65400)
#define RTP_PS_HEAD_LEN (sizeof(kPsHead)+sizeof(kSysMapHead)+sizeof(kPesHead))
#define RTP_PES_HEAD_LEN (sizeof(kPesHead))

static int RtpSendPsPacket(RtpInfo* info, unsigned char* packet, int size) {
    CHECK_BOOL(packet, return -1);
    CHECK_LE(size, 0, return -1);

    int offset = 0;
    unsigned char pkt[RTP_MAX_PACKET_SIZE] = {0};
    while(offset + RTP_MAX_PACKET_SIZE < size) {
        info->sess.SendPacket((const void *)(packet + offset), RTP_MAX_PACKET_SIZE, RTP_PAYLOAD_PS, false, 0);
        offset += RTP_MAX_PACKET_SIZE;
    }

    info->sess.SendPacket((const void *)(packet + offset), size - offset, RTP_PAYLOAD_PS, true, 90000/25);

    return 0;
}

static int RtpTransPs(RtpInfo* info, unsigned char* packet, int size) {
    static unsigned long long ts = 0;

    int frame_type = 0;
    if (packet[0] == 0 && packet[1] == 0 && packet[2] == 0 && packet[3] == 1 && packet[4] == 0x67) {
        frame_type = 1;
    }

    if (packet[0] == 0 && packet[1] == 0 && packet[2] == 0 && packet[3] == 0x67) {
        frame_type = 1;
    }

    int offset = 0;
    int len = size > RTP_MAX_PES_SIZE ? RTP_MAX_PES_SIZE : size;

    unsigned char ps_frame_buff[RTP_MAX_PS_SIZE];
    int ret = RtpGetPsHeader(ps_frame_buff, frame_type, ts, len);
    memcpy(ps_frame_buff + ret, packet, len);
    RtpSendPsPacket(info, ps_frame_buff, len+ret);
    offset += len;

    while (offset < size) {
        len = size - offset > RTP_MAX_PES_SIZE ? RTP_MAX_PES_SIZE : size - offset; 

        memset(ps_frame_buff, 0, RTP_MAX_PS_SIZE);
        RtpGetSinglePesHeader(ps_frame_buff, 0, len);
        memcpy(ps_frame_buff + RTP_PES_HEAD_LEN, packet + offset, len);
        RtpSendPsPacket(info, ps_frame_buff, len+RTP_PES_HEAD_LEN);
        offset += len;
    }
 
    ts += 3600;
    return 0;
}

int RtpPush(void* hander, unsigned char* packet, int size) {
    CHECK_POINTER(hander, return -1);
    CHECK_POINTER(packet, return -1);
    CHECK_LE(size, 0, return -1);

    // RtpTransH264((RtpInfo*)hander, packet, size);

    RtpTransPs((RtpInfo*)hander, packet, size);

    return 0;
}

int RtpTransportPort(void* hander) {
    return ((RtpInfo*)hander)->rtp_port;
}