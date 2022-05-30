#ifndef __GW_DEFINE_H
#define __GW_DEFINE_H

// COO share header file

#define ZIGBEE_COO_ADDR "0000000000000000"
#define ADDRESS_MAX_LEN 17 //address的最大长度 
#define ENDPOINTID_MAX_LEN 4 //EndpointId的最大长度
#define GPIO_DEV	"/dev/gpio"
#define  CABLE_STATUS_CONNECT 1  //gateway cable connect 
#define  CABLE_STATUS_DISCONNECT 0  //gateway cable disconnect
#define RET_FAIL (-1)  // return fail value
#define RET_OK (0)
#define VER_BUF_MAX_LEN 96
#define NEW_LOG_FILE "/etc/gatewayConf/fenda_log.txt"  // offline log 转存目录


#define CLOUD_SERVICE_ID_LEN 20       //阿里服务id最大长度
#define REPORT_LONG_JSON_LEN (400)    // long json buffer size


//#define TRUE  1
//#define FALSE (!TRUE)
/** FALSE */
#ifndef FALSE
#define FALSE 0
#endif
/** TRUE */
#ifndef TRUE
#define TRUE (!FALSE)
#endif


typedef enum
{
    REX_DEVICE_TYPE_ELECTRIC_POWER = 10,  //电能管理设备类，比如智能插座
    REX_DEVICE_TYPE_LIGHTING   = 11,  //照明设备类，
    REX_DEVICE_TYPE_SECUTIRY   = 12,  //安防设备类，比如智能锁具、安防传感器等
    REX_DEVICE_TYPE_ENVIROMENT = 13,  //环境设备，比如温度传感器、电动窗帘、空调温控等
    REX_DEVICE_TYPE_OTHERS     = 90,  //其它设备，主要是非标设备，比如红外转发器、晾衣机等
    REX_DEVICE_TYPE_UNKNOWN    = 99,  //未知设备
}rex_device_type_t;


//zigbee子设备的设备类型, 与入网回调上报的类型一致
typedef enum
{
    REX_LOCK_DEVICE_TYPE = 1203,        //门锁
    REX_DOOR_SENSOR_DEVICE_TYPE = 1204, //门磁类型
    REX_INFRARED_CONTROLLER_TYPE = 9001,   // 红外转发器
    REX_ONEWAY_SENSE_DEVICE_TYPE = 1313,   //一路情景面板
    REX_FOURWAY_SENSE_DEVICE_TYPE = 1362,  //四路情景面板
}rex_subdev_type_t;  

#define PARAM_CHECK_NULL(ptr, ret)     \
do                                   \
{                                    \
    if(ptr == NULL)                    \
    {                                  \
        printf("parameter is null at %s\n", __FUNCTION__); \
        return ret;                  \
    }                                \
} while(0)   

#define SAFE_FREE_MEM(ptr)            \
    do                                   \
    {                                    \
        if(ptr != NULL)                  \
        {                                \
            free(ptr);                   \
            ptr = NULL;                  \
        }                                \
    } while(0)     
#endif
