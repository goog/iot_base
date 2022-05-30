#include <stdio.h>
#include <time.h>
#include "gw_misc.h"

static volatile int gateway_in_blocked = GATEWAY_UNBLOCKED;


/**@brief 带超时的信号量等待函数            
* @param[in]  sem, 信号量指针
* @param[in]  msecs, 超时参数单位毫秒
* @return  0 成功获取到信号量 
* - 0  success，超时时间内，获取到信号量
* - -1 failed， 超时时间内没获取到信号量
**/
int sem_timedwait_ms(sem_t *sem, long msecs)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long secs = msecs/1000;  // most case is 0
    msecs = msecs%1000;    //秒范围内的毫秒

    long add = 0;
    msecs = msecs*1000*1000 + ts.tv_nsec;  // store the nanosecond
    add = msecs / (1000*1000*1000);     // 纳秒进位的秒
    ts.tv_sec += (add + secs);
    ts.tv_nsec = msecs%(1000*1000*1000);

    return sem_timedwait(sem, &ts);
}


//返回网关是否处于block状态
gw_block_status_t gw_misc_get_block_status()
{
    return gateway_in_blocked;
}


// 设置网关当前为block状态，后续网关收到重大操作指令（比如恢复出厂/OTA /重启），网关不应该执行
void gw_misc_set_block_status(gw_block_status_t flag)
{
    gateway_in_blocked = flag;
}




void gw_rm_all_files(const char *dir)    
{
    int number = 0;
    char cmd[256] = {0};
    size_t cmd_size = sizeof(cmd);
    
    number = snprintf(cmd, cmd_size, "rm -rf %s", dir);
    if(number > 0 && number < cmd_size)    // 字符完整写入
    {
        system(cmd);
    }

}