
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include <list>

#include "sip.h"
#include "log.h"

#ifdef CHIP_TYPE_RV1126

#include "rkmedia_api.h"
#include "rkmedia_venc.h"

void video_packet_cb(MEDIA_BUFFER mb) {
    SipPushStream(RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb));  

	RK_MPI_MB_ReleaseBuffer(mb);
}

int MediaInit() {
    int ret = 0;
    const char* device_name = "/dev/video0";
    int32_t cam_id = 0;
    uint32_t width = 640;
    uint32_t height = 512;
    CODEC_TYPE_E codec_type = RK_CODEC_TYPE_H264;

    RK_MPI_SYS_Init();
    VI_CHN_ATTR_S vi_chn_attr;
    vi_chn_attr.pcVideoNode = device_name;
    vi_chn_attr.u32BufCnt = 3;
    vi_chn_attr.u32Width = width;
    vi_chn_attr.u32Height = height;
    vi_chn_attr.enPixFmt = IMAGE_TYPE_NV16;
    vi_chn_attr.enBufType = VI_CHN_BUF_TYPE_MMAP;
    vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
    ret = RK_MPI_VI_SetChnAttr(cam_id, 0, &vi_chn_attr);
    ret |= RK_MPI_VI_EnableChn(cam_id, 0);
    if (ret) {
        printf("ERROR: create VI[0] error! ret=%d\n", ret);
        return -1;
    }

    VENC_CHN_ATTR_S venc_chn_attr;
    memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
    switch (codec_type) {
    case RK_CODEC_TYPE_H265:
        venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H265;
        venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
        venc_chn_attr.stRcAttr.stH265Cbr.u32Gop = 30;
        venc_chn_attr.stRcAttr.stH265Cbr.u32BitRate = width * height;
        // frame rate: in 30/1, out 30/1.
        venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateDen = 1;
        venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateNum = 30;
        venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateDen = 1;
        venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateNum = 30;
        break;
    case RK_CODEC_TYPE_H264:
    default:
        venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
        venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
        venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 30;
        venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = width * height;
        // frame rate: in 30/1, out 30/1.
        venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
        venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = 30;
        venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
        venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = 30;
        break;
    }
    venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_NV16;
    venc_chn_attr.stVencAttr.u32PicWidth = width;
    venc_chn_attr.stVencAttr.u32PicHeight = height;
    venc_chn_attr.stVencAttr.u32VirWidth = width;
    venc_chn_attr.stVencAttr.u32VirHeight = height;
    venc_chn_attr.stVencAttr.u32Profile = 77;
    ret = RK_MPI_VENC_CreateChn(0, &venc_chn_attr);
    if (ret) {
        printf("ERROR: create VENC[0] error! ret=%d\n", ret);
        return -1;
    }

	MPP_CHN_S stEncChn;
	stEncChn.enModId = RK_ID_VENC;
	stEncChn.s32DevId = 0;
	stEncChn.s32ChnId = 0;
	ret = RK_MPI_SYS_RegisterOutCb(&stEncChn, video_packet_cb);
	if (ret) {
		printf("ERROR: register output callback for VENC[0] error! ret=%d\n", ret);
		return 0;
	}

    MPP_CHN_S src_chn;
    src_chn.enModId = RK_ID_VI;
    src_chn.s32DevId = 0;
    src_chn.s32ChnId = 0;
    MPP_CHN_S dest_chn;
    dest_chn.enModId = RK_ID_VENC;
    dest_chn.s32DevId = 0;
    dest_chn.s32ChnId = 0;
    ret = RK_MPI_SYS_Bind(&src_chn, &dest_chn);
    if (ret) {
        printf("ERROR: Bind VI[0] and VENC[0] error! ret=%d\n", ret);
        return -1;
    }

    printf("%s initial finish\n", __FUNCTION__);
    return 0;
}

#else

typedef struct {
    int size;
    unsigned char* data;
}Frame;

std::list<Frame> kMediaList;
unsigned char* kMediaData;

static unsigned char* FindFrame(const unsigned char* buff, int len, int* size) {
	unsigned char *s = NULL;
	while (len >= 3) {
		if (buff[0] == 0 && buff[1] == 0 && buff[2] == 1 && (buff[3] == 0x41 || buff[3] == 0x67)) {
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
		if (len >= 4 && buff[0] == 0 && buff[1] == 0 && buff[2] == 0 && buff[3] == 1 && (buff[4] == 0x41 || buff[4] == 0x67)) {
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

void* VideoPush(void* arg) {
    while(1) {
        for(Frame frame : kMediaList) {
            SipPushStream(frame.data, frame.size);

            usleep(40*1000);
        }
    }
}

static int MediaInit() {
    FILE* fp = fopen("recordfile01.h264", "r");
    if (fp == NULL) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    kMediaData = (unsigned char*)malloc(size+1);
    if (kMediaData == NULL) {
        fclose(fp);
        return -1;
    }
    memset(kMediaData, 0, size+1);
    int ret = fread(kMediaData, 1, size, fp);
    if (ret != size) {
        printf("fread fail ,size:%d-%d\n", ret, size);
        fclose(fp);
        free(kMediaData);
        return -1;
    }

    fclose(fp);

    int start = 0;
    while(start < size) {
        Frame frame;
        frame.data = FindFrame(kMediaData+start, size-start, &frame.size);
        if (frame.data == NULL) {
            break;
        }

        kMediaList.push_back(frame);
        start = frame.data - kMediaData + frame.size;
    }

    printf("media size:%d len:%d\n", kMediaList.size(), size);

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, VideoPush, NULL);
    return 0;
}

#endif

int main(int argc, char *argv[])
{
    log_init((char*)"./gb28181.log", 512*1024, 3, 5);
    MediaInit();
    SipInit();

    

    while (1){
        sleep(1);
    }

    // pthread_cancel(thread_id);
    // free(kMediaData);
    SipUnInit();

    return 0;
}
