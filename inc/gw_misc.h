#ifndef __GW_MISC_H
#define __GW_MISC_H

#include <semaphore.h>


typedef enum
{
    GATEWAY_UNBLOCKED = 0, // 网关处于非阻塞态
    GATEWAY_BLOCKED = 1,   // 网关处于阻塞态（处在ota 或恢复出厂过程中）
} gw_block_status_t;

/**@brief 带超时的信号量等待函数            
* @param[in]  sem, 信号量指针
* @param[in]  msecs, 超时参数单位毫秒
* @return  0 成功获取到信号量 
* - 0  success，超时时间内，获取到信号量
* - -1 failed， 超时时间内没获取到信号量
**/
int sem_timedwait_ms(sem_t *sem, long msecs);



/**@brief 
查询网关是否处于block状态，如果处于阻塞状态，网关应用程序不应该执行一些可能重启的重大操作（比如重启/OTA升级/恢复出厂设置）
* @return
* - 0  非阻塞态，网关可以执行任何操作
* - 1  阻塞态， 网关不能执行一些重大操作
**/
gw_block_status_t gw_misc_get_block_status();


/**@brief 设置网关当前状态（阻塞态/非阻塞态），
一般和gw_misc_check_block_status配套使用，先判断处于非阻塞态，再执行set操作
* @param[in]  flag, 网关状态设置，0 非阻塞，1，阻塞
**/
void gw_misc_set_block_status(gw_block_status_t flag);


/**@brief 删除文件夹及下面的所有文件 内部使用rm -rf
* @param[in]  dir, 目录路径，长度不能超过240
* @return function return
* - 0  success
* - -1 fail
* @note 调用前请确认路径dir是正确的
**/
void gw_rm_all_files(const char *dir);
#endif