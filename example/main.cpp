
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#include <list>

#include "gb28181.h"
#include "gb28181_common.h"

typedef struct {
    int size;
    unsigned char* data;
}Frame;

typedef struct {
	unsigned char* media_data;
	std::list<Frame> media_list;

	int play_stream;

	int play_record;
	int oper_record;

}Mng;
static Mng kMng;

long GetTime() {
    struct timeval time_;
    memset(&time_, 0, sizeof(struct timeval));

    gettimeofday(&time_, NULL);
    return time_.tv_sec*1000 + time_.tv_usec/1000;
}

#ifdef CHIP_TYPE_RV1126

#include "rkmedia_api.h"
#include "rkmedia_venc.h"

void StreamPush(MEDIA_BUFFER mb) {
	if (kMng.play_stream) {
    	Gb28181PushStream(GB28181_PUSH_REAL_TIME_STREAM, (unsigned char*)RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb), 0); 
	}

	RK_MPI_MB_ReleaseBuffer(mb);
}

int StreamInit() {
    int ret = 0;
    const char* device_name = "/dev/video0";
    int cam_id = 0;
    int width = 640;
    int height = 512;
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
	ret = RK_MPI_SYS_RegisterOutCb(&stEncChn, StreamPush);
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

#endif

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

static void* RecordPush(void* arg) {
    while(1) {
		if (!kMng.play_record) {
			usleep(10*1000);
			continue;
		}

		int i = 0;
        for(Frame frame : kMng.media_list) {
            Gb28181PushStream(GB28181_PUSH_RECORD_STREAM, frame.data, frame.size, (++i == kMng.media_list.size()) ? true : false);
			if (!kMng.play_record) {
				break;
			}
            usleep(40*1000);
        }

		kMng.play_record = false;
    }
}

static int RecordInit() {
    FILE* fp = fopen("recordfile01.h264", "r");
    if (fp == NULL) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    kMng.media_data = (unsigned char*)malloc(size+1);
    if (kMng.media_data == NULL) {
        fclose(fp);
        return -1;
    }
    memset(kMng.media_data, 0, size+1);
    int ret = fread(kMng.media_data, 1, size, fp);
    if (ret != size) {
        printf("fread fail ,size:%d-%d\n", ret, size);
        fclose(fp);
        free(kMng.media_data);
        return -1;
    }

    fclose(fp);

    int start = 0;
    while(start < size) {
        Frame frame;
        frame.data = FindFrame(kMng.media_data+start, size-start, &frame.size);
        if (frame.data == NULL) {
            break;
        }

        kMng.media_list.push_back(frame);
        start = frame.data - kMng.media_data + frame.size;
    }

    printf("media size:%d len:%d\n", kMng.media_list.size(), size);

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, RecordPush, NULL);
    return 0;
}

static int ContorlProc(int type, void* st, int size) {
	switch (type) {
	case GB28181_CONTORL_PLAY_STREAM:
		if (((Gb28181PlayStream*)st)->type == GB28181_PLAY_VIDEO_START) {
			kMng.play_stream = true;
		} else {
			kMng.play_stream = false;
		}
		break;
	case GB28181_CONTORL_PLAY_RECORD:
		if (((Gb28181PlayRecord*)st)->type == GB28181_PLAY_VIDEO_START) {
			kMng.play_record = true;
		} else if (((Gb28181PlayRecord*)st)->type == GB28181_PLAY_VIDEO_STOP) {
			kMng.play_record = false;
		}
		break;
	case GB28181_CONTORL_QUERY_RECORD:{
		Gb28181QueryRecordInfo* _st = (Gb28181QueryRecordInfo*)st;
		_st->num = 2;
		_st->info = (Gb28181RecordInfo*)malloc(sizeof(Gb28181RecordInfo)*_st->num);
		if (_st->info == NULL) {
			return -1;
		}

		_st->info[0].start_time = 1735539547;
		_st->info[0].end_time = 1735539607;
		snprintf(_st->info[0].name, sizeof(_st->info[0].name), "1735539547-1735539607.mp4");
		snprintf(_st->info[0].path, sizeof(_st->info[0].path), "/data/media");
		
		_st->info[1].start_time = 1735539607;
		_st->info[1].end_time = 1735539667;
		snprintf(_st->info[1].name, sizeof(_st->info[1].name), "1735539607-1735539667.mp4");
		snprintf(_st->info[1].path, sizeof(_st->info[1].path), "/data/media");

		break;
	}
	case GB28181_CONTORL_PTZ:{
		Gb28181PtzCtrl* _st = (Gb28181PtzCtrl*)st;
		printf("ptz type:%d, preset:%d, grop:%d, speed:%d, time:%d\n", 
			_st->type, _st->preset_num, _st->grop_num, _st->speed, _st->time);
		break;
	}
	case GB28181_CONTORL_ZOOM:{
		Gb28181ZoomCtrl* _st = (Gb28181ZoomCtrl*)st;
		printf("zoom type:%d\n", _st->type);
		break;
	}
	case GB28181_CONTORL_IRIS:{
		Gb28181IrisCtrl* _st = (Gb28181IrisCtrl*)st;
		printf("iris type:%d, speed:%d\n", _st->type, _st->speed);
		break;
	}
	case GB28181_CONTORL_FOCUS:{
		Gb28181FocusCtrl* _st = (Gb28181FocusCtrl*)st;
		printf("focus type:%d, speed:%d\n", _st->type, _st->speed);
		break;
	}
	case GB28181_CONTORL_RECORDING:
		break;
	default:
		break;
	}
	return 0;
}

int main(int argc, char *argv[])
{
    StreamInit();

    RecordInit();

    Gb28181Init();

	Gb28181OperationRegister(GB28181_OPERATION_CONTROL, (void*)ContorlProc);

	Gb28181Info info;
	snprintf(info.sip_id, sizeof(info.sip_id), "41010500002000000001");
	snprintf(info.sip_domain, sizeof(info.sip_domain), "4101050000");
	snprintf(info.sip_addr, sizeof(info.sip_addr), "192.168.110.124");
	info.sip_port = 15060;
	snprintf(info.sip_username, sizeof(info.sip_username), "41010500002000000002");
	snprintf(info.sip_password, sizeof(info.sip_password), "12345678");
	info.register_inretval = 2*60*60;
	info.keeplive_interval = 60;
	info.max_timeout = 3;
	info.protocol_type = 0;
	info.register_inretval = 60;
    Gb28181Register(&info);

    while (1){
        sleep(1);
    }
    
    Gb28181Unregister(&info);
    Gb28181UnInit();

    return 0;
}
