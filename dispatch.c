#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <errno.h>
#include "dispatch.h"
#include "gw_define.h"


#define MAX_ARRAY_LEN MODULE_ID_ALL      //注册的FD句柄最大数目
#define DISPATCH_INVALID_FD 0            // 无效fd标志，也代表该模块id没有注册通道
#define DISPATCH_SELECT_TIMEOUT 10000    // 10000us=10MS
#define FD_ARRAY_COLUMNS 2
#define FD_COLS_INDEX 0    //二维数组第一列 store fd
#define MODE_COLS_INDEX 1  //二维数组第二列 store message mode

// 根据目标模块id寻址返回结果的数据结构定义
typedef struct
{
    int cnt;  // 寻址到的FD个数，单点发送这里个数是1，群发则是除本模块外，其余所有注册模块个数
    int fds[MAX_ARRAY_LEN]; // 存储寻址到的FD句柄数组
} dispatch_fd_info_t;


typedef struct
{
    dispatch_module_id_t src_module_id;  // 消息的源module id
    dispatch_module_id_t dest_module_id;  //消息的目标module id
    dispatch_msg_type_t msg;    // msg 类型
} message_t;

// default fd is invalid and mode is send_recv
//数组的行是标识对应哪个模块ID，第一列保存dispatch使用的fd，第二列保存消息模式
static int fd_array[MAX_ARRAY_LEN][FD_ARRAY_COLUMNS] = {0};  // 维护模块id和对应的fd的二维全局数组
static pthread_mutex_t dispatch_array_lock = PTHREAD_MUTEX_INITIALIZER;  // 保护fd_array数组

/**@brief 通过目的地址module id寻址返回转发需要使用的fd 可以是一个或多个
* @param[in]  dest_module_id, 目标线程模块的id
* @param[in] src_module_id, 源线程模块的id 此参数用于在群发时排除自己 点对点是此参数没有使用
* @param[in/out] fd_info, dispatch_fd_info_t 指针 用于存放转发时需要使用的套接字列表
* - >= 0 success
* - -1 fail
**/
static int dispatch_get_fd(dispatch_module_id_t dest_module_id, dispatch_module_id_t src_module_id,
                                   dispatch_fd_info_t *fd_info);



int dispatch_register_channel(dispatch_module_id_t module_id, dispatch_rs_mode_t mode)
{
    int ret = RET_FAIL;
    int fd[2] = {-1, -1};
    
    if(module_id >= MODULE_ID_ALL || module_id < MODULE_ID_MAIN_CONSOLE)
    {
        return RET_FAIL;
    }

    // already registered, return
    if(fd_array[module_id][FD_COLS_INDEX] != DISPATCH_INVALID_FD)
    {
        printf("socketpair existed\n");
        return RET_FAIL;
    }
    
    
    ret = socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fd);
    if(ret == RET_FAIL)
    {
        perror("create socketpair fail");
        return RET_FAIL;
    }

    pthread_mutex_lock(&dispatch_array_lock);
    fd_array[module_id][FD_COLS_INDEX] = fd[0]; // dispatch 端的fd
    fd_array[module_id][MODE_COLS_INDEX] = mode; //configure whether need receive message at local end
    pthread_mutex_unlock(&dispatch_array_lock);

    return fd[1];   // socketpair中对应本线程端的fd,associated with the thread 
}


void dispatch_del_channel(dispatch_module_id_t module_id, int fd)
{
    int dispatch_fd = DISPATCH_INVALID_FD;
    
    if(module_id >= MODULE_ID_ALL || module_id < MODULE_ID_MAIN_CONSOLE)
    {
        return;
    }

    pthread_mutex_lock(&dispatch_array_lock);
    dispatch_fd = fd_array[module_id][FD_COLS_INDEX];
    if(dispatch_fd != DISPATCH_INVALID_FD)
    {
        close(dispatch_fd);
        fd_array[module_id][FD_COLS_INDEX] = DISPATCH_INVALID_FD;
        fd_array[module_id][MODE_COLS_INDEX] = DISPATCH_MODE_SEND_RECV;
    }
    pthread_mutex_unlock(&dispatch_array_lock);

    if(fd > 0)
    {
        close(fd);
    }
}



/**@brief socket发送数据通用接口
* @param[in]  fd, 准备发送数据的套接字对象
* @param[in]  msg, 消息指针
* @param[in]  msgLength, 消息字节长度
* @return
* - 0 success
* - -1 fail
**/
static int dispatch_send(int fd, void *msg, size_t msg_len)
{
    if(fd < 0 || msg == NULL || msg_len <= 0)
    {
        return RET_FAIL;
    }
    
    ssize_t nWrite;
    int count = 0;

    
    do
    {
        if(count > 0)
        {
            usleep(2000);    // 第一次发送失败后延时2ms
        }
        
        //向目标Fd写入消息，采用异步非阻塞方式
        nWrite = send(fd, msg, msg_len, MSG_DONTWAIT | MSG_NOSIGNAL);
        count++;
   
    } while ((nWrite == -1 && errno == EINTR) && count < 3);  // 如果因为中断原因导致发送失败，则重发


    // nwrite = -1, or 由于系统资源不足 socket peer被关闭 发不完整
    if((size_t)nWrite != msg_len)
    {
        perror("dispatch send fail");
        printf("fd %d send fail ret %zd\n", fd, nWrite);
        return RET_FAIL;
    }

    //printf("dispatch_send success\n");
    return RET_OK;
}



// 是给其他非dispatch线程使用的
int dispatch_send_msg(int fd, dispatch_module_id_t dest_module_id, dispatch_msg_type_t msg)
{
    message_t message = {0};
    message.src_module_id = MODULE_ID_UNSPECIFIED;
    message.dest_module_id = dest_module_id;
    message.msg = msg;
    
    return dispatch_send(fd, &message, sizeof(message));
}


int dispatch_send_multimsg(int fd, dispatch_module_id_t local_module_id, dispatch_msg_type_t msg)
{
    message_t message = {0};
    message.src_module_id = local_module_id;
    message.dest_module_id = MODULE_ID_ALL;
    message.msg = msg;
    
    return dispatch_send(fd, &message, sizeof(message));
}


dispatch_msg_type_t dispatch_recv_msg(int local_fd)
{
    int ret = RET_FAIL;
    
    fd_set rfds;
    struct timeval tv;
    int readlen = 0;
    

    dispatch_msg_type_t msg = DP_GATEWAY_INVAILD;
    int msg_size = sizeof(msg);

    FD_ZERO(&rfds);
    FD_SET(local_fd, &rfds);
    
    tv.tv_sec = 0;
    tv.tv_usec = 15000; //15ms timeout
    ret = select(local_fd+1, &rfds, NULL, NULL, &tv);

    if(ret > 0)
    {
        readlen = recv(local_fd, &msg, msg_size, MSG_DONTWAIT);
        if(readlen == msg_size)
        {
            printf("msg content is %d\n", msg);    
        }
        else
        {
            printf("recv message bytes not correct\n"); 
            msg = DP_GATEWAY_INVAILD;
        }
    }
    else if(ret == -1)
    {
        perror("select()");
    }

    return msg;    
}

/**@brief 根据目标ID进行消息转发 包含寻址和发送消息两个动作
* @param[in]  message, 消息指针
* @return
* - >= 0 success
* - -1 fail
**/
static int dispatch_transfer_msg(message_t *message)
{
    int i = 0;
    int ret = RET_FAIL;
    
    dispatch_module_id_t dest_id = message->dest_module_id;
    dispatch_module_id_t src_id = message->src_module_id;
    dispatch_fd_info_t info = {0};
    ret = dispatch_get_fd(dest_id, src_id, &info);  //寻址
    if(ret == RET_FAIL)
    {
        printf("dispatch_get_fd failed at %s\n", __FUNCTION__);
        return ret;
    }
    //printf("fd list len %d at %s\n", info.cnt, __FUNCTION__);
    for(i = 0; i < info.cnt; i++) //根据寻址结果进行发送操作，可能有多次发送操作
    {
        //nwrite = send(info.fds[i], &(message->msg), sizeof(dispatch_msg_type_t), MSG_NOSIGNAL);
        ret = dispatch_send(info.fds[i], &(message->msg), sizeof(dispatch_msg_type_t));
        if(ret == RET_FAIL)
        {
            printf("dispatch_send fail\n"); 
        }
        
    }
    
    return ret;
}



 
/**@brief 创建可读的套接字集合
* @param[in/out]  read_fds, 读集合指针
* @return fd >0: fd套接字数组里的套接字最大值，为后续select接口使用
* - -1 FAIL
**/
static int build_fd_sets(fd_set *read_fds)
{
    int i;
    int tmp_fd = DISPATCH_INVALID_FD;
    int max_fd = -1;

    if(read_fds == NULL)
    {
        return -1;
    }
    
    FD_ZERO(read_fds);
    for(i = MODULE_ID_MAIN_CONSOLE; i < MAX_ARRAY_LEN; i++)  //遍历FD数组，将每个FD都set到读套接字里去
    {
        tmp_fd = fd_array[i][FD_COLS_INDEX];
        if(tmp_fd != DISPATCH_INVALID_FD)
        {
            FD_SET(tmp_fd, read_fds);

            if(tmp_fd > max_fd)      // 作用是就是为了返回一个fd最大值 select函数需要知道
            {
                max_fd = tmp_fd;
            }
        }
    }

    return max_fd;
}


static int dispatch_get_fd(dispatch_module_id_t dest_module_id, dispatch_module_id_t src_module_id,
                                   dispatch_fd_info_t *fd_info)
{
    dispatch_fd_info_t info = {0};
    int fd = DISPATCH_INVALID_FD;
    int i = 0;
    int j = 0;

    //printf("querying id %d\n", dest_module_id);
    if(dest_module_id > MODULE_ID_ALL || dest_module_id < MODULE_ID_MAIN_CONSOLE)
    {
        printf("querying id %d not in range\n", dest_module_id);
        return RET_FAIL;
    }
    
    if(dest_module_id != MODULE_ID_ALL)  // 单点发送
    {
        // 寻址fd
        fd = fd_array[dest_module_id][FD_COLS_INDEX];
        printf("fd %d\n", fd);
        if(fd != DISPATCH_INVALID_FD)
        {
            info.fds[0] = fd;
            info.cnt = 1;
        }
        else
        {
            printf("**id %d does not register channel\n", dest_module_id);
            return RET_FAIL;
        }
    }
    else          //群发消息
    {
        // 在有效模块id范围里遍历
        dispatch_rs_mode_t mode;
        for(i = MODULE_ID_MAIN_CONSOLE; i < MODULE_ID_ALL; i++)
        {
            fd = fd_array[i][FD_COLS_INDEX];
            mode = fd_array[i][MODE_COLS_INDEX];
            // 1、fd valid，2、 id is not src module id，3、目标模块注册时设置为可接收模式，才把该FD保存到寻址队列里
            if(fd != DISPATCH_INVALID_FD && i != src_module_id && mode == DISPATCH_MODE_SEND_RECV)
            {
                info.fds[j++] = fd;
                info.cnt++;
            }
        }

        if(0 == info.cnt)          //群发没寻址到目标模块FD
        {
            return RET_FAIL;
        }
    }

    memcpy(fd_info, &info, sizeof(info));
    return RET_OK;
}



void *dispatch_thread_func(void *arg)
{
    int ret = RET_FAIL;
    fd_set read_fds; // 可读的集合
    int activity=-1; // select结果
    int maxfd = 0;
    int i = 0;
    
    int dispatch_fd = 0; //socketpair在dispatch侧的fd
    int readlen = 0;  // 接收的数据长度
    struct timeval tv;
    
    message_t message = {0};
    int message_size = sizeof(message);
    printf("%s begin\n", __FUNCTION__);
    
    // 让其他线程启动好都注册完通道
    // 需要确保其他转发通道都注册好，否则消息可能会丢失
    sleep(2);
    
    
    while(1)
    {
        
        maxfd = build_fd_sets(&read_fds);
        if(maxfd < 0)    //没有可读套接字
        {
            // no socket add to readset
            usleep(100000);  // sleep 100ms
            continue;
        }
        
        tv.tv_sec = 0;
        tv.tv_usec = DISPATCH_SELECT_TIMEOUT;

        activity = select(maxfd + 1, &read_fds, NULL, NULL, &tv);
        if(activity > 0) //有数据可读
        {
            // find the readable socket
            for(i = MODULE_ID_MAIN_CONSOLE; i < MODULE_ID_ALL; i++)
            {
                // dispatch_fd dispatch线程侧套接字
                dispatch_fd = fd_array[i][FD_COLS_INDEX];
                if(dispatch_fd == DISPATCH_INVALID_FD)
                {
                    continue;
                }
                
                // 查看该socket是否可读
                if(FD_ISSET(dispatch_fd, &read_fds))
                {
                    //printf("fd %d is readable\n", dispatch_fd);
                    memset(&message, 0, message_size);
                    
                    readlen = recv(dispatch_fd, &message, message_size, MSG_DONTWAIT);
                    if(readlen == message_size)
                    {
                        printf("have recv some message\n");
                        ret = dispatch_transfer_msg(&message);
                        if(ret == RET_FAIL)
                        {
                            printf("dispatch_transfer_msg fail\n");
                        }
                    }
                }
            }
                
        }
        else
        {
            if(activity == -1)
            {
                perror("select()");
            }
                
        }       
    }
    

}

