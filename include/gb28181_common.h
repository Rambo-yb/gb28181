#ifndef __GB28181_COMMON_H__
#define __GB28181_COMMON_H__

typedef enum {
	GB28181_CONTORL_PLAY_STREAM = 1,		// 播放实时视频, Gb28181PlayStream
	GB28181_CONTORL_PLAY_RECORD,			// 播放录像、下载录像, Gb28181PlayRecord
	GB28181_CONTORL_QUERY_RECORD,			// 查询录像, Gb28181QueryRecordInfo
	GB28181_CONTORL_PTZ,					// 云台, Gb28181PtzCtrl
	GB28181_CONTORL_ZOOM,					// 变倍, Gb28181ZoomCtrl
	GB28181_CONTORL_IRIS,					// 光圈, Gb28181IrisCtrl
	GB28181_CONTORL_FOCUS,					// 聚焦, Gb28181FocusCtrl
	GB28181_CONTORL_RECORDING,				// 录制, Gb28181RecordingCtrl
	GB28181_CONTORL_ALARM_SUBSCRIPTION,		// 报警订阅, Gb28181AlarmSubscription
}Gb28181ContorlType;
typedef int (*Gb28181OperationControlCb)(int, void*, int); 

typedef enum {
	GB28181_PLAY_VIDEO_START,	// 开始
	GB28181_PLAY_VIDEO_STOP,	// 结束

	GB28181_PLAY_VIDEO_PAUSE,	// 暂停
	GB28181_PLAY_VIDEO_PLAY,	// 播放
	GB28181_PLAY_VIDEO_SCALE,	// 倍数
}Gb28181PlayVideoType;

typedef struct {
	// in
	int type;				// Gb28181PlayVideoType
}Gb28181PlayStream;


typedef struct {
	// in
	int type;			// Gb28181PlayVideoType
	int start_time;		// 播放周期，开始时间
	int end_time;		// 播放周期，结束时间
	float scale;		// 倍数
}Gb28181PlayRecord;


typedef enum {
	GB28181_RECORD_TIME,	// 定时录像
	GB28181_RECORD_ALRAM,	// 报警录像
	GB28181_RECORD_MANUAL,	// 手动录像
	GB28181_RECORD_ALL,		// 所有
}Gb28181RecordType;

typedef struct {
	char name[64];		// 文件名
	char path[32];		// 文件路径
	int start_time;		// 开始时间
	int end_time;		// 结束时间
	int type;			// 录像类型
}Gb28181RecordInfo;

typedef struct {
	// in
	int start_time;		// 开始时间
	int end_time;		// 结束时间
	int type;			// 查询类型

	// out
	int num;					// 录像数量
	Gb28181RecordInfo* info;	// 使用malloc开辟对应大小的空间，内部释放
}Gb28181QueryRecordInfo;


typedef enum Gb28181PtzCtrlType {
    GB28181_PTZ_CTRL_UP,			// 上
    GB28181_PTZ_CTRL_DOWN,			// 下
    GB28181_PTZ_CTRL_LEFT,			// 左
    GB28181_PTZ_CTRL_RIGHT,			// 右
    GB28181_PTZ_CTRL_LEFT_UP,		// 左上
    GB28181_PTZ_CTRL_LEFT_DOWN,		// 左下
    GB28181_PTZ_CTRL_RIGHT_UP,		// 右上
    GB28181_PTZ_CTRL_RIGHT_DOWN,	// 右下

    GB28181_PTZ_CTRL_PRESET_ADD,	// 设置预置点
    GB28181_PTZ_CTRL_PRESET_USE,	// 调用预置点
    GB28181_PTZ_CTRL_PRESET_DEL,	// 删除预置点

    GB28181_PTZ_CTRL_CRUISE_ADD,	// 加入巡航点
    GB28181_PTZ_CTRL_CRUISE_DEL,	// 删除巡航点
    GB28181_PTZ_CTRL_CRUISE_SPEED,	// 设置巡航速度
    GB28181_PTZ_CTRL_CRUISE_TIME,	// 设置巡航停留时间
    GB28181_PTZ_CTRL_CRUISE_START,	// 开始巡航

    GB28181_PTZ_CTRL_SCAN_START,	// 开始自动扫描
    GB28181_PTZ_CTRL_SCAN_LEFT,		// 设置自动扫描左边界
    GB28181_PTZ_CTRL_SCAN_RIGHT,	// 设置自动扫描右边界
    GB28181_PTZ_CTRL_SCAN_SPEED,	// 设置自动扫描速度

    GB28181_PTZ_CTRL_STOP,			// 停止云台操作
}Gb28181PtzCtrlType;

typedef struct {
	int type;
	int preset_num;		// 预置位号
	int grop_num;		// 巡航组号
	int speed;			// 设置扇扫和巡航速度时
	int time;			// 设置巡航点停留时间
}Gb28181PtzCtrl;


typedef enum Gb28181ZoomCtrlType {
    GB28181_ZOOM_CTRL_IN,		// 放大
    GB28181_ZOOM_CTRL_OUT,		// 缩小
    GB28181_ZOOM_CTRL_STOP,		// 停止
}Gb28181ZoomCtrlType;

typedef struct {
	int type;
}Gb28181ZoomCtrl;


typedef enum Gb28181IrisCtrlType {
    GB28181_IRIS_CTRL_IN,		// 放大
    GB28181_IRIS_CTRL_OUT,		// 缩小
    GB28181_IRIS_CTRL_STOP,		// 停止
}Gb28181IrisCtrlType;

typedef struct {
	int type;
	int speed;		// 光圈速度
}Gb28181IrisCtrl;


typedef enum Gb28181FocusCtrlType {
    GB28181_FOCUS_CTRL_FAR,		// 远
    GB28181_FOCUS_CTRL_NEAR,	// 近
    GB28181_FOCUS_CTRL_STOP,	// 停止
}Gb28181FocusCtrlType;

typedef struct {
	int type;
	int speed;		// 聚焦速度
}Gb28181FocusCtrl;


typedef enum {
	GB28181_RECORDING_CTRL_START,	// 开始
	GB28181_RECORDING_CTRL_STOP,		// 停止
}Gb28181RecordingCtrlType;

typedef struct {
	int type;
}Gb28181RecordingCtrl;
 
typedef enum {
	GB28181_ALARM_PRIORITY_ONE_LVL,		// 一级警情
	GB28181_ALARM_PRIORITY_TWO_LVL,		// 二级警情
	GB28181_ALARM_PRIORITY_THIRD_LVL,	// 三级警情
	GB28181_ALARM_PRIORITY_FOur_LVL,	// 四级警情
}Gb28181AlarmPriorityType;

typedef enum {
	GB28181_ALARM_METHOD_TELEPHONE,			// 电话报警
	GB28181_ALARM_METHOD_DEVICE,			// 设备报警，报警类型：Gb28181DeviceAlarmType
	GB28181_ALARM_METHOD_SHORT_MSG,			// 短信报警
	GB28181_ALARM_METHOD_GPS,				// GPS报警
	GB28181_ALARM_METHOD_VIDEO,				// 视频报警，报警类型：Gb28181VideoAlarmType
	GB28181_ALARM_METHOD_BREAKDOWN,			// 故障报警，报警类型：Gb28181BreakDownAlarmType
	GB28181_ALARM_METHOD_OTHER,				// 其他报警
}Gb28181AlarmMethodType;

typedef struct {
	// in
	int start_priority;		// 报警起始级别
	int end_priority;		// 报警终止级别
	int method_mask;		// 报警方式
	int start_time;			// 报警发生起止时间
	int end_time;			// 报警发生起止时间
}Gb28181AlarmSubscription;


typedef enum {
	GB28181_DEVICE_ALARM_VIDEO_LOSS = 1,			// 视频丢失
	GB28181_DEVICE_ALARM_PREVENT_DISASSEMBLE,		// 设备防拆
	GB28181_DEVICE_ALARM_STORAGE_SPACE_FULL,		// 存储空间满
	GB28181_DEVICE_ALARM_DEVICE_HIGH_TEMPERATURE,	// 设备高温
	GB28181_DEVICE_ALARM_DEVICE_LOW_TEMPERATURE,	// 设备低温
}Gb28181DeviceAlarmType;

typedef enum {
	GB28181_VIDEO_ALARM_MANUAL_VIDEO = 1,			// 人工视频
	GB28181_VIDEO_ALARM_TARGET_DETECTION,			// 运动目标检测
	GB28181_VIDEO_ALARM_RESIDUE_DETECTION,			// 遗留物检测
	GB28181_VIDEO_ALARM_OBJECT_REMOVE_DETECTION,	// 物体移除检测
	GB28181_VIDEO_ALARM_TRIP_WIRE,					// 绊线检测
	GB28181_VIDEO_ALARM_INTRUSION_DETECTION,		// 入侵检测
	GB28181_VIDEO_ALARM_RETROGRADE_DETECTION,		// 逆行检测
	GB28181_VIDEO_ALARM_WANDERING_DETECTION,		// 徘徊检测
	GB28181_VIDEO_ALARM_FLOW_STATISTICS,			// 流量统计
	GB28181_VIDEO_ALARM_DENSITY_DETECTION,			// 密度检测
	GB28181_VIDEO_ALARM_VIDEO_ABNORMAL,				// 视频异常检测
	GB28181_VIDEO_ALARM_FAST_MOTION,				// 快速移动
}Gb28181VideoAlarmType;

typedef enum {
	GB28181_BREAKDOWN_ALARM_DISK = 1,				// 磁盘故障
	GB28181_BREAKDOWN_ALARM_FAN,					// 风扇故障
}Gb28181BreakDownAlarmType;

typedef enum {
	GB28181_ALARM_PARAM_IN = 1,		// 进入区域
	GB28181_ALARM_PARAM_OUT,		// 离开区域
}Gb28181AlarmParamType;

typedef struct {
	int priority;				// 报警级别，Gb28181AlarmPriorityType
	int method;					// 报警方式，Gb28181AlarmMethodType
	int time;					// 时间
	char description[512];		// 描述
	int type;					// 类型，根据报警方式对报警类型赋值，无具体类型可传0
	int type_param;				// 参数，Gb28181AlarmParamType，无需参数可传0
}Gb28181AlarmInfo;

#endif
