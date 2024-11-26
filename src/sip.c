#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "sip.h"
#include "rtp.h"
#include "check_common.h"
#include "gb28181_commom.h"

#include "libmxml4/mxml.h"
#include "eXosip2/eXosip.h"

#define SIP_DOMAIN      "3402000000"
#define SIP_USER        "34020000002000000002"
#define SIP_PASSWORD    "12345678"

typedef struct {
    int reg_id;
    struct eXosip_t *excontext;
    int play_flag;
}SipMng;
static SipMng kSipMng;

static void SipCallProc(int tid, osip_message_t* req) {
    osip_message_t* resp = NULL;
    eXosip_call_build_answer(kSipMng.excontext, tid, 200, &resp);

    osip_body_t* body = NULL;
    osip_message_get_body(req, 0, &body);

    sdp_message_t* rem_sdp = NULL;
    sdp_message_init(&rem_sdp);
    sdp_message_parse(rem_sdp, body->body);

    const char* rem_addr = sdp_message_o_addr_get(rem_sdp);
    int rem_port = atoi(sdp_message_m_port_get(rem_sdp, 0));
    RtpInit(rem_addr, rem_port, atoi(rem_sdp->y_ssrc));

    char buff[1024] = {0};
    snprintf(buff, sizeof(buff), "v=0\r\n" \
                                "o=%s 0 0 IN IP4 %s\r\n" \
                                "s=gb28181_test\r\n" \
                                "c=IN IP4 %s\r\n" \
                                "t=0 0\r\n" \
                                "m=video %d RTP/AVP %d\r\n" \
                                "a=sendonly\r\n" \
                                "a=rtpmap:%d H264/90000\r\n" \
                                "y=%s\r\n",
                                SIP_USER, "192.168.110.223", "192.168.110.223", 4000, RTP_PAYLOAD_PS, RTP_PAYLOAD_PS, rem_sdp->y_ssrc);
    LOG_DEBUG("\n%s", buff);

    osip_message_set_header(resp, "Content-Type", "application/sdp");
    osip_message_set_body(resp, buff, strlen(buff));

    sdp_message_free(rem_sdp);
    
    eXosip_call_send_answer(kSipMng.excontext, tid, 200, resp);
}

static void SipMsgCatalog(bool space, const char* sn, const char* device_id, osip_message_t* resp) {
    mxml_options_t* options = mxmlOptionsNew();
    mxml_node_t *xml = mxmlNewXML("1.0");
    mxml_node_t *response = mxmlNewElement(xml, "Response");
    mxmlNewText(mxmlNewElement(response, "CmdType"), space, "Catalog");
    mxmlNewText(mxmlNewElement(response, "SN"), space, sn);
    mxmlNewText(mxmlNewElement(response, "DeviceID"), space,  device_id);
    mxmlNewText(mxmlNewElement(response, "SumNum"), space, "1");

    mxml_node_t *device_list = mxmlNewElement(response, "DeviceList");
    mxmlElementSetAttr(device_list, "Num", "1");
    mxml_node_t *item = mxmlNewElement(device_list, "Item");
    mxmlNewText(mxmlNewElement(item, "DeviceID"), space, device_id);
    mxmlNewText(mxmlNewElement(item, "Name"), space, "Camera");
    mxmlNewText(mxmlNewElement(item, "Manufacturer"), space, "IPC_Company");
    mxmlNewText(mxmlNewElement(item, "Model"), space, "HI351X");
    mxmlNewText(mxmlNewElement(item, "Owner"), space, "General");
    mxmlNewText(mxmlNewElement(item, "CivilCode"), space, SIP_DOMAIN);
    mxmlNewText(mxmlNewElement(item, "Block"), space, "General");
    mxmlNewText(mxmlNewElement(item, "Address"), space, "China");
    mxmlNewInteger(mxmlNewElement(item, "Parental"), 0);
    mxmlNewText(mxmlNewElement(item, "ParentID"), space, SIP_USER);
    mxmlNewText(mxmlNewElement(item, "SafetyWay"), space, "0");
    mxmlNewText(mxmlNewElement(item, "RegisterWay"), space, "1");
    mxmlNewText(mxmlNewElement(item, "Secrecy"), space, "0");
    mxmlNewText(mxmlNewElement(item, "IPAddress"), space, "192.168.110.223");
    mxmlNewText(mxmlNewElement(item, "Port"), space, "5060");
    mxmlNewText(mxmlNewElement(item, "Status"), space, "ON");
    mxmlNewText(mxmlNewElement(item, "Longitude"), space, "0.0");
    mxmlNewText(mxmlNewElement(item, "Latitude"), space, "0.0");

    mxml_node_t *info = mxmlNewElement(item, "Info");
    mxmlNewText(mxmlNewElement(info, "PTZType"), space, "3");
    mxmlNewText(mxmlNewElement(info, "Resolution"), space, "6/5/4/2");

    char* buff = mxmlSaveAllocString(xml, options);
    CHECK_POINTER(buff, goto end);

    osip_message_set_body(resp, buff, strlen(buff));

    free(buff);
end:
    mxmlDelete(xml);
    mxmlOptionsDelete(options);
}

static void SipMsgDeviceStatus(bool space, const char* sn, const char* device_id, osip_message_t* resp) {
    mxml_options_t* options = mxmlOptionsNew();
    mxml_node_t *xml = mxmlNewXML("1.0");
    mxml_node_t *response = mxmlNewElement(xml, "Response");
    mxmlNewText(mxmlNewElement(response, "CmdType"), space, "DeviceStatus");
    mxmlNewText(mxmlNewElement(response, "SN"), space, sn);
    mxmlNewText(mxmlNewElement(response, "DeviceID"), space, device_id);
    mxmlNewText(mxmlNewElement(response, "Result"), space, "OK");
    mxmlNewText(mxmlNewElement(response, "Online"), space, "ONLINE");
    mxmlNewText(mxmlNewElement(response, "Status"), space, "OK");
    mxmlNewText(mxmlNewElement(response, "Encode"), space, "ON");
    mxmlNewText(mxmlNewElement(response, "Record"), space, "OFF");

    time_t cur_time = time(NULL);
    struct tm* cur_tm = localtime(&cur_time);
    char device_time[20] = {0};
    snprintf(device_time, sizeof(device_time), "%04d-%02d-%02dT%02d:%02d:%02d", 
        cur_tm->tm_year+1900, cur_tm->tm_mon+1, cur_tm->tm_mday, cur_tm->tm_hour, cur_tm->tm_min, cur_tm->tm_sec);
    mxmlNewText(mxmlNewElement(response, "DeviceTime"), space, device_time);

    char* buff = mxmlSaveAllocString(xml, options);
    
    osip_message_set_body(resp, buff, strlen(buff));

    free(buff);
    mxmlDelete(xml);
    mxmlOptionsDelete(options);
}

#define SIP_CMD_HEAD (0xa5)

#define SIP_CMD_SCAN_START (0x89)
#define SIP_CMD_SCAN_SPEED (0x8A)

#define SIP_CMD_AUXILIARY_CTRL_ON (0x8C)
#define SIP_CMD_AUXILIARY_CTRL_OFF (0x8D)

static void SipMsgPtzCmd(const char* arg, osip_message_t* resp) {
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

static void SipMsgProc(int tid, osip_message_t* req) {
    osip_message_t* resp = NULL;
    int ret = eXosip_message_build_answer(kSipMng.excontext, tid, 200, &resp);
    CHECK_POINTER(resp, return);

    osip_body_t* body = NULL;
    osip_message_get_body(req, 0, &body);
    LOG_DEBUG("body:%s", body->body);
    
    bool space = 0;
    mxml_options_t* options = mxmlOptionsNew();     
    mxml_node_t* tree = mxmlLoadString(NULL, options, body->body);

    if (mxmlFindElement(tree, tree, "Query", NULL, NULL, MXML_DESCEND_ALL) != NULL) {
        mxml_node_t* cmd_type = mxmlFindElement(tree, tree, "CmdType", NULL, NULL, MXML_DESCEND_ALL);
        mxml_node_t* sn = mxmlFindElement(tree, tree, "SN", NULL, NULL, MXML_DESCEND_ALL);
        mxml_node_t* device_id = mxmlFindElement(tree, tree, "DeviceID", NULL, NULL, MXML_DESCEND_ALL);
        
        if (strcmp("Catalog", mxmlGetText(cmd_type, &space)) == 0) {
            SipMsgCatalog(space, mxmlGetText(sn, &space), mxmlGetText(device_id, &space), resp);
        } else if (strcmp("DeviceStatus", mxmlGetText(cmd_type, &space)) == 0) {
            SipMsgDeviceStatus(space, mxmlGetText(sn, &space), mxmlGetText(device_id, &space), resp);
        } else {
            LOG_ERR("not support query cmd!");
            goto end;
        }
        
        eXosip_message_send_answer(kSipMng.excontext, tid, 200, resp);
    } else if (mxmlFindElement(tree, tree, "Control", NULL, NULL, MXML_DESCEND_ALL) != NULL) {
        mxml_node_t* cmd_type = mxmlFindElement(tree, tree, "CmdType", NULL, NULL, MXML_DESCEND_ALL);
        mxml_node_t* sn = mxmlFindElement(tree, tree, "SN", NULL, NULL, MXML_DESCEND_ALL);
        mxml_node_t* device_id = mxmlFindElement(tree, tree, "DeviceID", NULL, NULL, MXML_DESCEND_ALL);

        if (strcmp("DeviceControl", mxmlGetText(cmd_type, &space)) == 0) {
            mxml_node_t* ptz_cmd = mxmlFindElement(tree, tree, "PTZCmd", NULL, NULL, MXML_DESCEND_ALL);
            SipMsgPtzCmd(mxmlGetText(ptz_cmd, &space), resp);
        } else {
            LOG_ERR("not support query cmd!");
        }
    }

end:
    mxmlDelete(tree);
    mxmlOptionsDelete(options);
}

static void* proc(void* arg) {
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
        case EXOSIP_REGISTRATION_FAILURE:
            break;
        case EXOSIP_CALL_INVITE:
            if (MSG_IS_INVITE(event->request)){
                SipCallProc(event->tid, event->request);
            }
            break;
        case EXOSIP_CALL_ACK:
            if (MSG_IS_ACK(event->ack)) {
                kSipMng.play_flag = 1;
            }
            break;
        case EXOSIP_CALL_CLOSED:
            if (MSG_IS_BYE(event->request)) {
                kSipMng.play_flag = 0;
                RtpUnInit();
            }
            break;
        case EXOSIP_MESSAGE_NEW:
            if (MSG_IS_MESSAGE(event->request)) {
                SipMsgProc(event->tid, event->request);
            }
            break;
        default:
            break;
        }

        eXosip_unlock(kSipMng.excontext);
        eXosip_event_free(event);
    }
}

int SipInit() {
    kSipMng.excontext = eXosip_malloc();

    int ret = eXosip_init(kSipMng.excontext);
    CHECK_EQ(ret, 0, goto end);

    ret = eXosip_listen_addr(kSipMng.excontext, IPPROTO_UDP, NULL, 5060, AF_INET, 0);
    CHECK_EQ(ret, 0, goto end);

    eXosip_set_user_agent(kSipMng.excontext, "HbsGBSIP-1.0");

    pthread_t pthread_id;
    pthread_create(&pthread_id, NULL, proc, NULL);

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

    eXosip_register_remove(kSipMng.excontext, kSipMng.reg_id);
        
    eXosip_clear_authentication_info(kSipMng.excontext);
}

int SipPushStream(void* buff, int size) {
    CHECK_POINTER(buff, return -1);
    CHECK_LE(size, 0, return -1);

    if (kSipMng.play_flag) {
        RtpPush((unsigned char*)buff, size);
    }

    return 0;
}