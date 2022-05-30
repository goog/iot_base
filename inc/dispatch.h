#ifndef __DISPATCH_H
#define __DISPATCH_H


//every thread task can define one module id 
typedef enum
{
    MODULE_ID_UNSPECIFIED = 0,
    MODULE_ID_THREAD1 = 1, // 
    MODULE_ID_ALL,  // multiple targets send id
} dispatch_module_id_t;


typedef enum
{
    DISPATCH_MODE_SEND_RECV = 0,    
    DISPATCH_MODE_SEND_ONLY = 1,
} dispatch_rs_mode_t;



typedef enum
{
    DP_GATEWAY_INVAILD = -1,    // invalid data message
    DP_GATEWAY_PERMIT_JOIN = 1,  // gateway permit join device
    DP_GATEWAY_NOT_PERMIT_JOIN,
} dispatch_msg_type_t;




/**@brief register message channel for two modules, 1、含创建socketpair2、将dispatch线程侧的fd加到一个按照module id维护的整型一维数组里面
* @param[in]  module_id, the module id of thread refs dispatch_module_id_t
* @param[in]  mode, set message mode
* @return >=0: socketpair的一个fd，as input parameter of dispatch_send_msg 
* - -1：fail
**/
int dispatch_register_channel(dispatch_module_id_t module_id, dispatch_rs_mode_t mode);


void dispatch_del_channel(dispatch_module_id_t module_id, int fd);



/**@brief point to point send message (one thread to another)
* @param[in]  fd, to write data to id , returned by dispatch_register_channel
* @param[in]  dest_module_id, destination module id
* @param[in]  msg, message data
* @return 
* - 0 success
* - -1 fail
**/
int dispatch_send_msg(int fd, dispatch_module_id_t dest_module_id, dispatch_msg_type_t msg);




/**@brief send message to all others threads
* @param[in]  fd, 准备写入的套接字描述符，一般是dispatch_register_channel接口的返回值
* @param[in] local_module_id, 调用该接口的本模块线程的id
* @param[in]  msg, 消息数据
* @return 
* - 0 success
* - -1 fail
**/
int dispatch_send_multimsg(int fd, dispatch_module_id_t local_module_id,
                                       dispatch_msg_type_t msg);

/**@brief receive message from socket fd by non-block
* @param[in]  fd, 准备读取的套接字描述符，一般是dispatch_register_channel接口的返回值
* @return messasge type 接收到的消息
* - DP_GATEWAY_INVAILD when fail
**/
dispatch_msg_type_t dispatch_recv_msg(int fd);

/*
dispatch thread function
*/
void *dispatch_thread_func(void *arg);
#endif