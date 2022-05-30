#ifndef __SUBDEV_ADDRESS_HASH_H
#define __SUBDEV_ADDRESS_HASH_H

#include "uthash.h"
#include "gw_define.h"
//#define HASH_ADDRESS_LEN 17
#define HASH_ADDR_TYPE_NO_FIND (65535)  //hash 查找失败返回值


typedef struct 
{
    char address[ADDRESS_MAX_LEN];  // hash key, address string
    unsigned short type;             // device type, value associated with address(key)
    unsigned char flag;        // flag use for topo compare
    UT_hash_handle hh;         /* makes this structure hashable */
} hash_entry_t;



/**@brief hash add key/value pair, key is address
* @param[in]  address, device address , is hash key
* @param[in]  value, value is device type for the hash table
* @return function return 
* - 0  success
* - -1 failed
* @note  if first time, it will initialize hash head pointer(the global variable)
**/
int subdev_address_hash_add(const char *address, unsigned short value);


/**@brief find hash value by key
* @param[in]  key, key string pointer
* @return function return hash value device type, if not found then return HASH_ADDR_TYPE_NO_FIND 65535
* @note rex device type max value 9999 (四位)
**/
unsigned short subdev_address_hash_find(const char *key);

/**@brief traverse the hash table
* @param[in]  function pointer
* @return void
* @note 遍历设备端子设备列表接口，找到flag为1的子设备，说明端云均存在该子设备，找到flag为0的设备，调用回调函数注销设备
**/
void subdev_address_hash_iterator(int (*topo_leave_device_cb)(char *));


/**@brief find hash value by key
* @param[in]  key, key string pointer
* @return function return hash value device type, if not found then return HASH_ADDR_TYPE_NO_FIND 65535
* @note If it is not found, it returns return HASH_ADDR_TYPE_NO_FIND 65535, and if it is found, set flag 1
* @note 此函数与unsigned short subdev_address_hash_find(const char *key)函数的区别在于p_entry->flag = 1
* @note 为云端对比topo查找设备端子设备列表专用接口
**/
unsigned short subdev_address_hash_find_ex(const char *key);

/**@brief hash delete one key-value entry
* @param[in]  key, key string eg.device mac address
* @return function whether do delete operate
* - 0 do delete
* - -1 did not find key, do nothing
* @note 一般不用检查返回值，除非需要确认是否有删除动作
**/
int subdev_address_hash_delete(const char *key);


/**@brief delete all key and value, and free memory
**/
void subdev_address_hash_delete_all();


/**@brief 获取指定类型的子设备address到二维数组
* @param[in]  type, device type 1203
* @param[out]  list, 2d array
* @param[in]  row, the row number of list
* @return 
* - the count of keys if succ
* - -1 fail
* @note 获取哈希表里面的所有key
**/
int subdev_address_hash_get_keys(unsigned short type, char list[][ADDRESS_MAX_LEN], int rows);
#endif