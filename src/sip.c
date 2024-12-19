#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "sip.h"
#include "rtp.h"
#include "check_common.h"
#include "gb28181_commom.h"
#include "list.h"

#include "libmxml4/mxml.h"
#include "eXosip2/eXosip.h"

#define SIP_DOMAIN      "4101050000"
#define SIP_USER        "41010500002000000002"
#define SIP_PASSWORD    "12345678"

typedef struct {
    char method[8];
	char* body;
}SipMsgSendInfo;

typedef struct {
    int cid;
    int did;
    int flag;
    void* rtp_hander;
}SipRtpInfo;

typedef struct {
    int reg_id;
    struct eXosip_t *excontext;
    int reg_flag;
    int sn;

    SipRtpInfo rtp_play;
    SipRtpInfo rtp_playback;
    
    void* list_header;
    pthread_t pthread_recv;
    pthread_t pthread_send;
    pthread_t pthread_keeplive;
    pthread_mutex_t mutex;
}SipMng;
static SipMng kSipMng = {.mutex = PTHREAD_MUTEX_INITIALIZER};
extern int kRecordFlag;

static void SipCallProc(eXosip_event_t * event) {
    osip_message_t* resp = NULL;
    eXosip_call_build_answer(kSipMng.excontext, event->tid, 200, &resp);

    osip_body_t* body = NULL;
    osip_message_get_body(event->request, 0, &body);
    LOG_DEBUG("body:%s", body->body);

    sdp_message_t* rem_sdp = NULL;
    sdp_message_init(&rem_sdp);
    sdp_message_parse(rem_sdp, body->body);

    const char* rem_addr = sdp_message_o_addr_get(rem_sdp);
    int rem_port = atoi(sdp_message_m_port_get(rem_sdp, 0));

    SipRtpInfo* rtp_info = strcmp(rem_sdp->s_name, "Playback") == 0 || strcmp(rem_sdp->s_name, "Download") == 0 ? &kSipMng.rtp_playback : &kSipMng.rtp_play;
    rtp_info->cid = event->cid;
    rtp_info->did = event->did;
    rtp_info->rtp_hander = RtpInit(rem_addr, rem_port, atoi(rem_sdp->y_ssrc));
    CHECK_POINTER(rtp_info->rtp_hander, return );

    char* start_time = sdp_message_t_start_time_get(rem_sdp, 0);
    char* stop_time = sdp_message_t_stop_time_get(rem_sdp, 0);

	char u[128] = {0};
	if(rem_sdp->u_uri != NULL) {
		snprintf(u, sizeof(u), "u=%s\r\n", rem_sdp->u_uri);
	}

    char buff[1024] = {0};
    snprintf(buff, sizeof(buff),
        "v=0\r\n" \
        "o=%s 0 0 IN IP4 %s\r\n" \
        "s=%s\r\n" \
		"%s" \
        "c=IN IP4 %s\r\n" \
        "t=%s %s\r\n" \
        "m=video %d RTP/AVP %d\r\n" \
        "a=sendonly\r\n" \
        "a=rtpmap:%d PS/90000\r\n" \
        "y=%s\r\n",
        SIP_USER, "192.168.110.223", rem_sdp->s_name, u, "192.168.110.223", start_time, stop_time, 
        RtpTransportPort(rtp_info->rtp_hander), RTP_PAYLOAD_PS, RTP_PAYLOAD_PS, rem_sdp->y_ssrc);
    LOG_DEBUG("\n%s", buff);

    osip_message_set_header(resp, "Content-Type", "application/sdp");
    osip_message_set_body(resp, buff, strlen(buff));

    sdp_message_free(rem_sdp);
    
    eXosip_call_send_answer(kSipMng.excontext, event->tid, 200, resp);
}

static void SipCallMsgProc(eXosip_event_t * event) {
	osip_body_t* body = NULL;
	osip_message_get_body(event->request, 0, &body);
	LOG_DEBUG("body:%s", body->body);

	if (strncmp(body->body, "PAUSE", strlen("PAUSE")) == 0) {
		// todo 暂停
		LOG_INFO("PAUSE");
	} else {
		if (strstr(body->body, "Range") != NULL) {
			// todo 开始
			LOG_INFO("PLAY");
		} else if (strstr(body->body, "Scale") != NULL) {
			float scale = 0.0;
			char* p = strstr(body->body, "Scale");
			sscanf(p, "Scale: %f", &scale);

			LOG_INFO("scale:%f", scale);
		}
	}
}

static void SipMsgCatalog(mxml_node_t* tree, char** resp) {
	bool space = false;
    mxml_node_t* sn = mxmlFindElement(tree, tree, "SN", NULL, NULL, MXML_DESCEND_ALL);
    mxml_node_t* device_id = mxmlFindElement(tree, tree, "DeviceID", NULL, NULL, MXML_DESCEND_ALL);

    mxml_options_t* options = mxmlOptionsNew();
    mxml_node_t *xml = mxmlNewXML("1.0");
    mxml_node_t *response = mxmlNewElement(xml, "Response");
    mxmlNewText(mxmlNewElement(response, "CmdType"), space, "Catalog");
    mxmlNewText(mxmlNewElement(response, "SN"), space, mxmlGetText(sn, &space));
    mxmlNewText(mxmlNewElement(response, "DeviceID"), space, mxmlGetText(device_id, &space));
    mxmlNewText(mxmlNewElement(response, "SumNum"), space, "1");

    mxml_node_t *device_list = mxmlNewElement(response, "DeviceList");
    mxmlElementSetAttr(device_list, "Num", "1");
    mxml_node_t *item = mxmlNewElement(device_list, "Item");
    mxmlNewText(mxmlNewElement(item, "DeviceID"), space, SIP_USER);
    mxmlNewText(mxmlNewElement(item, "Name"), space, "IPC_Company");
    mxmlNewText(mxmlNewElement(item, "Manufacturer"), space, "CDJP");
    mxmlNewText(mxmlNewElement(item, "Model"), space, "rv1126");
    mxmlNewText(mxmlNewElement(item, "Owner"), space, "Owner");
    mxmlNewText(mxmlNewElement(item, "CivilCode"), space, "4101050000");
    mxmlNewText(mxmlNewElement(item, "Address"), space, "Address");
    mxmlNewInteger(mxmlNewElement(item, "Parental"), 0);
    mxmlNewText(mxmlNewElement(item, "ParentID"), space, "41010500002000000001");
    mxmlNewText(mxmlNewElement(item, "RegisterWay"), space, "1");
    mxmlNewText(mxmlNewElement(item, "Secrecy"), space, "0");
    mxmlNewText(mxmlNewElement(item, "Status"), space, "ON");

    *resp = mxmlSaveAllocString(xml, options);
	
    mxmlDelete(xml);
    mxmlOptionsDelete(options);
}

static void SipMsgDeviceStatus(mxml_node_t* tree, char** resp) {
	bool space = false;
    mxml_node_t* sn = mxmlFindElement(tree, tree, "SN", NULL, NULL, MXML_DESCEND_ALL);
    mxml_node_t* device_id = mxmlFindElement(tree, tree, "DeviceID", NULL, NULL, MXML_DESCEND_ALL);

    mxml_options_t* options = mxmlOptionsNew();
    mxml_node_t *xml = mxmlNewXML("1.0");
    mxml_node_t *response = mxmlNewElement(xml, "Response");
    mxmlNewText(mxmlNewElement(response, "CmdType"), space, "DeviceStatus");
    mxmlNewText(mxmlNewElement(response, "SN"), space, mxmlGetText(sn, &space));
    mxmlNewText(mxmlNewElement(response, "DeviceID"), space, mxmlGetText(device_id, &space));
    mxmlNewText(mxmlNewElement(response, "Result"), space, "OK");
    mxmlNewText(mxmlNewElement(response, "Online"), space, "ONLINE");
    mxmlNewText(mxmlNewElement(response, "Status"), space, "OK");
    mxmlNewText(mxmlNewElement(response, "Encode"), space, "ON");
    mxmlNewText(mxmlNewElement(response, "Record"), space, "ON");

    time_t cur_time = time(NULL);
    struct tm* cur_tm = localtime(&cur_time);
    char device_time[20] = {0};
    snprintf(device_time, sizeof(device_time), "%04d-%02d-%02dT%02d:%02d:%02d", 
        cur_tm->tm_year+1900, cur_tm->tm_mon+1, cur_tm->tm_mday, cur_tm->tm_hour, cur_tm->tm_min, cur_tm->tm_sec);
    mxmlNewText(mxmlNewElement(response, "DeviceTime"), space, device_time);

    *resp = mxmlSaveAllocString(xml, options);

    mxmlDelete(xml);
    mxmlOptionsDelete(options);
}

static void SipMsgDeviceInfo(mxml_node_t* tree, char** resp) {
	bool space = false;
    mxml_node_t* sn = mxmlFindElement(tree, tree, "SN", NULL, NULL, MXML_DESCEND_ALL);
    mxml_node_t* device_id = mxmlFindElement(tree, tree, "DeviceID", NULL, NULL, MXML_DESCEND_ALL);

    mxml_options_t* options = mxmlOptionsNew();
    mxml_node_t *xml = mxmlNewXML("1.0");
    mxml_node_t *response = mxmlNewElement(xml, "Response");
    mxmlNewText(mxmlNewElement(response, "CmdType"), space, "DeviceInfo");
    mxmlNewText(mxmlNewElement(response, "SN"), space, mxmlGetText(sn, &space));
    mxmlNewText(mxmlNewElement(response, "DeviceID"), space, mxmlGetText(device_id, &space));
    mxmlNewText(mxmlNewElement(response, "Result"), space, "OK");
    mxmlNewText(mxmlNewElement(response, "DeviceName"), space, "IPC_Company");
    mxmlNewText(mxmlNewElement(response, "Manufacturer"), space, "CDJP");
    mxmlNewText(mxmlNewElement(response, "Model"), space, "rv1126");
    mxmlNewText(mxmlNewElement(response, "Firmware"), space, "V1.0.0");
    mxmlNewText(mxmlNewElement(response, "Channel"), space, "1");

    *resp = mxmlSaveAllocString(xml, options);

    mxmlDelete(xml);
    mxmlOptionsDelete(options);
}

static void SipMsgRecordInfo(mxml_node_t* tree, char** resp) {
	bool space = false;
    mxml_node_t* sn = mxmlFindElement(tree, tree, "SN", NULL, NULL, MXML_DESCEND_ALL);
    mxml_node_t* device_id = mxmlFindElement(tree, tree, "DeviceID", NULL, NULL, MXML_DESCEND_ALL);

    mxml_options_t* options = mxmlOptionsNew();
    mxml_node_t *xml = mxmlNewXML("1.0");
    mxml_node_t *response = mxmlNewElement(xml, "Response");
    mxmlNewText(mxmlNewElement(response, "CmdType"), space, "RecordInfo");
    mxmlNewText(mxmlNewElement(response, "SN"), space, mxmlGetText(sn, &space));
    mxmlNewText(mxmlNewElement(response, "DeviceID"), space, mxmlGetText(device_id, &space));
    mxmlNewText(mxmlNewElement(response, "Name"), space, "Camera");
    mxmlNewInteger(mxmlNewElement(response, "SumNum"), 2);

    mxml_node_t *record_list = mxmlNewElement(response, "RecordList");
    mxmlElementSetAttr(record_list, "Num", "2");
    mxml_node_t *item = mxmlNewElement(record_list, "Item");
    mxmlNewText(mxmlNewElement(item, "DeviceID"), space, mxmlGetText(device_id, &space));
    mxmlNewText(mxmlNewElement(item, "Name"), space, "Camera");
    mxmlNewText(mxmlNewElement(item, "FilePath"), space, "/data/media");
    mxmlNewText(mxmlNewElement(item, "Address"), space, "address_1");
    mxmlNewText(mxmlNewElement(item, "StartTime"), space, "2024-11-26T14:13:00");
    mxmlNewText(mxmlNewElement(item, "EndTime"), space, "2024-11-26T14:14:00");
    mxmlNewText(mxmlNewElement(item, "Secrecy"), space, mxmlGetText(mxmlFindElement(tree, tree, "Secrecy", NULL, NULL, MXML_DESCEND_ALL), &space));
    mxmlNewText(mxmlNewElement(item, "Type"), space, "time");
    mxmlNewText(mxmlNewElement(item, "RecorderID"), space, mxmlGetText(mxmlFindElement(tree, tree, "RecorderID", NULL, NULL, MXML_DESCEND_ALL), &space));

    mxml_node_t *item_1 = mxmlNewElement(record_list, "Item");
    mxmlNewText(mxmlNewElement(item_1, "DeviceID"), space, mxmlGetText(device_id, &space));
    mxmlNewText(mxmlNewElement(item_1, "Name"), space, "Camera");
    mxmlNewText(mxmlNewElement(item_1, "FilePath"), space, mxmlGetText(mxmlFindElement(tree, tree, "FilePath", NULL, NULL, MXML_DESCEND_ALL), &space));
    mxmlNewText(mxmlNewElement(item_1, "Address"), space, mxmlGetText(mxmlFindElement(tree, tree, "Address", NULL, NULL, MXML_DESCEND_ALL), &space));
    mxmlNewText(mxmlNewElement(item_1, "StartTime"), space, "2024-11-26T14:14:00");
    mxmlNewText(mxmlNewElement(item_1, "EndTime"), space, "2024-11-26T14:15:00");
    mxmlNewText(mxmlNewElement(item_1, "Secrecy"), space, mxmlGetText(mxmlFindElement(tree, tree, "Secrecy", NULL, NULL, MXML_DESCEND_ALL), &space));
    mxmlNewText(mxmlNewElement(item_1, "Type"), space, "time");
    mxmlNewText(mxmlNewElement(item_1, "RecorderID"), space, mxmlGetText(mxmlFindElement(tree, tree, "RecorderID", NULL, NULL, MXML_DESCEND_ALL), &space));

    *resp = mxmlSaveAllocString(xml, options);
    mxmlDelete(xml);
    mxmlOptionsDelete(options);
}

#define SIP_CMD_HEAD (0xa5)

#define SIP_CMD_SCAN_START (0x89)
#define SIP_CMD_SCAN_SPEED (0x8A)

#define SIP_CMD_AUXILIARY_CTRL_ON (0x8C)
#define SIP_CMD_AUXILIARY_CTRL_OFF (0x8D)

static void SipMsgPtzCmd(mxml_node_t* tree, char** resp) {
	bool space = false;
    mxml_node_t* ptz_cmd = mxmlFindElement(tree, tree, "PTZCmd", NULL, NULL, MXML_DESCEND_ALL);
    const char* arg = mxmlGetText(ptz_cmd, &space);

    unsigned int cmd[8] = {0};
    sscanf(arg, "%02x%02x%02x%02x%02x%02x%02x%02x", 
        &cmd[0], &cmd[1], &cmd[2], &cmd[3], &cmd[4], &cmd[5], &cmd[6], &cmd[7]);
    CHECK_EQ(cmd[0], SIP_CMD_HEAD, return);

    if (cmd[3] & (0x01 << 6)) {
        int focus_type = GB28181_FOCUS_CTRL_STOP;
        if (cmd[3] & (0x01 << 0)) {
            focus_type = GB28181_FOCUS_CTRL_FAR;
        } else if (cmd[3] & (0x01 << 1)) {
            focus_type = GB28181_FOCUS_CTRL_NEAR;
        }

        int iris_type = GB28181_IRIS_CTRL_STOP;
        if (cmd[3] & (0x01 << 2)) {
            iris_type = GB28181_IRIS_CTRL_IN;
        } else if (cmd[3] & (0x01 << 3)) {
            iris_type = GB28181_IRIS_CTRL_OUT;
        }
        LOG_INFO("focus_type:%d iris_type:%d", focus_type, iris_type);
    } else if (cmd[3] & (0x01 << 7)) {
        if (cmd[3] == SIP_CMD_AUXILIARY_CTRL_ON || cmd[3] == SIP_CMD_AUXILIARY_CTRL_OFF) {
            // todo
        } else if (cmd[3] == SIP_CMD_SCAN_START || cmd[3] == SIP_CMD_SCAN_SPEED) {
            int ptz_type = (cmd[3] == SIP_CMD_SCAN_START) ? GB28181_PTZ_CTRL_SCAN_START: GB28181_PTZ_CTRL_SCAN_SPEED;
            if (ptz_type == GB28181_PTZ_CTRL_SCAN_START) {
                ptz_type += cmd[5];
            }
            int scan_num = cmd[5];
            int data = (cmd[5] & 0xff) | ((cmd[6] & 0xf0) << 8);
        } else {
            int ptz_type = GB28181_PTZ_CTRL_PRESET_ADD + (cmd[3] & 0x0f) - 1;
            int preset_num = cmd[5];
            int cruise_num = cmd[4];
            int data = (cmd[5] & 0xff) | ((cmd[6] & 0xf0) << 8);

            LOG_INFO("ptz_type:%d preset:%d cruise:%d data:%d", ptz_type, preset_num, cruise_num, data);
        }
    } else {
        int ptz_type = GB28181_PTZ_CTRL_STOP;
        if(cmd[3] & (0x01 << 0)) {
            if (cmd[3] & (0x01 << 2)) {
                ptz_type = GB28181_PTZ_CTRL_RIGHT_DOWN;
            } else if (cmd[3] & (0x01 << 3)){
                ptz_type = GB28181_PTZ_CTRL_RIGHT_UP;
            } else {
                ptz_type = GB28181_PTZ_CTRL_RIGHT;
            }
        } else if (cmd[3] & (0x01 << 1)) {
            if (cmd[3] & (0x01 << 2)) {
                ptz_type = GB28181_PTZ_CTRL_LEFT_DOWN;
            } else if (cmd[3] & (0x01 << 3)){
                ptz_type = GB28181_PTZ_CTRL_LEFT_UP;
            } else {
                ptz_type = GB28181_PTZ_CTRL_LEFT;
            }
        } else if (cmd[3] & (0x01 << 2)) {
            ptz_type = GB28181_PTZ_CTRL_DOWN;
        } else if (cmd[3] & (0x01 << 3)) {
            ptz_type = GB28181_PTZ_CTRL_UP;
        }
        
        int zoom_type = GB28181_ZOOM_CTRL_STOP;
        if (cmd[3] & (0x01) << 4) {
            zoom_type = GB28181_ZOOM_CTRL_IN;
        } else if (cmd[3] & (0x01) << 5) {
            zoom_type = GB28181_ZOOM_CTRL_OUT;
        }

        LOG_INFO("ptz_type:%d zoom_type:%d", ptz_type, zoom_type);
    }
}

static void SipMsgRecordCmd(mxml_node_t* tree, char** resp) {
	bool space = false;
	mxml_node_t* cmd = mxmlFindElement(tree, tree, "RecordCmd", NULL, NULL, MXML_DESCEND_ALL);
    const char* arg = mxmlGetText(cmd, &space);

	if (strcmp(arg, "Record") == 0) {
		LOG_WRN("TODO: Record");
	} else if (strcmp(arg, "StopRecord") == 0) {
		LOG_WRN("TODO: StopRecord");
	}

	mxml_node_t* sn = mxmlFindElement(tree, tree, "SN", NULL, NULL, MXML_DESCEND_ALL);
    mxml_node_t* device_id = mxmlFindElement(tree, tree, "DeviceID", NULL, NULL, MXML_DESCEND_ALL);

    mxml_options_t* options = mxmlOptionsNew();
    mxml_node_t *xml = mxmlNewXML("1.0");
    mxml_node_t *response = mxmlNewElement(xml, "Response");
    mxmlNewText(mxmlNewElement(response, "CmdType"), space, "DeviceControl");
    mxmlNewText(mxmlNewElement(response, "SN"), space, mxmlGetText(sn, &space));
    mxmlNewText(mxmlNewElement(response, "DeviceID"), space, mxmlGetText(device_id, &space));
    mxmlNewText(mxmlNewElement(response, "Result"), space, "OK");

    *resp = mxmlSaveAllocString(xml, options);

    mxmlDelete(xml);
    mxmlOptionsDelete(options);
}

static void SipMsgGuardCmd(mxml_node_t* tree, char** resp) {
	bool space = false;
	mxml_node_t* cmd = mxmlFindElement(tree, tree, "GuardCmd", NULL, NULL, MXML_DESCEND_ALL);
    const char* arg = mxmlGetText(cmd, &space);

	if (strcmp(arg, "SetGuard") == 0) {
		LOG_WRN("TODO: SetGuard");
	} else if (strcmp(arg, "ResetGuard") == 0) {
		LOG_WRN("TODO: ResetGuard");
	}

	mxml_node_t* sn = mxmlFindElement(tree, tree, "SN", NULL, NULL, MXML_DESCEND_ALL);
    mxml_node_t* device_id = mxmlFindElement(tree, tree, "DeviceID", NULL, NULL, MXML_DESCEND_ALL);

    mxml_options_t* options = mxmlOptionsNew();
    mxml_node_t *xml = mxmlNewXML("1.0");
    mxml_node_t *response = mxmlNewElement(xml, "Response");
    mxmlNewText(mxmlNewElement(response, "CmdType"), space, "DeviceControl");
    mxmlNewText(mxmlNewElement(response, "SN"), space, mxmlGetText(sn, &space));
    mxmlNewText(mxmlNewElement(response, "DeviceID"), space, mxmlGetText(device_id, &space));
    mxmlNewText(mxmlNewElement(response, "Result"), space, "OK");

    *resp = mxmlSaveAllocString(xml, options);

    mxmlDelete(xml);
    mxmlOptionsDelete(options);
}

typedef void(*SipMsgSubCb)(mxml_node_t*, char**);

typedef struct {
    const char* msg_type;
    const char* cmd_type;
	const char* ctrl_type;
    bool resp_flag;
    SipMsgSubCb cb;
}SipMsgProcInfo;

static SipMsgProcInfo kProcInfo[] = {
    {.msg_type = "Query", .cmd_type = "Catalog", .resp_flag = true, .cb = SipMsgCatalog},
    {.msg_type = "Query", .cmd_type = "DeviceStatus", .resp_flag = true, .cb = SipMsgDeviceStatus},
    {.msg_type = "Query", .cmd_type = "DeviceInfo", .resp_flag = true, .cb = SipMsgDeviceInfo},
    {.msg_type = "Query", .cmd_type = "RecordInfo", .resp_flag = true, .cb = SipMsgRecordInfo},

    {.msg_type = "Control", .cmd_type = "DeviceControl", .ctrl_type = "PTZCmd", .resp_flag = false, .cb = SipMsgPtzCmd},
    {.msg_type = "Control", .cmd_type = "DeviceControl", .ctrl_type = "RecordCmd", .resp_flag = true, .cb = SipMsgRecordCmd},
    {.msg_type = "Control", .cmd_type = "DeviceControl", .ctrl_type = "GuardCmd", .resp_flag = true, .cb = SipMsgGuardCmd},
};

static void SipMsgProc(eXosip_event_t * event) {
    osip_body_t* body = NULL;
    osip_message_get_body(event->request, 0, &body);
    LOG_DEBUG("body:%s", body->body);
    
    bool space = false;
    mxml_options_t* options = mxmlOptionsNew();     
    mxml_node_t* tree = mxmlLoadString(NULL, options, body->body);
    mxml_node_t* cmd_type = mxmlFindElement(tree, tree, "CmdType", NULL, NULL, MXML_DESCEND_ALL);

    for (int i = 0; i < sizeof(kProcInfo) / sizeof(SipMsgProcInfo); i++) {
        if (mxmlFindElement(tree, tree, kProcInfo[i].msg_type, NULL, NULL, MXML_DESCEND_ALL) != NULL 
            && strcmp(kProcInfo[i].cmd_type, mxmlGetText(cmd_type, &space)) == 0 
			&& (kProcInfo[i].ctrl_type == NULL || mxmlFindElement(tree, tree, kProcInfo[i].ctrl_type, NULL, NULL, MXML_DESCEND_ALL) != NULL)) {
            if (kProcInfo[i].resp_flag) {
                SipMsgSendInfo info;
                snprintf(info.method, sizeof(info.method), "MESSAGE");
				kProcInfo[i].cb(tree, &info.body);

                pthread_mutex_lock(&kSipMng.mutex);
                ListPush(kSipMng.list_header, &info, sizeof(SipMsgSendInfo));
                pthread_mutex_unlock(&kSipMng.mutex);

				eXosip_message_send_answer(kSipMng.excontext, event->tid, 200, NULL);
            } else {
                kProcInfo[i].cb(tree, NULL);
            }

            break;
        }
    }

end:
    mxmlDelete(tree);
    mxmlOptionsDelete(options);
}

static void* SipRecvProc(void* arg) {
    while(1) {
        eXosip_event_t *event = eXosip_event_wait(kSipMng.excontext, 0, 10);

        eXosip_lock(kSipMng.excontext);
        eXosip_automatic_action(kSipMng.excontext);
        eXosip_unlock(kSipMng.excontext);

        if (event == NULL) {
            usleep(10*1000);
            continue;
        }

        eXosip_lock(kSipMng.excontext);
        LOG_INFO("cid:%d, did:%d, type:%d", event->cid, event->did, event->type);
        switch (event->type)
        {
        case EXOSIP_REGISTRATION_SUCCESS:
            kSipMng.reg_flag = true;
            break;
        case EXOSIP_REGISTRATION_FAILURE:
            break;
        case EXOSIP_CALL_INVITE:
            if (MSG_IS_INVITE(event->request)){
                SipCallProc(event);
            }
            break;
        case EXOSIP_CALL_ACK:
            if (MSG_IS_ACK(event->ack)) {
                if (event->cid == kSipMng.rtp_play.cid && event->did == kSipMng.rtp_play.did) {
                    kSipMng.rtp_play.flag = true;
                } else if (event->cid == kSipMng.rtp_playback.cid && event->did == kSipMng.rtp_playback.did) {
					kRecordFlag = true;
                    kSipMng.rtp_playback.flag = true;
                }
            }
            break;
        case EXOSIP_CALL_MESSAGE_NEW:
            if (MSG_IS_INFO(event->request)) {
				SipCallMsgProc(event);
            }
            break;
        case EXOSIP_CALL_CLOSED:
            if (MSG_IS_BYE(event->request)) {
				if (event->cid == kSipMng.rtp_play.cid && event->did == kSipMng.rtp_play.did) {
					RtpUnInit(kSipMng.rtp_play.rtp_hander);
					memset(&kSipMng.rtp_play, 0, sizeof(SipRtpInfo));
				} else if (event->cid == kSipMng.rtp_playback.cid && event->did == kSipMng.rtp_playback.did) {
					RtpUnInit(kSipMng.rtp_playback.rtp_hander);
					memset(&kSipMng.rtp_playback, 0, sizeof(SipRtpInfo));
					kRecordFlag = false;
				}
            }
            break;
        case EXOSIP_MESSAGE_NEW:
            if (MSG_IS_MESSAGE(event->request)) {
                SipMsgProc(event);
            }
            break;
        default:
            break;
        }

        eXosip_unlock(kSipMng.excontext);
        eXosip_event_free(event);
    }
}

static void* SipSendProc(void* arg) {

    while (1) {
        SipMsgSendInfo info;
        pthread_mutex_lock(&kSipMng.mutex);
        if (ListSize(kSipMng.list_header) <= 0) {
            pthread_mutex_unlock(&kSipMng.mutex);
            usleep(1000);
            continue;
        }

        if (ListPop(kSipMng.list_header, &info, sizeof(SipMsgSendInfo)) < 0) {
            pthread_mutex_unlock(&kSipMng.mutex);
            usleep(1000);
            continue;
        }
        pthread_mutex_unlock(&kSipMng.mutex);

		osip_message_t* msg = NULL;
		char to[256] = {0};
		snprintf(to, sizeof(to), "sip:192.168.110.124:15060");

		char from[256] = {0};
		snprintf(from, sizeof(from), "sip:%s@%s", SIP_USER, SIP_DOMAIN);

		eXosip_message_build_request(kSipMng.excontext, &msg, info.method, to, from, NULL);
		osip_message_set_header(msg, "Content-Type", "Application/MANSCDP+xml");
		osip_message_set_body(msg, info.body, strlen(info.body));
		LOG_DEBUG("%s", info.body);
		eXosip_message_send_request(kSipMng.excontext, msg);
		free(info.body);

        usleep(1000);
    }
    
}

static void* SipKeepliveProc(void* arg) {
	int keeplive_time = 60;
	int last_time = 0;
	while(1) {
        if (!kSipMng.reg_flag) {
			usleep(1000*1000);
			continue;
        }

		int cur_time = time(NULL);
		if (last_time + keeplive_time > cur_time) {
			usleep(1000*1000);
			continue;
		}

		int space = 0;
		mxml_options_t* options = mxmlOptionsNew();
		mxml_node_t *xml = mxmlNewXML("1.0");
		mxml_node_t *response = mxmlNewElement(xml, "Notify");
		mxmlNewText(mxmlNewElement(response, "CmdType"), space, "Keepalive");
		mxmlNewInteger(mxmlNewElement(response, "SN"), kSipMng.sn++);
		mxmlNewText(mxmlNewElement(response, "DeviceID"), space, SIP_USER);
		mxmlNewText(mxmlNewElement(response, "Result"), space, "OK");

		SipMsgSendInfo info;
        snprintf(info.method, sizeof(info.method), "MESSAGE");
		info.body = mxmlSaveAllocString(xml, options);

		pthread_mutex_lock(&kSipMng.mutex);
		ListPush(kSipMng.list_header, &info, sizeof(SipMsgSendInfo));
		pthread_mutex_unlock(&kSipMng.mutex);

		last_time = cur_time;
		mxmlDelete(xml);
		mxmlOptionsDelete(options);
	}
}

int SipInit() {
    kSipMng.excontext = eXosip_malloc();

    int ret = eXosip_init(kSipMng.excontext);
    CHECK_EQ(ret, 0, goto end);

    ret = eXosip_listen_addr(kSipMng.excontext, IPPROTO_UDP, NULL, 5060, AF_INET, 0);
    CHECK_EQ(ret, 0, goto end);

    eXosip_set_user_agent(kSipMng.excontext, "HbsGBSIP-1.0");

    kSipMng.list_header = ListCreate();
    CHECK_POINTER(kSipMng.list_header, goto end);

    pthread_create(&kSipMng.pthread_recv, NULL, SipRecvProc, NULL);
    pthread_create(&kSipMng.pthread_send, NULL, SipSendProc, NULL);
    pthread_create(&kSipMng.pthread_keeplive, NULL, SipKeepliveProc, NULL);

    return 0;

end:
    eXosip_quit(kSipMng.excontext);
    osip_free(kSipMng.excontext);
    return -1;
}

void SipUnInit() {
    if (kSipMng.excontext != NULL) {
        eXosip_quit(kSipMng.excontext);
        osip_free(kSipMng.excontext);
    }
}

int SipRegister(const char* rem_addr, int rem_port, const char* user, const char* password, const char* domain) {
    CHECK_POINTER(rem_addr, return -1);
    CHECK_POINTER(user, return -1);
    CHECK_POINTER(password, return -1);
    CHECK_POINTER(domain, return -1);
    CHECK_POINTER(kSipMng.excontext, return -1);
    CHECK_BOOL(!kSipMng.reg_flag, return -1);

    int ret = eXosip_add_authentication_info(kSipMng.excontext, user, user, password, NULL, NULL);
    CHECK_EQ(ret, 0, return -1);

    osip_message_t* reg = NULL;
    eXosip_lock(kSipMng.excontext);
    char proxy[256] = {0};
    snprintf(proxy, sizeof(proxy), "sip:%s:%d", rem_addr, rem_port);
    char form[256] = {0};
    snprintf(form, sizeof(form), "sip:%s@%s", user, domain);
    kSipMng.reg_id = eXosip_register_build_initial_register(kSipMng.excontext, form, proxy, form, 7200, &reg);
    CHECK_LT(kSipMng.reg_id, 1, eXosip_unlock(kSipMng.excontext);return -1);
    ret = eXosip_register_send_register(kSipMng.excontext, kSipMng.reg_id, reg);
    eXosip_unlock(kSipMng.excontext);
    CHECK_EQ(ret, 0, return -1);

    return 0;
}

void SipUnRegister() {
    CHECK_POINTER(kSipMng.excontext, return );
    CHECK_BOOL(kSipMng.reg_flag, return );

    kSipMng.reg_flag = false;

    eXosip_register_remove(kSipMng.excontext, kSipMng.reg_id);
        
    eXosip_clear_authentication_info(kSipMng.excontext);
}

int SipPushStream(void* buff, int size) {
    CHECK_POINTER(buff, return -1);
    CHECK_LE(size, 0, return -1);

    if (kSipMng.rtp_play.rtp_hander != NULL && kSipMng.rtp_play.flag) {
        RtpPush(kSipMng.rtp_play.rtp_hander, (unsigned char*)buff, size);
    }

    return 0;
}

int SipPushRecordStream(void* buff, int size, int end) {
    CHECK_POINTER(buff, return -1);
    CHECK_LE(size, 0, return -1);

    if (kSipMng.rtp_playback.rtp_hander != NULL && kSipMng.rtp_playback.flag) {
        RtpPush(kSipMng.rtp_playback.rtp_hander, (unsigned char*)buff, size);
    }

	if(end) {
		int space = 0;
		mxml_options_t* options = mxmlOptionsNew();
		mxml_node_t *xml = mxmlNewXML("1.0");
		mxml_node_t *response = mxmlNewElement(xml, "Notify");
		mxmlNewText(mxmlNewElement(response, "CmdType"), space, "MediaStatus");
		mxmlNewInteger(mxmlNewElement(response, "SN"), kSipMng.sn++);
		mxmlNewText(mxmlNewElement(response, "DeviceID"), space, SIP_USER);
		mxmlNewInteger(mxmlNewElement(response, "NotifyType"), 121);

		SipMsgSendInfo info;
        snprintf(info.method, sizeof(info.method), "MESSAGE");
		info.body = mxmlSaveAllocString(xml, options);

		pthread_mutex_lock(&kSipMng.mutex);
		ListPush(kSipMng.list_header, &info, sizeof(SipMsgSendInfo));
		pthread_mutex_unlock(&kSipMng.mutex);

		mxmlDelete(xml);
		mxmlOptionsDelete(options);
	}

    return 0;
}