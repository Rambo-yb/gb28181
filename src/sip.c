#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include "sip.h"
#include "rtp.h"
#include "check_common.h"
#include "gb28181_common.h"
#include "libmxml4/mxml.h"

int SipMsgCatalog(void* st, mxml_node_t* tree, char** resp, void* cb) {
	SipCatalogInfo* _st = (SipCatalogInfo*)st;

	LOG_INFO("%s", _st->device_id);

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
    mxmlNewText(mxmlNewElement(item, "DeviceID"), space, _st->device_id);
    mxmlNewText(mxmlNewElement(item, "Name"), space, _st->name);
    mxmlNewText(mxmlNewElement(item, "Manufacturer"), space, _st->manu_facturer);
    mxmlNewText(mxmlNewElement(item, "Model"), space, _st->model);
    mxmlNewText(mxmlNewElement(item, "Owner"), space, _st->owner);
    mxmlNewText(mxmlNewElement(item, "CivilCode"), space, _st->civil_code);
    mxmlNewText(mxmlNewElement(item, "Address"), space, _st->address);
    mxmlNewInteger(mxmlNewElement(item, "Parental"), _st->parental);
    mxmlNewText(mxmlNewElement(item, "ParentID"), space, _st->parent_id);
    mxmlNewInteger(mxmlNewElement(item, "RegisterWay"), _st->register_way);
    mxmlNewInteger(mxmlNewElement(item, "Secrecy"), _st->secrecy);
    mxmlNewText(mxmlNewElement(item, "Status"), space, "ON");

    *resp = mxmlSaveAllocString(xml, options);
	
    mxmlDelete(xml);
    mxmlOptionsDelete(options);

	return 0;
}

int SipMsgDeviceStatus(void* st, mxml_node_t* tree, char** resp, void* cb) {
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
	return 0;
}

int SipMsgDeviceInfo(void* st, mxml_node_t* tree, char** resp, void* cb) {
	SipCatalogInfo* _st = (SipCatalogInfo*)st;

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
    mxmlNewText(mxmlNewElement(response, "DeviceName"), space, _st->name);
    mxmlNewText(mxmlNewElement(response, "Manufacturer"), space, _st->manu_facturer);
    mxmlNewText(mxmlNewElement(response, "Model"), space, _st->model);
    mxmlNewText(mxmlNewElement(response, "Firmware"), space, "V1.0.0");
    mxmlNewText(mxmlNewElement(response, "Channel"), space, "1");

    *resp = mxmlSaveAllocString(xml, options);

    mxmlDelete(xml);
    mxmlOptionsDelete(options);
	return 0;
}

int SipMsgRecordInfo(void* st, mxml_node_t* tree, char** resp, void* cb) {
	CHECK_POINTER(cb, return -1);

	bool space = false;
    mxml_node_t* sn = mxmlFindElement(tree, tree, "SN", NULL, NULL, MXML_DESCEND_ALL);
    mxml_node_t* device_id = mxmlFindElement(tree, tree, "DeviceID", NULL, NULL, MXML_DESCEND_ALL);

	Gb28181QueryRecordInfo info;
	memset(&info, 0, sizeof(Gb28181QueryRecordInfo));
	const char* start_time = mxmlGetText(mxmlFindElement(tree, tree, "StartTime", NULL, NULL, MXML_DESCEND_ALL), &space);
	CHECK_POINTER(start_time, return -1);

	struct tm tm_in;
	memset(&tm_in, 0, sizeof(struct tm));
	sscanf(start_time, "%04d-%02d-%02dT%02d:%02d:%02d", &tm_in.tm_year, &tm_in.tm_mon, &tm_in.tm_mday, &tm_in.tm_hour, &tm_in.tm_min, &tm_in.tm_sec);
    info.start_time = mktime(&tm_in);

	const char* end_time = mxmlGetText(mxmlFindElement(tree, tree, "EndTime", NULL, NULL, MXML_DESCEND_ALL), &space);
	CHECK_POINTER(end_time, return -1);

	memset(&tm_in, 0, sizeof(struct tm));
	sscanf(end_time, "%04d-%02d-%02dT%02d:%02d:%02d", &tm_in.tm_year, &tm_in.tm_mon, &tm_in.tm_mday, &tm_in.tm_hour, &tm_in.tm_min, &tm_in.tm_sec);
    info.end_time = mktime(&tm_in);

	int ret = ((Gb28181OperationControlCb)cb)(GB28181_CONTORL_QUERY_RECORD, &info, sizeof(Gb28181QueryRecordInfo));
	CHECK_LT(ret, 0, return -1);

    mxml_options_t* options = mxmlOptionsNew();
    mxml_node_t *xml = mxmlNewXML("1.0");
    mxml_node_t *response = mxmlNewElement(xml, "Response");
    mxmlNewText(mxmlNewElement(response, "CmdType"), space, "RecordInfo");
    mxmlNewText(mxmlNewElement(response, "SN"), space, mxmlGetText(sn, &space));
    mxmlNewText(mxmlNewElement(response, "DeviceID"), space, mxmlGetText(device_id, &space));
    mxmlNewText(mxmlNewElement(response, "Name"), space, "Camera");
    mxmlNewInteger(mxmlNewElement(response, "SumNum"), info.num);

    mxml_node_t *record_list = mxmlNewElement(response, "RecordList");
	char buff[20] = {0};
	snprintf(buff, sizeof(buff), "%d", info.num);
    mxmlElementSetAttr(record_list, "Num", buff);

	for(int i = 0; i < info.num; i++) {
		mxml_node_t *item = mxmlNewElement(record_list, "Item");
		mxmlNewText(mxmlNewElement(item, "DeviceID"), space, mxmlGetText(device_id, &space));
		mxmlNewText(mxmlNewElement(item, "Name"), space, info.info[i].name);
		mxmlNewText(mxmlNewElement(item, "FilePath"), space, info.info[i].path);
		mxmlNewText(mxmlNewElement(item, "Address"), space, "address_1");

		struct tm* tm_out = gmtime((time_t*)&info.info[i].start_time);
		snprintf(buff, sizeof(buff), "%04d-%02d-%02dT%02d:%02d:%02d", 
			tm_out->tm_year+1900, tm_out->tm_mon+1, tm_out->tm_mday, tm_out->tm_hour, tm_out->tm_min, tm_out->tm_sec);
		mxmlNewText(mxmlNewElement(item, "StartTime"), space, buff);

		tm_out = gmtime((time_t*)&info.info[i].end_time);
		snprintf(buff, sizeof(buff), "%04d-%02d-%02dT%02d:%02d:%02d", 
			tm_out->tm_year+1900, tm_out->tm_mon+1, tm_out->tm_mday, tm_out->tm_hour, tm_out->tm_min, tm_out->tm_sec);
		mxmlNewText(mxmlNewElement(item, "EndTime"), space, buff);

		mxmlNewText(mxmlNewElement(item, "Secrecy"), space, mxmlGetText(mxmlFindElement(tree, tree, "Secrecy", NULL, NULL, MXML_DESCEND_ALL), &space));
		mxmlNewText(mxmlNewElement(item, "Type"), space, "time");
		mxmlNewText(mxmlNewElement(item, "RecorderID"), space, mxmlGetText(mxmlFindElement(tree, tree, "RecorderID", NULL, NULL, MXML_DESCEND_ALL), &space));
	}

    *resp = mxmlSaveAllocString(xml, options);

end:
	if (info.info != NULL) {
		free(info.info);
	}
    mxmlDelete(xml);
    mxmlOptionsDelete(options);
	return 0;
}

static int SipAlarmMethodPaser(const char* method) {
	if (atoi(method) == 0) {
		return 0x7f;
	}

	if (strlen(method) == 1) {
		return 0x1 << (atoi(method) - 1);
	}

	int m = 0;
	for (int i = 0; i < strlen(method); i++) {
		m |= 0x01 << (method[i] - '1');
	}
	return m;
}

int SipMsgAlarm(void* st, mxml_node_t* tree, char** resp, void* cb) {
	bool space = false;
	Gb28181AlarmSubscription info;
	memset(&info, 0, sizeof(Gb28181AlarmSubscription));
	const char* priority = mxmlGetText(mxmlFindElement(tree, tree, "StartAlarmPriority", NULL, NULL, MXML_DESCEND_ALL), &space);
	CHECK_POINTER(priority, return -1);
	info.start_priority = atoi(priority);

	priority = mxmlGetText(mxmlFindElement(tree, tree, "EndAlarmPriority", NULL, NULL, MXML_DESCEND_ALL), &space);
	CHECK_POINTER(priority, return -1);
	info.end_priority = atoi(priority);

	const char* method = mxmlGetText(mxmlFindElement(tree, tree, "AlarmMethod", NULL, NULL, MXML_DESCEND_ALL), &space);
	CHECK_POINTER(priority, return -1);
	info.method_mask = SipAlarmMethodPaser(method);

	const char* start_time = mxmlGetText(mxmlFindElement(tree, tree, "StartTime", NULL, NULL, MXML_DESCEND_ALL), &space);
	CHECK_POINTER(start_time, return -1);

	struct tm tm_in;
	memset(&tm_in, 0, sizeof(struct tm));
	sscanf(start_time, "%04d-%02d-%02dT%02d:%02d:%02d", &tm_in.tm_year, &tm_in.tm_mon, &tm_in.tm_mday, &tm_in.tm_hour, &tm_in.tm_min, &tm_in.tm_sec);
    info.start_time = mktime(&tm_in);

	const char* end_time = mxmlGetText(mxmlFindElement(tree, tree, "EndTime", NULL, NULL, MXML_DESCEND_ALL), &space);
	CHECK_POINTER(end_time, return -1);

	memset(&tm_in, 0, sizeof(struct tm));
	sscanf(end_time, "%04d-%02d-%02dT%02d:%02d:%02d", &tm_in.tm_year, &tm_in.tm_mon, &tm_in.tm_mday, &tm_in.tm_hour, &tm_in.tm_min, &tm_in.tm_sec);
    info.end_time = mktime(&tm_in);

	int ret = ((Gb28181OperationControlCb)cb)(GB28181_CONTORL_ALARM_SUBSCRIPTION, &info, sizeof(Gb28181AlarmSubscription));
	CHECK_LT(ret, 0, return -1);

	return 0;
}

#define SIP_CMD_HEAD (0xa5)

#define SIP_CMD_SCAN_START (0x89)
#define SIP_CMD_SCAN_SPEED (0x8A)

#define SIP_CMD_AUXILIARY_CTRL_ON (0x8C)
#define SIP_CMD_AUXILIARY_CTRL_OFF (0x8D)

int SipMsgPtzCmd(void* st, mxml_node_t* tree, char** resp, void* cb) {
	CHECK_POINTER(cb, return -1);

	bool space = false;
    const char* arg = mxmlGetText(mxmlFindElement(tree, tree, "PTZCmd", NULL, NULL, MXML_DESCEND_ALL), &space);
	CHECK_POINTER(arg, return -1);

    unsigned int cmd[8] = {0};
    sscanf(arg, "%02x%02x%02x%02x%02x%02x%02x%02x", 
        &cmd[0], &cmd[1], &cmd[2], &cmd[3], &cmd[4], &cmd[5], &cmd[6], &cmd[7]);
    CHECK_EQ(cmd[0], SIP_CMD_HEAD, return -1);

    if (cmd[3] & (0x01 << 6)) {
		Gb28181FocusCtrl focus_ctrl;
		memset(&focus_ctrl, 0, sizeof(Gb28181FocusCtrl));
		focus_ctrl.type = GB28181_FOCUS_CTRL_STOP;
        if (cmd[3] & (0x01 << 0)) {
            focus_ctrl.type = GB28181_FOCUS_CTRL_FAR;
        } else if (cmd[3] & (0x01 << 1)) {
            focus_ctrl.type = GB28181_FOCUS_CTRL_NEAR;
        }
		focus_ctrl.speed = cmd[4];

		Gb28181IrisCtrl iris_ctrl;
		memset(&iris_ctrl, 0, sizeof(Gb28181IrisCtrl));
        iris_ctrl.type = GB28181_IRIS_CTRL_STOP;
        if (cmd[3] & (0x01 << 2)) {
            iris_ctrl.type = GB28181_IRIS_CTRL_IN;
        } else if (cmd[3] & (0x01 << 3)) {
            iris_ctrl.type = GB28181_IRIS_CTRL_OUT;
        }
		iris_ctrl.speed = cmd[5];
		
		((Gb28181OperationControlCb)cb)(GB28181_CONTORL_FOCUS, &focus_ctrl, sizeof(Gb28181FocusCtrl));
		((Gb28181OperationControlCb)cb)(GB28181_CONTORL_IRIS, &iris_ctrl, sizeof(Gb28181IrisCtrl));
    } else if (cmd[3] & (0x01 << 7)) {
        if (cmd[3] == SIP_CMD_AUXILIARY_CTRL_ON || cmd[3] == SIP_CMD_AUXILIARY_CTRL_OFF) {
			LOG_WRN("TODO: Auxiliart ctrl on/off");
        } else{
			Gb28181PtzCtrl ptz_ctrl;
			memset(&ptz_ctrl, 0, sizeof(Gb28181PtzCtrl));
			if (cmd[3] == SIP_CMD_SCAN_START || cmd[3] == SIP_CMD_SCAN_SPEED) {
				int ptz_type = (cmd[3] == SIP_CMD_SCAN_START) ? GB28181_PTZ_CTRL_SCAN_START: GB28181_PTZ_CTRL_SCAN_SPEED;
				if (ptz_type == GB28181_PTZ_CTRL_SCAN_START) {
					ptz_type += cmd[5];
				}
				int scan_num = cmd[5];
				int data = (cmd[5] & 0xff) | ((cmd[6] & 0xf0) << 8);
			} else {
				ptz_ctrl.type = GB28181_PTZ_CTRL_PRESET_ADD + (cmd[3] & 0x0f) - 1;
				ptz_ctrl.preset_num = cmd[5];
				ptz_ctrl.grop_num = cmd[4];
				if (ptz_ctrl.type == GB28181_PTZ_CTRL_CRUISE_SPEED) {
					ptz_ctrl.speed = (cmd[5] & 0xff) | ((cmd[6] & 0xf0) << 8);
				} else if (ptz_ctrl.type == GB28181_PTZ_CTRL_CRUISE_SPEED) {
					ptz_ctrl.time = (cmd[5] & 0xff) | ((cmd[6] & 0xf0) << 8);
				}
			}
			((Gb28181OperationControlCb)cb)(GB28181_CONTORL_PTZ, &ptz_ctrl, sizeof(Gb28181PtzCtrl));
		}
    } else {
		Gb28181PtzCtrl ptz_ctrl;
		memset(&ptz_ctrl, 0, sizeof(Gb28181PtzCtrl));
        ptz_ctrl.type = GB28181_PTZ_CTRL_STOP;
        if(cmd[3] & (0x01 << 0)) {
            if (cmd[3] & (0x01 << 2)) {
                ptz_ctrl.type = GB28181_PTZ_CTRL_RIGHT_DOWN;
            } else if (cmd[3] & (0x01 << 3)){
                ptz_ctrl.type = GB28181_PTZ_CTRL_RIGHT_UP;
            } else {
                ptz_ctrl.type = GB28181_PTZ_CTRL_RIGHT;
            }
        } else if (cmd[3] & (0x01 << 1)) {
            if (cmd[3] & (0x01 << 2)) {
                ptz_ctrl.type = GB28181_PTZ_CTRL_LEFT_DOWN;
            } else if (cmd[3] & (0x01 << 3)){
                ptz_ctrl.type = GB28181_PTZ_CTRL_LEFT_UP;
            } else {
                ptz_ctrl.type = GB28181_PTZ_CTRL_LEFT;
            }
        } else if (cmd[3] & (0x01 << 2)) {
            ptz_ctrl.type = GB28181_PTZ_CTRL_DOWN;
        } else if (cmd[3] & (0x01 << 3)) {
            ptz_ctrl.type = GB28181_PTZ_CTRL_UP;
        }
        
		Gb28181ZoomCtrl zoom_ctrl;
		memset(&zoom_ctrl, 0, sizeof(Gb28181ZoomCtrl));
        zoom_ctrl.type = GB28181_ZOOM_CTRL_STOP;
        if (cmd[3] & (0x01) << 4) {
            zoom_ctrl.type = GB28181_ZOOM_CTRL_IN;
        } else if (cmd[3] & (0x01) << 5) {
            zoom_ctrl.type = GB28181_ZOOM_CTRL_OUT;
        }

		((Gb28181OperationControlCb)cb)(GB28181_CONTORL_PTZ, &ptz_ctrl, sizeof(Gb28181PtzCtrl));
		((Gb28181OperationControlCb)cb)(GB28181_CONTORL_ZOOM, &zoom_ctrl, sizeof(Gb28181ZoomCtrl));
    }

	return 0;
}

int SipMsgRecordCmd(void* st, mxml_node_t* tree, char** resp, void* cb) {
	CHECK_POINTER(cb, return -1);

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

	return 0;
}

int SipMsgGuardCmd(void* st, mxml_node_t* tree, char** resp, void* cb) {
	CHECK_POINTER(cb, return -1);

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
	return 0;
}


int SipNotifyKeepalive(int sn, char* device_id, char** resp) {
	int space = 0;
	mxml_options_t* options = mxmlOptionsNew();
	mxml_node_t *xml = mxmlNewXML("1.0");
	mxml_node_t *response = mxmlNewElement(xml, "Notify");
	mxmlNewText(mxmlNewElement(response, "CmdType"), space, "Keepalive");
	mxmlNewInteger(mxmlNewElement(response, "SN"), sn);
	mxmlNewText(mxmlNewElement(response, "DeviceID"), space, device_id);
	mxmlNewText(mxmlNewElement(response, "Result"), space, "OK");

	*resp = mxmlSaveAllocString(xml, options);

    mxmlDelete(xml);
    mxmlOptionsDelete(options);
	return 0;
}

int SipNotifyMediaStatus(int sn, char* device_id, char** resp) {
	int space = 0;
	mxml_options_t* options = mxmlOptionsNew();
	mxml_node_t *xml = mxmlNewXML("1.0");
	mxml_node_t *response = mxmlNewElement(xml, "Notify");
	mxmlNewText(mxmlNewElement(response, "CmdType"), space, "MediaStatus");
	mxmlNewInteger(mxmlNewElement(response, "SN"), sn);
	mxmlNewText(mxmlNewElement(response, "DeviceID"), space, device_id);
	mxmlNewText(mxmlNewElement(response, "Result"), space, "OK");
	mxmlNewInteger(mxmlNewElement(response, "NotifyType"), 121);

	*resp = mxmlSaveAllocString(xml, options);

    mxmlDelete(xml);
    mxmlOptionsDelete(options);
	return 0;
}

int SipNotifyAlarm(int sn, char* device_id, void* st, char** resp) {
	Gb28181AlarmInfo* _st = (Gb28181AlarmInfo*)st;

	int space = 0;
	mxml_options_t* options = mxmlOptionsNew();
	mxml_node_t *xml = mxmlNewXML("1.0");
	mxml_node_t *response = mxmlNewElement(xml, "Notify");
	mxmlNewText(mxmlNewElement(response, "CmdType"), space, "Alarm");
	mxmlNewInteger(mxmlNewElement(response, "SN"), sn);
	mxmlNewText(mxmlNewElement(response, "DeviceID"), space, device_id);
	mxmlNewInteger(mxmlNewElement(response, "AlarmPriority"), _st->priority+1);
	mxmlNewInteger(mxmlNewElement(response, "AlarmMethod"), _st->method+1);
	
	struct tm* _tm = gmtime((time_t*)&_st->time);
	char buff[20] = {0};
	snprintf(buff, sizeof(buff), "%04d-%02d-%02dT%02d:%02d:%02d", 
		_tm->tm_year+1900, _tm->tm_mon+1, _tm->tm_mday, _tm->tm_hour, _tm->tm_min, _tm->tm_sec);
	mxmlNewText(mxmlNewElement(response, "AlarmTime"), space, buff);
	mxmlNewText(mxmlNewElement(response, "AlarmDescription"), space, _st->description);

	if (_st->type != 0) {
	    mxml_node_t *type = mxmlNewElement(response, "Info");
		mxmlNewInteger(mxmlNewElement(type, "AlarmType"), _st->type);

		if(_st->type_param != 0) {
			mxml_node_t *type_param = mxmlNewElement(type, "AlarmTypeParam");
			mxmlNewInteger(mxmlNewElement(type_param, "EventType"), _st->type_param);
		}
	}

	*resp = mxmlSaveAllocString(xml, options);

    mxmlDelete(xml);
    mxmlOptionsDelete(options);
	return 0;

}