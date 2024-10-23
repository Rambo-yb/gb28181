#include <pthread.h>
#include "sip.h"
#include "check_common.h"

#include "pjsua-lib/pjsua.h"
#include "pjsua-lib/pjsua_internal.h"
#include "libmxml4/mxml.h"

#define SIP_DOMAIN      "3402000000"
#define SIP_USER        "34020000002000000002"
#define SIP_PASSWD      "12345678"

typedef struct {
    pj_timer_entry          timer;
    pj_bool_t               ringback_on;
    pj_bool_t               ring_on;
}CallData;


typedef struct {
    pjmedia_vid_stream_info stream_info;
    pjmedia_rtp_session out_session;
    pjmedia_rtp_session in_session;
    pjmedia_rtcp_session rtcp_session;
} SipStreamInfo;

typedef struct {
    pj_pool_t *pool;
    pjsua_config cfg;
    pjsua_logging_config log_cfg;
    pjsua_media_config media_cfg;
    pjsua_transport_config  udp_cfg;
    pjsua_transport_config  tcp_cfg;
    pjsua_transport_config  rtp_cfg;
    CallData call_data[PJSUA_MAX_CALLS];

    pjsua_acc_id acc_id;
    pjsua_acc_config acc_cfg;
    pjsua_call_setting  call_opt;

    pj_thread_t* stream_thread_id;
    bool stream_thread_quit;
    pjmedia_port* port;
    pthread_mutex_t stream_mutex;
    SipStreamFrame stream_frame;

    SipStreamInfo sip_stream_info;
}SipMng;
static SipMng kSipMng = {.stream_mutex = PTHREAD_MUTEX_INITIALIZER};

static pj_status_t SipVideoCapture(pjmedia_vid_dev_stream *stream, void *user_data, pjmedia_frame *frame) {
    LOG_WRN("");
    return PJ_SUCCESS;
}

static void SipOnCallState(pjsua_call_id call_id, pjsip_event *e) {
    pjsua_call_info call_info;
    pjsua_call_get_info(call_id, &call_info);

    LOG_INFO("call id:%d state:%d", call_id, call_info.state);

    pjsua_call* call = NULL;
    pjsip_dialog* dlg = NULL;
    acquire_call(NULL, call_id, &call, &dlg);
    LOG_INFO("cap_win_id:%d rdr_win_id:%d", call->media[0].strm.v.cap_dev, call->media[0].strm.v.rdr_dev);

    if (call_info.state == PJSIP_INV_STATE_CONFIRMED) {
    //     pjmedia_vid_stream_resume(call->media[0].strm.v.stream, call->media[0].dir);
    //     pj_status_t status = pjmedia_vid_stream_get_port(call->media[0].strm.v.stream, call->media[0].dir, &kSipMng.port);
    //     CHECK_EQ(status, PJ_SUCCESS, return);
        pjsua_call_set_vid_strm(call_id, PJSUA_CALL_VID_STRM_START_TRANSMIT, NULL);
        
        // pjsua_vid_win* win = &data->win[call->media[0].strm.v.cap_dev];
        // LOG_INFO("cap:%p rand:%p", win->vp_cap, win->vp_rend);
        // pjmedia_vid_dev_cb vid_dev_cb = {&SipVideoCapture, NULL};
        // pjmedia_vid_port_set_cb(win->vp_cap, &vid_dev_cb, NULL);
    } else if (call_info.state == PJSIP_INV_STATE_DISCONNECTED) {
    //     pjmedia_vid_stream_pause(call->media[0].strm.v.stream, call->media[0].dir);
    //     kSipMng.port = NULL;
    }

    return;
}

static void SipOnStreamDestroyed(pjsua_call_id call_id, pjmedia_stream *strm, unsigned stream_idx) {
    LOG_WRN("");
    return;
}

static void on_rx_rtp(void *user_data, void *pkt, pj_ssize_t size)
{
    // struct media_stream *strm;
    // pj_status_t status;
    // const pjmedia_rtp_hdr *hdr;
    // const void *payload;
    // unsigned payload_len;

    // strm = (struct media_stream *)user_data;

    // /* Discard packet if media is inactive */
    // if (!strm->active)
    //     return;

    // /* Check for errors */
    // if (size < 0) {
    //     app_perror(THIS_FILE, "RTP recv() error", (pj_status_t)-size);
    //     return;
    // }

    // /* Decode RTP packet. */
    // status = pjmedia_rtp_decode_rtp(&strm->in_sess, 
    //                                 pkt, (int)size, 
    //                                 &hdr, &payload, &payload_len);
    // if (status != PJ_SUCCESS) {
    //     app_perror(THIS_FILE, "RTP decode error", status);
    //     return;
    // }

    // //PJ_LOG(4,(THIS_FILE, "Rx seq=%d", pj_ntohs(hdr->seq)));

    // /* Update the RTCP session. */
    // pjmedia_rtcp_rx_rtp(&strm->rtcp, pj_ntohs(hdr->seq),
    //                     pj_ntohl(hdr->ts), payload_len);

    // /* Update RTP session */
    // pjmedia_rtp_session_update(&strm->in_sess, hdr, NULL);

}

static void on_rx_rtcp(void *user_data, void *pkt, pj_ssize_t size)
{
    // struct media_stream *strm;

    // strm = (struct media_stream *)user_data;

    // /* Discard packet if media is inactive */
    // if (!strm->active)
    //     return;

    // /* Check for errors */
    // if (size < 0) {
    //     app_perror(THIS_FILE, "Error receiving RTCP packet",(pj_status_t)-size);
    //     return;
    // }

    // /* Update RTCP session */
    // pjmedia_rtcp_rx_rtcp(&strm->rtcp, pkt, size);
}

static void SipOnCallMediaState(pjsua_call_id call_id) {
    LOG_WRN("");
}

static void SipOnIncomingCall(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata) {
    LOG_WRN("call:%d",call_id);
    pjsua_call_info call_info;
    pjsua_call_get_info(call_id, &call_info);

    pjsip_rdata_sdp_info* sdp_info = pjsip_rdata_get_sdp_info(rdata);

    pj_pool_t* tmp_pool = pjsua_pool_create("gb28181-sdp", 1000, 1000);

    pjmedia_sdp_session* t_sdp = pjmedia_sdp_session_clone(tmp_pool, sdp_info->sdp);
    CHECK_POINTER(t_sdp, pj_pool_release(tmp_pool);return);

    for (int i = 0; i < t_sdp->media_count; i++) {
        pjmedia_sdp_attr_remove_all(&t_sdp->media[i]->attr_count, t_sdp->media[i]->attr, "rtpmap");
        pjmedia_sdp_attr_remove_all(&t_sdp->media[i]->attr_count, t_sdp->media[i]->attr, "recvonly");

        pj_str_t fmt = pj_str("96");
        t_sdp->media[i]->desc.fmt_count = 1;
        pj_strdup(tmp_pool, &t_sdp->media[i]->desc.fmt[0], &fmt);

        pjmedia_sdp_attr_add(&t_sdp->media[i]->attr_count, t_sdp->media[i]->attr, pjmedia_sdp_attr_create(tmp_pool, "sendonly", NULL));

        pj_str_t h264_val = pj_str("96 H264/90000");
        pjmedia_sdp_attr_add(&t_sdp->media[i]->attr_count, t_sdp->media[i]->attr, pjmedia_sdp_attr_create(tmp_pool, "rtpmap", &h264_val));
    }


    char buff[1024] = {0};
    pjmedia_sdp_print(t_sdp, buff, sizeof(buff));
    LOG_INFO("\n%s", buff);

    pjsua_call_answer_with_sdp(call_id, t_sdp, NULL, 200, NULL, NULL);

    pj_pool_release(tmp_pool);
    return;
}

static void SipCallOnDtmfCallback2(pjsua_call_id call_id, const pjsua_dtmf_info *info) {
    LOG_WRN("");
    return;
}

static pjsip_redirect_op SipCallOnRedirected(pjsua_call_id call_id, const pjsip_uri *target, const pjsip_event *e) {
    LOG_WRN("");
    return 0;
}

static void SipOnRegState(pjsua_acc_id acc_id) {
    LOG_WRN("");
    return;
}

static void SipOnIncomingSubscribe(pjsua_acc_id acc_id, pjsua_srv_pres *srv_pres, pjsua_buddy_id buddy_id, const pj_str_t *from,
                                  pjsip_rx_data *rdata, pjsip_status_code *code, pj_str_t *reason, pjsua_msg_data *msg_data_) {
    LOG_WRN("");
    return;
}

static void SipOnBuddyState(pjsua_buddy_id buddy_id) {
    LOG_WRN("");
    return;
}

static void SipOnBuddyEvsubState(pjsua_buddy_id buddy_id, pjsip_evsub *sub, pjsip_event *event) {
    LOG_WRN("");
    return;
}

static void SipOnPager(pjsua_call_id call_id, const pj_str_t *from, const pj_str_t *to, const pj_str_t *contact, const pj_str_t *mime_type, const pj_str_t *body) {
    LOG_INFO("from:%.*s", (int)from->slen, from->ptr);
    LOG_INFO("to:%.*s", (int)to->slen, to->ptr);
    LOG_INFO("contact:%.*s", (int)contact->slen, contact->ptr);
    LOG_INFO("mime_type:%.*s", (int)mime_type->slen, mime_type->ptr);
    LOG_INFO("body:%.*s", (int)body->slen, body->ptr);

    mxml_options_t* options = mxmlOptionsNew();
    mxml_node_t* tree = mxmlLoadString(NULL, options, body->ptr);
    mxml_node_t* cmd_type = mxmlFindElement(tree, tree, "CmdType", NULL, NULL, MXML_DESCEND_ALL);
    mxml_node_t* sn = mxmlFindElement(tree, tree, "SN", NULL, NULL, MXML_DESCEND_ALL);
    bool space = 0;
    if (strcmp("Catalog", mxmlGetText(cmd_type, &space)) == 0) {
        mxml_node_t *xml = mxmlNewXML("1.0");
        mxml_node_t *response = mxmlNewElement(xml, "Response");
        mxmlNewText(mxmlNewElement(response, "CmdType"), space, "Catalog");
        mxmlNewText(mxmlNewElement(response, "SN"), space, mxmlGetText(sn, &space));
        mxmlNewText(mxmlNewElement(response, "DeviceID"), space, SIP_USER);
        mxmlNewText(mxmlNewElement(response, "SumNum"), space, "1");

        mxml_node_t *device_list = mxmlNewElement(response, "DeviceList");
        mxmlElementSetAttr(device_list, "Num", "1");
        mxml_node_t *item = mxmlNewElement(device_list, "Item");
        mxmlNewText(mxmlNewElement(item, "DeviceID"), space, SIP_USER);
        mxmlNewText(mxmlNewElement(item, "Name"), space, "Camera");
        mxmlNewText(mxmlNewElement(item, "Manufacturer"), space, "IPC_Company");
        mxmlNewText(mxmlNewElement(item, "Model"), space, "HI351X");
        mxmlNewText(mxmlNewElement(item, "Owner"), space, "General");
        mxmlNewText(mxmlNewElement(item, "CivilCode"), space, "3402000000");
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

        pj_str_t send_buf = pj_str(buff);

        pjsua_im_send(kSipMng.acc_id, to, mime_type, &send_buf, NULL, NULL);
        free(buff);
        mxmlDelete(xml);
    }
    mxmlDelete(tree);
    mxmlOptionsDelete(options);
}

static void SipOnTyping(pjsua_call_id call_id, const pj_str_t *from, const pj_str_t *to, const pj_str_t *contact, pj_bool_t is_typing) {
    LOG_WRN("");
    return;
}

static void SipOnCallTransferStatus(pjsua_call_id call_id, int status_code, const pj_str_t *status_text, pj_bool_t final, pj_bool_t *p_cont) {
    LOG_WRN("");
    return;
}

static void SipOnCallReplaced(pjsua_call_id old_call_id, pjsua_call_id new_call_id) {
    LOG_WRN("");
    return;
}

static void SipOnNatDetect(const pj_stun_nat_detect_result *res) {
    LOG_WRN("");
    return;
}

static void SipOnMwiInfo(pjsua_acc_id acc_id, pjsua_mwi_info *mwi_info) {
    LOG_WRN("");
    return;
}

static void SipOnTransportState(pjsip_transport *tp, pjsip_transport_state state, const pjsip_transport_state_info *info) {
    LOG_WRN("");
    return;
}

static void SipOnIceTransportError(int index, pj_ice_strans_op op, pj_status_t status, void *param) {
    LOG_WRN("");
    return;
}

static pj_status_t SipOnSndDevOperation(int operation) {
    LOG_WRN("");
    return 0;
}

static void SipOnCallMediaEvent(pjsua_call_id call_id, unsigned med_idx, pjmedia_event *event) {
    LOG_WRN("");
    return;
}

static void SipOnIpChangeProgress(pjsua_ip_change_op op, pj_status_t status, const pjsua_ip_change_op_info *info) {
    LOG_WRN("");
    return;
}

static pjmedia_transport* SipOnCreateMediaTransport(pjsua_call_id call_id, unsigned media_idx, pjmedia_transport *base_tp, unsigned flags) {
    LOG_WRN("");
    pjmedia_transport *adapter;
    pj_status_t status = pjmedia_tp_adapter_create(pjsua_get_pjmedia_endpt(), NULL, base_tp, (flags & PJSUA_MED_TP_CLOSE_MEMBER), &adapter);
    CHECK_EQ(status, PJ_SUCCESS, return NULL);

    return adapter;
}

static void SipSimpleRegistrar(pjsip_rx_data *rdata)
{
    pjsip_tx_data *tdata;
    const pjsip_expires_hdr *exp;
    const pjsip_hdr *h;
    pjsip_generic_string_hdr *srv;
    pj_status_t status;

    status = pjsip_endpt_create_response(pjsua_get_pjsip_endpt(), rdata, 200, NULL, &tdata);
    if (status != PJ_SUCCESS)
    return;

    exp = (pjsip_expires_hdr *)pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_EXPIRES, NULL);

    h = rdata->msg_info.msg->hdr.next;
    while (h != &rdata->msg_info.msg->hdr) {
        if (h->type == PJSIP_H_CONTACT) {
            const pjsip_contact_hdr *c = (const pjsip_contact_hdr*)h;
            unsigned e = c->expires;

            if (e != PJSIP_EXPIRES_NOT_SPECIFIED) {
                if (exp)
                    e = exp->ivalue;
                else
                    e = 3600;
            }

            if (e > 0) {
                pjsip_contact_hdr *nc = (pjsip_contact_hdr *)pjsip_hdr_clone(tdata->pool, h);
                nc->expires = e;
                pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr*)nc);
            }
        }
        h = h->next;
    }

    srv = pjsip_generic_string_hdr_create(tdata->pool, NULL, NULL);
    srv->name = pj_str("Server");
    srv->hvalue = pj_str("pjsua simple registrar");
    pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr*)srv);

    status = pjsip_endpt_send_response2(pjsua_get_pjsip_endpt(), rdata, tdata, NULL, NULL);
    if (status != PJ_SUCCESS) {
        pjsip_tx_data_dec_ref(tdata);
    }
}

static pj_bool_t SipOnRxRequest(pjsip_rx_data *rdata)
{
    pjsip_tx_data *tdata;
    pjsip_status_code status_code;
    pj_status_t status;

    /* Don't respond to ACK! */
    if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method, &pjsip_ack_method) == 0)
        return PJ_TRUE;

    /* Simple registrar */
    if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method, &pjsip_register_method) == 0)
    {
        SipSimpleRegistrar(rdata);
        return PJ_TRUE;
    }

    /* Create basic response. */
    if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method, &pjsip_notify_method) == 0)
    {
        /* Unsolicited NOTIFY's, send with Bad Request */
        status_code = PJSIP_SC_BAD_REQUEST;
    } else {
        /* Probably unknown method */
        status_code = PJSIP_SC_METHOD_NOT_ALLOWED;
    }
    status = pjsip_endpt_create_response(pjsua_get_pjsip_endpt(), rdata, status_code, NULL, &tdata);
    if (status != PJ_SUCCESS) {
        LOG_ERR("Unable to create response");
        return PJ_TRUE;
    }

    /* Add Allow if we're responding with 405 */
    if (status_code == PJSIP_SC_METHOD_NOT_ALLOWED) {
        const pjsip_hdr *cap_hdr;
        cap_hdr = pjsip_endpt_get_capability(pjsua_get_pjsip_endpt(), PJSIP_H_ALLOW, NULL);
        if (cap_hdr) {
            pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr *)pjsip_hdr_clone(tdata->pool, cap_hdr));
        }
    }

    /* Add User-Agent header */
    {
        pj_str_t user_agent;
        char tmp[80];
        const pj_str_t USER_AGENT = { "User-Agent", 10};
        pjsip_hdr *h;

        pj_ansi_snprintf(tmp, sizeof(tmp), "PJSUA v%s/%s", pj_get_version(), PJ_OS_NAME);
        pj_strdup2_with_null(tdata->pool, &user_agent, tmp);

        h = (pjsip_hdr*) pjsip_generic_string_hdr_create(tdata->pool, &USER_AGENT, &user_agent);
        pjsip_msg_add_hdr(tdata->msg, h);
    }

    status = pjsip_endpt_send_response2(pjsua_get_pjsip_endpt(), rdata, tdata, NULL, NULL);
            if (status != PJ_SUCCESS) pjsip_tx_data_dec_ref(tdata);

    return PJ_TRUE;
}

static void SipCallTimeoutCb(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry) {
    pjsua_call_id call_id = entry->id;
    pjsua_msg_data msg_data_;
    pjsip_generic_string_hdr warn;
    pj_str_t hname = pj_str("Warning");
    pj_str_t hvalue = pj_str("399 pjsua \"Call duration exceeded\"");

    PJ_UNUSED_ARG(timer_heap);

    if (call_id == PJSUA_INVALID_ID) {
        LOG_ERR("Invalid call ID in timer callback");
        return;
    }
    
    /* Add warning header */
    pjsua_msg_data_init(&msg_data_);
    pjsip_generic_string_hdr_init2(&warn, &hname, &hvalue);
    pj_list_push_back(&msg_data_.hdr_list, &warn);

    /* Call duration has been exceeded; disconnect the call */
    LOG_ERR("Invalid call ID in timer callback");

    LOG_INFO("Duration (60 seconds) has been exceeded for call %d, disconnecting the call", call_id);
    entry->id = PJSUA_INVALID_ID;
    pjsua_call_hangup(call_id, 200, NULL, &msg_data_);
}

static pjsip_module kPjModule = {
    NULL, NULL,                         /* prev, next.          */
    { "pj-module", 9 },                 /* Name.                */
    -1,                                 /* Id                   */
    PJSIP_MOD_PRIORITY_APPLICATION+99,  /* Priority             */
    NULL,                               /* load()               */
    NULL,                               /* start()              */
    NULL,                               /* stop()               */
    NULL,                               /* unload()             */
    &SipOnRxRequest,                    /* on_rx_request()      */
    NULL,                               /* on_rx_response()     */
    NULL,                               /* on_tx_request.       */
    NULL,                               /* on_tx_response()     */
    NULL,                               /* on_tsx_state()       */
};

static int SipPushStreamProc(void* arg) {
    unsigned short seq = 0;
    while (!kSipMng.stream_thread_quit) {
        if (kSipMng.port == NULL) {
            usleep(10*1000);
            continue;
        }

        pthread_mutex_lock(&kSipMng.stream_mutex);
        if (kSipMng.stream_frame.buff == NULL || kSipMng.stream_frame.size == 0) {
            pthread_mutex_unlock(&kSipMng.stream_mutex);
            usleep(1000);
            continue;
        }

        pjmedia_frame frame;
        frame.type = PJMEDIA_FRAME_TYPE_VIDEO;
        frame.size = kSipMng.stream_frame.size;
        frame.timestamp.u64 = kSipMng.stream_frame.timestamp;
        frame.timestamp.u32.lo = kSipMng.stream_frame.timestamp & 0xffffffff;
        frame.timestamp.u32.hi = (kSipMng.stream_frame.timestamp >> 32) & 0xffffffff;
        frame.buf = malloc(frame.size+1);
        memset(frame.buf, 0, frame.size+1);
        memcpy(frame.buf, kSipMng.stream_frame.buff, frame.size);

        free(kSipMng.stream_frame.buff);
        memset(&kSipMng.stream_frame, 0, sizeof(SipStreamFrame));
        pthread_mutex_unlock(&kSipMng.stream_mutex);

        pj_status_t status = pjmedia_port_put_frame(kSipMng.port, &frame);
        LOG_INFO("push video stream, status:%d size:%d", status, frame.size);
        free(frame.buf);
    }
    
    return 0;
}

int SipInit() {
    pj_status_t status = pjsua_create();
    CHECK_EQ(status, PJ_SUCCESS, return -1);

    kSipMng.pool = pjsua_pool_create("gb28181-app", 1000, 1000);
    pj_pool_t* tmp_pool = pjsua_pool_create("gb28181-test", 1000, 1000);

    pjsua_config_default(&kSipMng.cfg);
    char buff[128] = {0};
    pj_ansi_snprintf(buff, sizeof(buff), "PJSUA v%s %s", pj_get_version(), pj_get_sys_info()->info.ptr);
    pj_strdup2_with_null(kSipMng.pool, &kSipMng.cfg.user_agent, buff);
    kSipMng.cfg.cb.on_call_state = &SipOnCallState;
    kSipMng.cfg.cb.on_stream_destroyed = &SipOnStreamDestroyed;
    kSipMng.cfg.cb.on_call_media_state = &SipOnCallMediaState;
    kSipMng.cfg.cb.on_incoming_call = &SipOnIncomingCall;
    kSipMng.cfg.cb.on_dtmf_digit2 = &SipCallOnDtmfCallback2;
    kSipMng.cfg.cb.on_call_redirected = &SipCallOnRedirected;
    kSipMng.cfg.cb.on_reg_state = &SipOnRegState;
    kSipMng.cfg.cb.on_incoming_subscribe = &SipOnIncomingSubscribe;
    kSipMng.cfg.cb.on_buddy_state = &SipOnBuddyState;
    kSipMng.cfg.cb.on_buddy_evsub_state = &SipOnBuddyEvsubState;
    kSipMng.cfg.cb.on_pager = &SipOnPager;
    kSipMng.cfg.cb.on_typing = &SipOnTyping;
    kSipMng.cfg.cb.on_call_transfer_status = &SipOnCallTransferStatus;
    kSipMng.cfg.cb.on_call_replaced = &SipOnCallReplaced;
    kSipMng.cfg.cb.on_nat_detect = &SipOnNatDetect;
    kSipMng.cfg.cb.on_mwi_info = &SipOnMwiInfo;
    kSipMng.cfg.cb.on_transport_state = &SipOnTransportState;
    kSipMng.cfg.cb.on_ice_transport_error = &SipOnIceTransportError;
    kSipMng.cfg.cb.on_snd_dev_operation = &SipOnSndDevOperation;
    kSipMng.cfg.cb.on_call_media_event = &SipOnCallMediaEvent;
    kSipMng.cfg.cb.on_ip_change_progress = &SipOnIpChangeProgress;
    kSipMng.cfg.cb.on_create_media_transport = &SipOnCreateMediaTransport;

    pjsua_media_config_default(&kSipMng.media_cfg);
    kSipMng.media_cfg.snd_rec_latency = PJMEDIA_SND_DEFAULT_REC_LATENCY;
    kSipMng.media_cfg.snd_play_latency = PJMEDIA_SND_DEFAULT_PLAY_LATENCY;
    kSipMng.media_cfg.vid_preview_enable_native = PJ_FALSE;

    pjsua_logging_config_default(&kSipMng.log_cfg);
    kSipMng.log_cfg.level = 5;
    status = pjsua_init(&kSipMng.cfg, &kSipMng.log_cfg, &kSipMng.media_cfg);
    CHECK_EQ(status, PJ_SUCCESS, goto end);

    status = pjsip_endpt_register_module(pjsua_get_pjsip_endpt(), &kPjModule);
    CHECK_EQ(status, PJ_SUCCESS, goto end);

    for (int i = 0; i < sizeof(kSipMng.call_data)/sizeof(kSipMng.call_data[0]); i++) {
        kSipMng.call_data[i].timer.id = PJSUA_INVALID_ID;
        kSipMng.call_data[i].timer.cb = SipCallTimeoutCb;
    }
    
    pjsua_acc_id acc_id;
    pjsua_acc_config acc_cfg;
    pjsua_transport_id transport_id;

    pjsua_transport_config_default(&kSipMng.udp_cfg);
    kSipMng.udp_cfg.port = 5060;

    pjsua_transport_config_default(&kSipMng.tcp_cfg);
    kSipMng.tcp_cfg.port = 5060;

    pjsua_transport_config_default(&kSipMng.rtp_cfg);
    kSipMng.rtp_cfg.port = 4000;

    status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &kSipMng.udp_cfg, &transport_id);
    CHECK_EQ(status, PJ_SUCCESS, goto end);
    pjsua_acc_add_local(transport_id, PJ_TRUE, &acc_id);

    pjsua_acc_get_config(acc_id, tmp_pool, &acc_cfg);
    acc_cfg.rtp_cfg = kSipMng.rtp_cfg;
    pjsua_acc_modify(acc_id, &acc_cfg);
    
    pjsua_acc_set_online_status(pjsua_acc_get_default(), PJ_TRUE);

    status = pjsua_transport_create(PJSIP_TRANSPORT_TCP, &kSipMng.tcp_cfg, &transport_id);
    CHECK_EQ(status, PJ_SUCCESS, goto end);
    pjsua_acc_add_local(transport_id, PJ_TRUE, &acc_id);

    pjsua_acc_get_config(acc_id, tmp_pool, &acc_cfg);

    acc_cfg.rtp_cfg = kSipMng.rtp_cfg;
    pjsua_acc_modify(acc_id, &acc_cfg);
    
    pjsua_acc_set_online_status(pjsua_acc_get_default(), PJ_TRUE);

    CHECK_LT(transport_id, -1, goto end);

    pjsua_acc_config_default(&kSipMng.acc_cfg);
    kSipMng.acc_cfg.rtp_cfg = acc_cfg.rtp_cfg;
    kSipMng.acc_cfg.proxy_cnt = 1;
    kSipMng.acc_cfg.proxy[0] = pj_str("sip:192.168.110.168:15060");
    kSipMng.acc_cfg.id = pj_str("sip:" SIP_USER "@" SIP_DOMAIN);
    kSipMng.acc_cfg.reg_uri = pj_str("sip:" SIP_DOMAIN);
    kSipMng.acc_cfg.cred_count = 1;
    kSipMng.acc_cfg.cred_info[0].realm = pj_str(SIP_DOMAIN);
    kSipMng.acc_cfg.cred_info[0].scheme = pj_str("digest");
    kSipMng.acc_cfg.cred_info[0].username = pj_str(SIP_USER);
    kSipMng.acc_cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    kSipMng.acc_cfg.cred_info[0].data = pj_str(SIP_PASSWD);
    status = pjsua_acc_add(&kSipMng.acc_cfg, PJ_TRUE, &kSipMng.acc_id);
    CHECK_EQ(status, PJ_SUCCESS, goto end);
    pjsua_acc_set_online_status(pjsua_acc_get_default(), PJ_TRUE);

    pj_str_t codec_arg = pj_str("H264/90000");
    pjsua_codec_set_priority(&codec_arg, PJMEDIA_CODEC_PRIO_NORMAL);
    // pjsua_vid_codec_set_priority(&codec_arg, PJMEDIA_CODEC_PRIO_NORMAL);

    pjsua_call_setting_default(&kSipMng.call_opt);
    kSipMng.call_opt.aud_cnt = 0;
    kSipMng.call_opt.vid_cnt = 1;

    status = pjsua_start();
    CHECK_EQ(status, PJ_SUCCESS, goto end);

    status = pj_thread_create(kSipMng.pool, "push_stream", &SipPushStreamProc, NULL, 0, 0, &kSipMng.stream_thread_id);
    CHECK_EQ(status, PJ_SUCCESS, goto end);

    pj_pool_release(tmp_pool);
    return 0;
end:
    pj_pool_release(tmp_pool);
    pj_pool_release(kSipMng.pool);
    pjsua_destroy();
    return -1;
}

void SipUnInit() {
    if (kSipMng.stream_thread_id != NULL) {
        kSipMng.stream_thread_quit = true;
        pj_thread_join(kSipMng.stream_thread_id);
        pj_thread_destroy(kSipMng.stream_thread_id);
        kSipMng.stream_thread_id = NULL;
        kSipMng.stream_thread_quit = false;
    }

    pjsua_destroy();
}

int SipPushStream(SipStreamFrame* frame) {
    CHECK_POINTER(frame, return -1);
    CHECK_POINTER(frame->buff, return -1);
    CHECK_LE(frame->size, 0, return -1);

    pthread_mutex_lock(&kSipMng.stream_mutex);
    if (kSipMng.stream_frame.buff != NULL) {
        void* p = realloc(kSipMng.stream_frame.buff, frame->size+1);
        kSipMng.stream_frame.buff = p;
    } else {
        kSipMng.stream_frame.buff = malloc(frame->size + 1);
    }
    CHECK_POINTER(kSipMng.stream_frame.buff, pthread_mutex_unlock(&kSipMng.stream_mutex);return -1);

    memset(kSipMng.stream_frame.buff, 0, frame->size+1);
    memcpy(kSipMng.stream_frame.buff, frame->buff, frame->size);
    kSipMng.stream_frame.size = frame->size;
    kSipMng.stream_frame.timestamp = frame->timestamp;

    pthread_mutex_unlock(&kSipMng.stream_mutex);
}