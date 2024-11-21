#include <stdlib.h>
#include <unistd.h>
#include "rtp.h"
#include "check_common.h"

#include "rtpsessionparams.h"
#include "rtpudpv4transmitter.h"
#include "rtpsession.h"

using namespace jrtplib;

typedef struct {
    bool init;
    RTPSession sess;
    RTPSessionParams sess_param;
    RTPUDPv4TransmissionParams trans_param;
}RtpMng;
static RtpMng kRtpMng;

#define RTP_TRANSPORT_PORT (4000)
#define RTP_MAX_PACKET_SIZE (1200)

int RtpInit(const char* addr, int port, int ssrc) {
    if (kRtpMng.init) {
        return 0;
    }

    kRtpMng.sess_param.SetOwnTimestampUnit(1.0/90000.0);
    kRtpMng.trans_param.SetPortbase(RTP_TRANSPORT_PORT);
    // kRtpMng.sess_param.SetUsePredefinedSSRC(true);
    // kRtpMng.sess_param.SetPredefinedSSRC(ssrc);

    int ret = kRtpMng.sess.Create(kRtpMng.sess_param, &kRtpMng.trans_param);
    CHECK_LT(ret, 0, return -1);

    kRtpMng.sess.SetDefaultPayloadType(96);
    kRtpMng.sess.SetDefaultMark(false);
    kRtpMng.sess.SetDefaultTimestampIncrement(0);

    RtpAddDest(addr, port);
    kRtpMng.init = true;

    return 0;
}

void RtpUnInit() {
    if (!kRtpMng.init) {
        return ;
    }

	RTPTime delay = RTPTime(1.0);
    kRtpMng.sess.BYEDestroy(delay, "28181 stop play", strlen("28181 stop play"));
    kRtpMng.init = false;
}

int RtpAddDest(const char* addr, int port) {
    CHECK_POINTER(addr, return -1);

    RTPIPv4Address dest_addr(ntohl(inet_addr(addr)), port);
    int ret = kRtpMng.sess.AddDestination(dest_addr);
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

static int RtpSendH264Packet(unsigned char* packet, int size) {
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
        kRtpMng.sess.SendPacket((const void *)packet, size, RTP_PAYLOAD_H264, true, ts);
    } else {
        int payload_len = RTP_MAX_PACKET_SIZE - 2;
        int offset = 1;

        unsigned char ualhdr = packet[0];
        unsigned char pkt[RTP_MAX_PACKET_SIZE] = {0};
        pkt[0] = (ualhdr & 0xe0) | 28;
        pkt[1] = (ualhdr & 0x1f) | 0x80;
        memcpy(pkt+2, packet+offset, RTP_MAX_PACKET_SIZE - 2);
        kRtpMng.sess.SendPacket((const void *)pkt, RTP_MAX_PACKET_SIZE, RTP_PAYLOAD_H264, false, 0);
        offset += (RTP_MAX_PACKET_SIZE - 2);

        pkt[1] = packet[4] & 0x1f;
        while(offset + (RTP_MAX_PACKET_SIZE - 2) < size) {
            memcmp(pkt+2, packet+offset, RTP_MAX_PACKET_SIZE - 2);
            kRtpMng.sess.SendPacket((const void *)pkt, RTP_MAX_PACKET_SIZE, RTP_PAYLOAD_H264, false, 0);
            offset += (RTP_MAX_PACKET_SIZE - 2);
        }

        pkt[1] = 0x40 | (packet[4] & 0x1F);
        memcmp(pkt+2, packet+offset, size - offset);
        kRtpMng.sess.SendPacket((const void *)pkt, size - offset, RTP_PAYLOAD_H264, true, ts);
    }

    return 0;
}

static int RtpTransH264(unsigned char* packet, int size) {
    int start = 0;
    while(start < size) {
        int len = 0;
        unsigned char* p = RtpFindNalu(packet + start, size - start, &len);
        if (p == NULL) {
            break;
        }

        RtpSendH264Packet(p, len);
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
    len += 13;
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

static int RtpSendPsPacket(unsigned char* packet, int size) {
    CHECK_BOOL(packet, return -1);
    CHECK_LE(size, 0, return -1);

    int offset = 0;
    unsigned char pkt[RTP_MAX_PACKET_SIZE] = {0};
    while(offset + RTP_MAX_PACKET_SIZE < size) {
        kRtpMng.sess.SendPacket((const void *)(packet + offset), RTP_MAX_PACKET_SIZE, RTP_PAYLOAD_PS, false, 0);
        offset += RTP_MAX_PACKET_SIZE;
    }

    kRtpMng.sess.SendPacket((const void *)(packet + offset), size - offset, RTP_PAYLOAD_PS, true, 90000/25);

    return 0;
}

static int RtpTransPs(unsigned char* packet, int size) {
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
    RtpGetPsHeader(ps_frame_buff, frame_type, ts, len);
    memcpy(ps_frame_buff + RTP_PS_HEAD_LEN, packet, len);
    RtpSendPsPacket(ps_frame_buff, len);
    offset += len;

    while (offset < size) {
        len = size - offset > RTP_MAX_PES_SIZE ? RTP_MAX_PES_SIZE : size - offset; 

        memset(ps_frame_buff, 0, RTP_MAX_PS_SIZE);
        RtpGetSinglePesHeader(ps_frame_buff, 0, len);
        memcpy(ps_frame_buff + RTP_PES_HEAD_LEN, packet + offset, len);
        RtpSendPsPacket(ps_frame_buff, len);
        offset += len;
    }
 
    ts += 3600;
    return 0;
}

int RtpPush(unsigned char* packet, int size) {
    CHECK_BOOL(packet, return -1);
    CHECK_LE(size, 0, return -1);

    if (!kRtpMng.init) {
        return 0;
    }

    // RtpTransH264(packet, size);

    RtpTransPs(packet, size);

    return 0;
}