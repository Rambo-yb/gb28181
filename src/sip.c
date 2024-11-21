#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "sip.h"
#include "rtp.h"
#include "check_common.h"

#include "libmxml4/mxml.h"
#include "eXosip2/eXosip.h"

#define SIP_DOMAIN      "3402000000"
#define SIP_USER        "34020000002000000002"
#define SIP_PASSWORD    "12345678"

typedef struct {
    struct eXosip_t *excontext;
    int play_flag;
}SipMng;
static SipMng kSipMng;

char* SipSdpValSet(const char* val) {
    CHECK_BOOL(val, return NULL);

    char* str = osip_malloc(strlen(val)+1);
    strcpy(str, val);
    return str;
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
                osip_body_t* body = NULL;
                osip_message_get_body(event->request, 0, &body);

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
                LOG_INFO("\n%s", buff);

                osip_message_t* answer = NULL;
                eXosip_call_build_answer(kSipMng.excontext, event->tid, 200, &answer);
                osip_message_set_header(answer, "Content-Type", "application/sdp");
                osip_message_set_body(answer, buff, strlen(buff));
                eXosip_call_send_answer(kSipMng.excontext, event->tid, 200, answer);

                sdp_message_free(rem_sdp);
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
                osip_body_t* body = NULL;
                osip_message_get_body(event->request, 0, &body);
                LOG_INFO("body:%s", body->body);

                osip_message_t* answer = NULL;
                eXosip_message_build_answer(kSipMng.excontext, event->tid, 200, &answer);
                
                mxml_options_t* options = mxmlOptionsNew();     
                mxml_node_t* tree = mxmlLoadString(NULL, options, body->body);
                if (mxmlFindElement(tree, tree, "Query", NULL, NULL, MXML_DESCEND_ALL) != NULL) {
                    mxml_node_t* cmd_type = mxmlFindElement(tree, tree, "CmdType", NULL, NULL, MXML_DESCEND_ALL);
                    mxml_node_t* sn = mxmlFindElement(tree, tree, "SN", NULL, NULL, MXML_DESCEND_ALL);
                    mxml_node_t* device_id = mxmlFindElement(tree, tree, "DeviceID", NULL, NULL, MXML_DESCEND_ALL);
                    
                    bool space = 0;
                    if (strcmp("Catalog", mxmlGetText(cmd_type, &space)) == 0) {
                        mxml_node_t *xml = mxmlNewXML("1.0");
                        mxml_node_t *response = mxmlNewElement(xml, "Response");
                        mxmlNewText(mxmlNewElement(response, "CmdType"), space, "Catalog");
                        mxmlNewText(mxmlNewElement(response, "SN"), space, mxmlGetText(sn, &space));
                        mxmlNewText(mxmlNewElement(response, "DeviceID"), space,  mxmlGetText(device_id, &space));
                        mxmlNewText(mxmlNewElement(response, "SumNum"), space, "1");

                        mxml_node_t *device_list = mxmlNewElement(response, "DeviceList");
                        mxmlElementSetAttr(device_list, "Num", "1");
                        mxml_node_t *item = mxmlNewElement(device_list, "Item");
                        mxmlNewText(mxmlNewElement(item, "DeviceID"), space, mxmlGetText(device_id, &space));
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
                        LOG_INFO("buff:%s", buff);
                        
                        osip_message_set_body(answer, buff, strlen(buff));

                        free(buff);
                        mxmlDelete(xml);
                    } else if (strcmp("DeviceStatus", mxmlGetText(cmd_type, &space)) == 0) {
                        mxml_node_t *xml = mxmlNewXML("1.0");
                        mxml_node_t *response = mxmlNewElement(xml, "Response");
                        mxmlNewText(mxmlNewElement(response, "CmdType"), space, "DeviceStatus");
                        mxmlNewText(mxmlNewElement(response, "SN"), space, mxmlGetText(sn, &space));
                        mxmlNewText(mxmlNewElement(response, "DeviceID"), space,  mxmlGetText(device_id, &space));
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
                        LOG_INFO("buff:%s", buff);
                        
                        osip_message_set_body(answer, buff, strlen(buff));

                        free(buff);
                        mxmlDelete(xml);
                    }
                }
                mxmlDelete(tree);
                mxmlOptionsDelete(options);

                eXosip_message_send_answer(kSipMng.excontext, event->tid, 200, answer);
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

    ret = eXosip_add_authentication_info(kSipMng.excontext, SIP_USER, SIP_USER, SIP_PASSWORD, NULL, NULL);
    CHECK_EQ(ret, 0, goto end);

    osip_message_t* reg = NULL;
    eXosip_lock(kSipMng.excontext);
    char proxy[256] = {0};
    snprintf(proxy, sizeof(proxy), "sip:192.168.110.168:15060");
    char form[256] = {0};
    snprintf(form, sizeof(form), "sip:%s@%s", SIP_USER, SIP_DOMAIN);
    int reg_id = eXosip_register_build_initial_register(kSipMng.excontext, form, proxy, form, 7200, &reg);
    CHECK_LT(reg_id, 1, eXosip_unlock(kSipMng.excontext);goto end);
    ret = eXosip_register_send_register(kSipMng.excontext, reg_id, reg);
    eXosip_unlock(kSipMng.excontext);
    CHECK_EQ(ret, 0, goto end);

    pthread_t pthread_id;
    pthread_create(&pthread_id, NULL, proc, NULL);

    // RtpInit(4000);

    return 0;

end:
    eXosip_quit(kSipMng.excontext);
    osip_free(kSipMng.excontext);
    return -1;
}

void SipUnInit() {

}

int SipPushStream(void* buff, int size) {
    CHECK_POINTER(buff, return -1);
    CHECK_LE(size, 0, return -1);

    if (kSipMng.play_flag) {
        RtpPush((unsigned char*)buff, size);
    }

    return 0;
}