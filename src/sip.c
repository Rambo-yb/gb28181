#include "sip.h"
#include "check_common.h"

#include "pjsua-lib/pjsua.h"
#include "libmxml4/mxml.h"

#define SIP_DOMAIN      "3402000000"
#define SIP_USER        "34020000002000000001"
#define SIP_PASSWD      "12345678"

pjsua_acc_id acc_id;

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
        mxmlNewText(mxmlNewElement(response, "DeviceID"), space, "34020000002000000001");
        mxmlNewText(mxmlNewElement(response, "SumNum"), space, "1");

        mxml_node_t *device_list = mxmlNewElement(response, "DeviceList");
        mxmlElementSetAttr(device_list, "Num", "1");
        mxml_node_t *item = mxmlNewElement(device_list, "Item");
        mxmlNewText(mxmlNewElement(item, "DeviceID"), space, "34020000002000000001");
        mxmlNewText(mxmlNewElement(item, "Name"), space, "Camera");
        mxmlNewText(mxmlNewElement(item, "Manufacturer"), space, "IPC_Company");
        mxmlNewText(mxmlNewElement(item, "Model"), space, "HI351X");
        mxmlNewText(mxmlNewElement(item, "Owner"), space, "General");
        mxmlNewText(mxmlNewElement(item, "CivilCode"), space, "3402000000");
        mxmlNewText(mxmlNewElement(item, "Block"), space, "General");
        mxmlNewText(mxmlNewElement(item, "Address"), space, "China");
        mxmlNewInteger(mxmlNewElement(item, "Parental"), 0);
        mxmlNewText(mxmlNewElement(item, "ParentID"), space, "34020000002000000001");
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

        pjsua_im_send(acc_id, to, mime_type, &send_buf, NULL, NULL);
        free(buff);
        mxmlDelete(xml);
    }
    mxmlDelete(tree);
    mxmlOptionsDelete(options);
}

int SipInit() {
    pj_status_t state = pjsua_create();
    CHECK_EQ(state, PJ_SUCCESS, return -1);
    pjsua_config cfg;
    pjsua_logging_config log_cfg;
    pjsua_config_default(&cfg);
    cfg.cb.on_pager = &SipOnPager;

    pjsua_logging_config_default(&log_cfg);
    log_cfg.console_level = 1;

    state = pjsua_init(&cfg, &log_cfg, NULL);
    CHECK_EQ(state, PJ_SUCCESS, pjsua_destroy();return -1);


    pjsua_transport_config ts_cfg;
    pjsua_transport_config_default(&ts_cfg);
    ts_cfg.port = 5060;
    state = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &ts_cfg, NULL);
    CHECK_EQ(state, PJ_SUCCESS, pjsua_destroy();return -1);

    state = pjsua_start();
    CHECK_EQ(state, PJ_SUCCESS, pjsua_destroy();return -1);

    pjsua_acc_config acc_cfg;
    pjsua_acc_config_default(&acc_cfg);
    acc_cfg.proxy_cnt = 1;
    acc_cfg.proxy[0] = pj_str("sip:192.168.110.168:15060");
    acc_cfg.id = pj_str("sip:" SIP_USER "@" SIP_DOMAIN);
    acc_cfg.reg_uri = pj_str("sip:" SIP_DOMAIN);
    acc_cfg.cred_count = 1;
    acc_cfg.cred_info[0].realm = pj_str(SIP_DOMAIN);
    acc_cfg.cred_info[0].scheme = pj_str("digest");
    acc_cfg.cred_info[0].username = pj_str(SIP_USER);
    acc_cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    acc_cfg.cred_info[0].data = pj_str(SIP_PASSWD);
    state = pjsua_acc_add(&acc_cfg, PJ_TRUE, &acc_id);
    CHECK_EQ(state, PJ_SUCCESS, pjsua_destroy();return -1);

    return 0;
}

void SipUnInit() {
    pjsua_destroy();
}