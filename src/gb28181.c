#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "gb28181.h"
#include "gb28181_common.h"
#include "rtp.h"
#include "sip.h"
#include "list.h"
#include "log.h"
#include "check_common.h"

#include "libmxml4/mxml.h"
#include "eXosip2/eXosip.h"

#define GB28181_RTP_LISTEN_PORT 3998

typedef struct {
	int type;
	void* cb;
}Gb28181OperationInfo;

typedef struct {
    char method[8];
	char* body;
}Gb28181MsgSendInfo;

typedef struct {
    int cid;
    int did;
    void* rtp_hander;

	int ts_type;
	char rem_addr[16];
	int rem_port;
	int ssrc;

	int start_time;
	int stop_time;
}Gb28181RtpInfo;

typedef struct {
	
	int reg_id;
	int reg_flag;
	Gb28181Info gb28181_info;
	struct eXosip_t *excontext;

	int sn;
	SipCatalogInfo catalog_info;

	Gb28181RtpInfo rtp_play;
	Gb28181RtpInfo rtp_playback;

	void* oper_list;
	pthread_mutex_t oper_mutex;

	void* send_list;
	pthread_mutex_t send_mutex;

	pthread_t pthread_recv;
	pthread_t pthread_send;
	pthread_t pthread_keeplive;
	pthread_t pthread_rtp_active;
}Gb28181Mng;
static Gb28181Mng kGb28181Mng = {.send_mutex = PTHREAD_MUTEX_INITIALIZER, .oper_mutex = PTHREAD_MUTEX_INITIALIZER};

static void* Gb28181GetOperationCb(int type) {
    Gb28181OperationInfo* oper_info = NULL;
    int list_size = ListSize(kGb28181Mng.oper_list);
    for (int i = 0; i < list_size; i++) {
        oper_info = ListGet(kGb28181Mng.oper_list, i);
        if (oper_info->type == type) {
            break;
        }
        oper_info = NULL;
    }

    return oper_info;
}

static void Gb28181CallProc(eXosip_event_t * event) {
    osip_message_t* resp = NULL;
    eXosip_call_build_answer(kGb28181Mng.excontext, event->tid, 200, &resp);

    osip_body_t* body = NULL;
    osip_message_get_body(event->request, 0, &body);
    LOG_DEBUG("body:%s", body->body);

    sdp_message_t* rem_sdp = NULL;
    sdp_message_init(&rem_sdp);
    sdp_message_parse(rem_sdp, body->body);

	char u[128] = {0};
	if(rem_sdp->u_uri != NULL) {
		snprintf(u, sizeof(u), "u=%s\r\n", rem_sdp->u_uri);
	}

    char* start_time = sdp_message_t_start_time_get(rem_sdp, 0);
    char* stop_time = sdp_message_t_stop_time_get(rem_sdp, 0);
	char* m_proto = sdp_message_m_proto_get(rem_sdp, 0);
	int m_port = atoi(sdp_message_m_port_get(rem_sdp, 0));

	char a[128] = {0};
	if (strcmp(m_proto, "TCP/RTP/AVP") == 0) {
		char value[8] = {0};
		for (int i = 0; ; i++)
		{
			sdp_attribute_t* attr = sdp_message_attribute_get(rem_sdp, 0, i);
			if (attr == NULL) {
				break;
			} if (strcmp(attr->a_att_field, "setup") == 0) {
				snprintf(value, sizeof(value), "%s", attr->a_att_value);
				break;
			}
		}

		snprintf(a, sizeof(a), "a=setup:%s\r\na=connection:new\r\n", value);
	}

	Gb28181RtpInfo* rtp_info = strcmp(rem_sdp->s_name, "Play") == 0 ? &kGb28181Mng.rtp_play : &kGb28181Mng.rtp_playback;
	rtp_info->cid = event->cid;
	rtp_info->did = event->did;
	rtp_info->start_time = atoi(start_time);
	rtp_info->stop_time = atoi(stop_time);
	
	rtp_info->ts_type = RTP_TRANSPORT_STREAM_UDP;
	if (strcmp(m_proto, "TCP/RTP/AVP") == 0 && strstr(a, "active") != NULL) {
		rtp_info->ts_type = RTP_TRANSPORT_STREAM_TCP_ACTIVE;
	} else if (strcmp(m_proto, "TCP/RTP/AVP") == 0 && strstr(a, "passive") != NULL) {
		rtp_info->ts_type = RTP_TRANSPORT_STREAM_TCP_PASSIVE;
	}
	
	rtp_info->rem_port = m_port;
	snprintf(rtp_info->rem_addr, sizeof(rtp_info->rem_addr), "%s", sdp_message_o_addr_get(rem_sdp));
	rtp_info->ssrc = atoi(rem_sdp->y_ssrc);

	if (rtp_info->ts_type != RTP_TRANSPORT_STREAM_TCP_ACTIVE) {
		rtp_info->rtp_hander = RtpInit(rtp_info->ts_type, rtp_info->rem_addr, rtp_info->rem_port, rtp_info->ssrc);
		CHECK_POINTER(rtp_info->rtp_hander, return );
	}

    char buff[1024] = {0};
    snprintf(buff, sizeof(buff),
        "v=0\r\n" \
        "o=%s 0 0 IN IP4 %s\r\n" \
        "s=%s\r\n" \
		"%s" \
        "c=IN IP4 %s\r\n" \
        "t=%s %s\r\n" \
        "m=video %d %s 96\r\n" \
        "a=sendonly\r\n" \
        "a=rtpmap:96 PS/90000\r\n" \
		"%s" \
        "y=%s\r\n",
        sdp_message_o_username_get(rem_sdp), "192.168.110.223", rem_sdp->s_name, u, "192.168.110.223", start_time, stop_time, 
        rtp_info->ts_type == RTP_TRANSPORT_STREAM_TCP_ACTIVE ? GB28181_RTP_LISTEN_PORT : RtpTransportPort(rtp_info->rtp_hander), m_proto, a, rem_sdp->y_ssrc);
    LOG_DEBUG("\n%s", buff);

    osip_message_set_header(resp, "Content-Type", "application/sdp");
    osip_message_set_body(resp, buff, strlen(buff));

    sdp_message_free(rem_sdp);
    
    eXosip_call_send_answer(kGb28181Mng.excontext, event->tid, 200, resp);
}

static void Gb28181CallAckProc(eXosip_event_t * event) {
	int tm = time(NULL);
	if (event->cid == kGb28181Mng.rtp_play.cid && event->did == kGb28181Mng.rtp_play.did) {
		while(!kGb28181Mng.rtp_play.rtp_hander) {
			if (tm + 10 < time(NULL)) {
				memset(&kGb28181Mng.rtp_play, 0, sizeof(Gb28181RtpInfo));
				break;
			}
			usleep(10*1000);
		}

		Gb28181OperationInfo* oper_info = Gb28181GetOperationCb(GB28181_OPERATION_CONTROL);
		CHECK_POINTER(oper_info, return);

		Gb28181PlayStream play_stream;
		memset(&play_stream, 0, sizeof(Gb28181PlayStream));
		((Gb28181OperationControlCb)oper_info->cb)(GB28181_CONTORL_PLAY_STREAM, &play_stream, sizeof(Gb28181PlayStream));
	} else if (event->cid == kGb28181Mng.rtp_playback.cid && event->did == kGb28181Mng.rtp_playback.did) {
		while(!kGb28181Mng.rtp_playback.rtp_hander) {
			if (tm + 10 < time(NULL)) {
				memset(&kGb28181Mng.rtp_playback, 0, sizeof(Gb28181RtpInfo));
				return;
			}
			usleep(10*1000);
		}

		Gb28181OperationInfo* oper_info = Gb28181GetOperationCb(GB28181_OPERATION_CONTROL);
		CHECK_POINTER(oper_info, return);

		Gb28181PlayRecord play_record;
		memset(&play_record, 0, sizeof(Gb28181PlayRecord));
		play_record.start_time = kGb28181Mng.rtp_playback.start_time;
		play_record.end_time = kGb28181Mng.rtp_playback.stop_time;
		((Gb28181OperationControlCb)oper_info->cb)(GB28181_CONTORL_PLAY_RECORD, &play_record, sizeof(Gb28181PlayRecord));
	}
}

static void Gb28181CallMsgProc(eXosip_event_t * event) {
	osip_body_t* body = NULL;
	osip_message_get_body(event->request, 0, &body);
	LOG_DEBUG("body:%s", body->body);

	Gb28181OperationInfo* oper_info = Gb28181GetOperationCb(GB28181_OPERATION_CONTROL);
	CHECK_POINTER(oper_info, return);

	Gb28181PlayRecord play_record;
	memset(&play_record, 0, sizeof(Gb28181PlayRecord));
	if (strncmp(body->body, "PAUSE", strlen("PAUSE")) == 0) {
		play_record.type = GB28181_PLAY_VIDEO_PAUSE;
	} else {
		if (strstr(body->body, "Range") != NULL) {
			play_record.type = GB28181_PLAY_VIDEO_PLAY;
		} else if (strstr(body->body, "Scale") != NULL) {
			play_record.type = GB28181_PLAY_VIDEO_SCALE;

			char* p = strstr(body->body, "Scale");
			sscanf(p, "Scale: %f", &play_record.scale);
		}
	}

	((Gb28181OperationControlCb)oper_info->cb)(GB28181_CONTORL_PLAY_RECORD, &play_record, sizeof(Gb28181PlayRecord));
}

static void Gb28181CallClosedProc(eXosip_event_t * event) {
	Gb28181OperationInfo* oper_info = Gb28181GetOperationCb(GB28181_OPERATION_CONTROL);
	CHECK_POINTER(oper_info, return);

	if (event->cid == kGb28181Mng.rtp_play.cid && event->did == kGb28181Mng.rtp_play.did) {
		Gb28181PlayStream play_stream;
		memset(&play_stream, 0, sizeof(Gb28181PlayStream));
		play_stream.type = GB28181_PLAY_VIDEO_STOP;
		((Gb28181OperationControlCb)oper_info->cb)(GB28181_CONTORL_PLAY_STREAM, &play_stream, sizeof(Gb28181PlayStream));

		RtpUnInit(kGb28181Mng.rtp_play.rtp_hander);
		memset(&kGb28181Mng.rtp_play, 0, sizeof(Gb28181RtpInfo));
	} else if (event->cid == kGb28181Mng.rtp_playback.cid && event->did == kGb28181Mng.rtp_playback.did) {
		Gb28181PlayRecord play_record;
		memset(&play_record, 0, sizeof(Gb28181PlayRecord));
		play_record.type = GB28181_PLAY_VIDEO_STOP;
		((Gb28181OperationControlCb)oper_info->cb)(GB28181_CONTORL_PLAY_RECORD, &play_record, sizeof(Gb28181PlayRecord));

		RtpUnInit(kGb28181Mng.rtp_playback.rtp_hander);
		memset(&kGb28181Mng.rtp_playback, 0, sizeof(Gb28181RtpInfo));
	}
}

typedef int(*Gb28181MsgSubCb)(void* st, mxml_node_t*, char**, void*);

typedef struct {
    const char* msg_type;
    const char* cmd_type;
	const char* ctrl_type;
    bool resp_flag;
	void* st;
    Gb28181MsgSubCb cb;
}Gb28181MsgProcInfo;

static Gb28181MsgProcInfo kProcInfo[] = {
	{.msg_type = "Query", .cmd_type = "Catalog", .resp_flag = true, .st = &kGb28181Mng.catalog_info, .cb = SipMsgCatalog},
	{.msg_type = "Query", .cmd_type = "DeviceStatus", .resp_flag = true, .cb = SipMsgDeviceStatus},
	{.msg_type = "Query", .cmd_type = "DeviceInfo", .resp_flag = true, .st = &kGb28181Mng.catalog_info, .cb = SipMsgDeviceInfo},
	{.msg_type = "Query", .cmd_type = "RecordInfo", .resp_flag = true, .cb = SipMsgRecordInfo},
	{.msg_type = "Query", .cmd_type = "Alarm", .resp_flag = true, .cb = SipMsgAlarm},

	{.msg_type = "Control", .cmd_type = "DeviceControl", .ctrl_type = "PTZCmd", .resp_flag = false, .cb = SipMsgPtzCmd},
	{.msg_type = "Control", .cmd_type = "DeviceControl", .ctrl_type = "RecordCmd", .resp_flag = true, .cb = SipMsgRecordCmd},
	{.msg_type = "Control", .cmd_type = "DeviceControl", .ctrl_type = "GuardCmd", .resp_flag = true, .cb = SipMsgGuardCmd},};

static void Gb28181MsgProc(eXosip_event_t * event) {
    osip_body_t* body = NULL;
    osip_message_get_body(event->request, 0, &body);
    LOG_DEBUG("body:%s", body->body);
    
    bool space = false;
    mxml_options_t* options = mxmlOptionsNew();     
    mxml_node_t* tree = mxmlLoadString(NULL, options, body->body);
    mxml_node_t* cmd_type = mxmlFindElement(tree, tree, "CmdType", NULL, NULL, MXML_DESCEND_ALL);

    for (int i = 0; i < sizeof(kProcInfo) / sizeof(Gb28181MsgProcInfo); i++) {
        if (mxmlFindElement(tree, tree, kProcInfo[i].msg_type, NULL, NULL, MXML_DESCEND_ALL) != NULL 
            && strcmp(kProcInfo[i].cmd_type, mxmlGetText(cmd_type, &space)) == 0 
			&& (kProcInfo[i].ctrl_type == NULL || mxmlFindElement(tree, tree, kProcInfo[i].ctrl_type, NULL, NULL, MXML_DESCEND_ALL) != NULL)) {
			Gb28181OperationInfo* oper_info = Gb28181GetOperationCb(GB28181_OPERATION_CONTROL);
			CHECK_POINTER(oper_info, goto end);

            if (kProcInfo[i].resp_flag) {
                Gb28181MsgSendInfo info;
                snprintf(info.method, sizeof(info.method), "MESSAGE");
				int ret = kProcInfo[i].cb(kProcInfo[i].st, tree, &info.body, oper_info->cb);

				if (ret < 0) {
					eXosip_message_send_answer(kGb28181Mng.excontext, event->tid, 400, NULL);
				} else {
					pthread_mutex_lock(&kGb28181Mng.send_mutex);
					ListPush(kGb28181Mng.send_list, &info, sizeof(Gb28181MsgSendInfo));
					pthread_mutex_unlock(&kGb28181Mng.send_mutex);

					eXosip_message_send_answer(kGb28181Mng.excontext, event->tid, 200, NULL);
				}
            } else {
                kProcInfo[i].cb(kProcInfo[i].st, tree, NULL, oper_info->cb);
            }

            break;
        }
    }

end:
    mxmlDelete(tree);
    mxmlOptionsDelete(options);
}

static void* Gb28181RecvProc(void* arg) {
    while(1) {
        eXosip_event_t *event = eXosip_event_wait(kGb28181Mng.excontext, 0, 10);

        eXosip_lock(kGb28181Mng.excontext);
        eXosip_automatic_action(kGb28181Mng.excontext);
        eXosip_unlock(kGb28181Mng.excontext);

        if (event == NULL) {
            usleep(10*1000);
            continue;
        }

        eXosip_lock(kGb28181Mng.excontext);
        LOG_INFO("cid:%d, did:%d, type:%d", event->cid, event->did, event->type);
        switch (event->type)
        {
        case EXOSIP_REGISTRATION_SUCCESS:
            kGb28181Mng.reg_flag = true;
            break;
        case EXOSIP_REGISTRATION_FAILURE:
            break;
        case EXOSIP_CALL_INVITE:
            if (MSG_IS_INVITE(event->request)){
                Gb28181CallProc(event);
            }
            break;
        case EXOSIP_CALL_ACK:
            if (MSG_IS_ACK(event->ack)) {
				Gb28181CallAckProc(event);
            }
            break;
        case EXOSIP_CALL_MESSAGE_NEW:
            if (MSG_IS_INFO(event->request)) {
				Gb28181CallMsgProc(event);
            }
            break;
        case EXOSIP_CALL_CLOSED:
            if (MSG_IS_BYE(event->request)) {
				Gb28181CallClosedProc(event);
            }
            break;
        case EXOSIP_MESSAGE_NEW:
            if (MSG_IS_MESSAGE(event->request)) {
                Gb28181MsgProc(event);
            }
            break;
        default:
            break;
        }

        eXosip_unlock(kGb28181Mng.excontext);
        eXosip_event_free(event);
    }
}

static void* Gb28181SendProc(void* arg) {
    while (1) {
        Gb28181MsgSendInfo info;
        pthread_mutex_lock(&kGb28181Mng.send_mutex);
        if (ListSize(kGb28181Mng.send_list) <= 0) {
            pthread_mutex_unlock(&kGb28181Mng.send_mutex);
            usleep(1000);
            continue;
        }

        if (ListPop(kGb28181Mng.send_list, &info, sizeof(Gb28181MsgSendInfo)) < 0) {
            pthread_mutex_unlock(&kGb28181Mng.send_mutex);
            usleep(1000);
            continue;
        }
        pthread_mutex_unlock(&kGb28181Mng.send_mutex);

		osip_message_t* msg = NULL;
		char to[256] = {0};
		snprintf(to, sizeof(to), "sip:%s:%d", kGb28181Mng.gb28181_info.sip_addr, kGb28181Mng.gb28181_info.sip_port);

		char from[256] = {0};
		snprintf(from, sizeof(from), "sip:%s@%s", kGb28181Mng.gb28181_info.sip_username, kGb28181Mng.gb28181_info.sip_domain);

		eXosip_message_build_request(kGb28181Mng.excontext, &msg, info.method, to, from, NULL);
		osip_message_set_header(msg, "Content-Type", "Application/MANSCDP+xml");
		osip_message_set_body(msg, info.body, strlen(info.body));
		LOG_DEBUG("%s", info.body);
		eXosip_message_send_request(kGb28181Mng.excontext, msg);
		free(info.body);

        usleep(1000);
    }
    
}

static void* Gb28181KeepliveProc(void* arg) {
	int last_time = 0;
	while(1) {
        if (!kGb28181Mng.reg_flag) {
			usleep(1000*1000);
			continue;
        }

		int cur_time = time(NULL);
		if (last_time + kGb28181Mng.gb28181_info.keeplive_interval > cur_time) {
			usleep(1000*1000);
			continue;
		}

		Gb28181MsgSendInfo info;
        snprintf(info.method, sizeof(info.method), "MESSAGE");
		SipNotifyKeepalive(kGb28181Mng.sn++, kGb28181Mng.gb28181_info.sip_username, &info.body);

		pthread_mutex_lock(&kGb28181Mng.send_mutex);
		ListPush(kGb28181Mng.send_list, &info, sizeof(Gb28181MsgSendInfo));
		pthread_mutex_unlock(&kGb28181Mng.send_mutex);

		last_time = cur_time;
	}
}

#define MAX_CLIENT_NUM 5
static void* Gb28181RtpListen(void* arg) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	CHECK_LT(sockfd, 0, return NULL);
	
	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt));

	struct sockaddr_in svr_addr;
	svr_addr.sin_family = AF_INET;
    svr_addr.sin_port = htons(GB28181_RTP_LISTEN_PORT);
    svr_addr.sin_addr.s_addr = INADDR_ANY;
	int ret = bind(sockfd, (struct sockaddr *)&svr_addr, sizeof(struct sockaddr_in));
	CHECK_LT(ret, 0, close(sockfd); return NULL);

	ret = listen(sockfd, MAX_CLIENT_NUM);
	CHECK_LT(ret, 0, close(sockfd); return NULL);

	while (1) {
		struct sockaddr_in cli_addr;
		int len = sizeof(struct sockaddr_in);
		int cli_fd = accept(sockfd, (struct sockaddr *)&svr_addr, &len);

		Gb28181RtpInfo* rtp_info = NULL;
		if (kGb28181Mng.rtp_play.ts_type == RTP_TRANSPORT_STREAM_TCP_ACTIVE && kGb28181Mng.rtp_play.rtp_hander == NULL) {
			rtp_info = &kGb28181Mng.rtp_play;
		} else if (kGb28181Mng.rtp_playback.ts_type == RTP_TRANSPORT_STREAM_TCP_ACTIVE && kGb28181Mng.rtp_playback.rtp_hander == NULL) {
			rtp_info = &kGb28181Mng.rtp_playback;
		} else {
			LOG_ERR("unknown connect !!!");
			close(cli_fd);
			continue;
		}

		rtp_info->rtp_hander = RtpInitV2(RTP_TRANSPORT_STREAM_TCP_ACTIVE, cli_fd, rtp_info->ssrc);
		CHECK_POINTER(rtp_info->rtp_hander, close(cli_fd); continue);

		usleep(10*1000);
	}
}

int Gb28181Init() {
    log_init((char*)"./gb28181.log", 512*1024, 3, 5);

	kGb28181Mng.excontext = eXosip_malloc();

	int ret = eXosip_init(kGb28181Mng.excontext);
	CHECK_EQ(ret, 0, goto end);

	ret = eXosip_listen_addr(kGb28181Mng.excontext, IPPROTO_UDP, NULL, 5060, AF_INET, 0);
	CHECK_EQ(ret, 0, goto end);

	eXosip_set_user_agent(kGb28181Mng.excontext, "HbsGBSIP-1.0");

	kGb28181Mng.send_list = ListCreate();
	CHECK_POINTER(kGb28181Mng.send_list, goto end);

	kGb28181Mng.oper_list = ListCreate();
	CHECK_POINTER(kGb28181Mng.oper_list, goto end);

	pthread_create(&kGb28181Mng.pthread_recv, NULL, Gb28181RecvProc, NULL);
	pthread_create(&kGb28181Mng.pthread_send, NULL, Gb28181SendProc, NULL);
	pthread_create(&kGb28181Mng.pthread_keeplive, NULL, Gb28181KeepliveProc, NULL);
	pthread_create(&kGb28181Mng.pthread_rtp_active, NULL, Gb28181RtpListen, NULL);

	return 0;

end:
	if (kGb28181Mng.send_list) {
		ListDestory(kGb28181Mng.send_list);
	}

	if (kGb28181Mng.oper_list) {
		ListDestory(kGb28181Mng.oper_list);
	}

	eXosip_quit(kGb28181Mng.excontext);
	osip_free(kGb28181Mng.excontext);
	return -1;
}

void Gb28181UnInit() {
    if (kGb28181Mng.excontext != NULL) {
        eXosip_quit(kGb28181Mng.excontext);
        osip_free(kGb28181Mng.excontext);
    }
}

static void Gb28181InitMsgInfo(Gb28181Info* info) {
	SipCatalogInfo catalog_info;
	memset(&catalog_info, 0, sizeof(SipCatalogInfo));
	strcpy(catalog_info.device_id, info->sip_username);
	snprintf(catalog_info.name, sizeof(catalog_info.name), "camera");
	snprintf(catalog_info.manu_facturer, sizeof(catalog_info.manu_facturer), "cdjp");
	snprintf(catalog_info.model, sizeof(catalog_info.model), "rv1126");
	snprintf(catalog_info.owner, sizeof(catalog_info.owner), "none");
	strcpy(catalog_info.civil_code, info->sip_domain);
	snprintf(catalog_info.address, sizeof(catalog_info.address), "chengdu");
	catalog_info.parental = 0;
	strcpy(catalog_info.parent_id, info->sip_id);
	catalog_info.register_way = 1;
	memcpy(&kGb28181Mng.catalog_info, &catalog_info, sizeof(SipCatalogInfo));
}

int Gb28181Register(Gb28181Info* info) {
    CHECK_POINTER(info, return -1);
    CHECK_POINTER(kGb28181Mng.excontext, return -1);
    CHECK_BOOL(!kGb28181Mng.reg_flag, return -1);

    int ret = eXosip_add_authentication_info(kGb28181Mng.excontext, info->sip_username, info->sip_username, info->sip_password, NULL, NULL);
    CHECK_EQ(ret, 0, return -1);

    osip_message_t* reg = NULL;
    eXosip_lock(kGb28181Mng.excontext);
    char proxy[256] = {0};
    snprintf(proxy, sizeof(proxy), "sip:%s:%d", info->sip_addr, info->sip_port);
    char form[256] = {0};
    snprintf(form, sizeof(form), "sip:%s@%s", info->sip_username, info->sip_domain);
    kGb28181Mng.reg_id = eXosip_register_build_initial_register(kGb28181Mng.excontext, form, proxy, form, info->register_valid, &reg);
    CHECK_LT(kGb28181Mng.reg_id, 1, eXosip_unlock(kGb28181Mng.excontext);return -1);

    ret = eXosip_register_send_register(kGb28181Mng.excontext, kGb28181Mng.reg_id, reg);
    eXosip_unlock(kGb28181Mng.excontext);
    CHECK_EQ(ret, 0, return -1);

	memcpy(&kGb28181Mng.gb28181_info, info, sizeof(Gb28181Info));
	Gb28181InitMsgInfo(info);

    return 0;
}

int Gb28181Unregister(Gb28181Info* info) {
    CHECK_POINTER(kGb28181Mng.excontext, return -1);
    CHECK_BOOL(kGb28181Mng.reg_flag, return -1);

    kGb28181Mng.reg_flag = false;

    eXosip_register_remove(kGb28181Mng.excontext, kGb28181Mng.reg_id);
        
    eXosip_clear_authentication_info(kGb28181Mng.excontext);

	return 0;
}

int Gb28181PushStream(int type, unsigned char* stream, unsigned int size, int end) {
    CHECK_POINTER(stream, return -1);
    CHECK_LE(size, 0, return -1);

	if (type == 0 && kGb28181Mng.rtp_play.rtp_hander != NULL) {
        RtpPush(kGb28181Mng.rtp_play.rtp_hander, (unsigned char*)stream, size);
    } else if (type == 1 && kGb28181Mng.rtp_playback.rtp_hander != NULL) {
        RtpPush(kGb28181Mng.rtp_playback.rtp_hander, (unsigned char*)stream, size);

		if(end) {
			Gb28181MsgSendInfo info;
			snprintf(info.method, sizeof(info.method), "MESSAGE");
			SipNotifyMediaStatus(kGb28181Mng.sn++, kGb28181Mng.gb28181_info.sip_username, &info.body);

			pthread_mutex_lock(&kGb28181Mng.send_mutex);
			ListPush(kGb28181Mng.send_list, &info, sizeof(Gb28181MsgSendInfo));
			pthread_mutex_unlock(&kGb28181Mng.send_mutex);
		}
    }

    return 0;
}

void Gb28181OperationRegister(int type, void* cb){
	Gb28181OperationInfo info = {.type = type, .cb = cb};

	pthread_mutex_lock(&kGb28181Mng.oper_mutex);
	ListPush(kGb28181Mng.oper_list, &info, sizeof(Gb28181OperationInfo));
	pthread_mutex_unlock(&kGb28181Mng.oper_mutex);
}

int Gb28181PushAlarm(Gb28181AlarmInfo* alarm_info) {
	CHECK_POINTER(alarm_info, return -1);

	Gb28181MsgSendInfo info;
	snprintf(info.method, sizeof(info.method), "MESSAGE");
	SipNotifyAlarm(kGb28181Mng.sn++, kGb28181Mng.gb28181_info.sip_username, (void*)alarm_info, &info.body);

	pthread_mutex_lock(&kGb28181Mng.send_mutex);
	ListPush(kGb28181Mng.send_list, &info, sizeof(Gb28181MsgSendInfo));
	pthread_mutex_unlock(&kGb28181Mng.send_mutex);
	
	return 0;
}