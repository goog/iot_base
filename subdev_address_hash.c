#include <stdio.h>
#include "subdev_address_hash.h"


#define CLOUD_DEVICE_MATCH 1 //端云子设备topo相匹配，即均存在
#define CLOUD_DEVICE_NOT_MATCH 0 //云端无该子设备，设备端存在该子设备

// RW LOCK SUCCESS 获取读写锁返回成功
#define RW_LOCK_OK 0
// hash header pointer
static hash_entry_t *p_device_hash = NULL;


// note: 由于rex sdk单线程 有些锁的范围进行了优化，如果复用需要自行重新适配
//devide端会调用hash_add和hash_delete接口，记性hash增删操作
//cloud端会调用find_ex以及hash_iterator接口，对hash表记性查找，遍历和修改flag操作
//为了避免cloud端调用subdev_address_hash_iterator时，device端口可能会有设备离网，删除hash数据，就会产生数据不一致导致不确定行为
//增加读写锁（可以多个线程读，只能单个线程写） 用于对uthash的保护 防止data race
static pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;                        


int subdev_address_hash_add(const char *address, unsigned short value)
{
    hash_entry_t *p_entry = NULL;  // store hash find result
    int ret = 0;
    
    if(address == NULL)
    {
        printf("parameter null at %s\n", __FUNCTION__);
        return -1;
    }

    
    HASH_FIND_STR(p_device_hash, address, p_entry);
    if(p_entry == NULL) // key entry no exist
    {
        p_entry = (hash_entry_t *)malloc(sizeof(hash_entry_t));
        if(p_entry == NULL)
        {
            perror("malloc fail");
            return -1;
        }

        memset(p_entry, 0, sizeof(hash_entry_t));
        strncpy(p_entry->address, address, ADDRESS_MAX_LEN - 1);  // at most 16 bytes
        p_entry->type = value;  // set device type

        ret = pthread_rwlock_wrlock(&lock);
        if(ret != RW_LOCK_OK)  //获取写锁失败
        {
            printf("can't get wrlock,err code %d at %s\n", ret, __FUNCTION__);
            return -1;
        }
        
        HASH_ADD_STR(p_device_hash, address, p_entry);  // address is hash field, make it hashable
        pthread_rwlock_unlock(&lock);
        printf("rwlock unlock at %s\n", __FUNCTION__);
    }
    else
    {
        ret = pthread_rwlock_wrlock(&lock);
        if(ret != RW_LOCK_OK)
        {
            printf("can't get wrlock,err code %d at %s\n", ret, __FUNCTION__);
            return -1;
        }
        p_entry->type = value;  // set device type
        pthread_rwlock_unlock(&lock);
        printf("rwlock unlock at %s\n", __FUNCTION__);
    }
    
    //printf("the key %s already existed, rejoin case.\n", address);
    
    
    return 0;
}


// return HASH_ADDR_TYPE_NO_FIND if not find
unsigned short subdev_address_hash_find(const char *key)
{
    int ret = 0;
    hash_entry_t *p_entry = NULL;
    unsigned short type = 0;  // SUB DEV TYPE 1204
    PARAM_CHECK_NULL(key, HASH_ADDR_TYPE_NO_FIND);

    ret = pthread_rwlock_rdlock(&lock);
    if(ret != RW_LOCK_OK)
    {
        printf("can't get rdlock err code %d at %s\n", ret, __FUNCTION__);
        return HASH_ADDR_TYPE_NO_FIND;
    }
    HASH_FIND_STR(p_device_hash, key, p_entry);
    
    
    if(p_entry == NULL)
    {
        printf("hash not found at %s\n", __FUNCTION__);
        type = HASH_ADDR_TYPE_NO_FIND;
    }
    else
    {
        type = p_entry->type;
    }
    
    pthread_rwlock_unlock(&lock);
    printf("rwlock unlock at %s\n", __FUNCTION__);

    return type;
}



int subdev_address_hash_delete(const char *key)
{
    int ret = 0; // 检测加锁的返回值
    
    if(key == NULL)
        return -1;

    hash_entry_t *p_entry = NULL;

    printf("%s begin\n", __FUNCTION__);
    HASH_FIND_STR(p_device_hash, key, p_entry);
    if(p_entry == NULL)
    {
        printf("hash not found at %s\n", __FUNCTION__);
        return -1;
    }

    ret = pthread_rwlock_wrlock(&lock);
    if(ret != RW_LOCK_OK)
    {
        printf("can't acquire write lock err code %d at %s\n", ret, __FUNCTION__);
        return -1;
    }
    
    HASH_DEL(p_device_hash, p_entry);
    pthread_rwlock_unlock(&lock);
    printf("rwlock unlock at %s\n", __FUNCTION__);
    if(p_entry != NULL)
        free(p_entry);

    return 0;
}


void subdev_address_hash_delete_all()
{
    hash_entry_t *current_element, *tmp;

    if(pthread_rwlock_wrlock(&lock) != RW_LOCK_OK)
    {
        perror("cant get wrlock");
        return;    
    }
    HASH_ITER(hh, p_device_hash, current_element, tmp) 
    {
        HASH_DEL(p_device_hash, current_element);  /* delete it (users advances to next) */
        free(current_element);             /* free it */
    }

    pthread_rwlock_unlock(&lock);
    return;
}


unsigned short subdev_address_hash_find_ex(const char *key)
{   
    hash_entry_t *p_entry = NULL;
    unsigned short type = 0;
    int ret = 0;
    PARAM_CHECK_NULL(key, HASH_ADDR_TYPE_NO_FIND);

    // 目前设置为读锁，应为这个flag只是云端单线程使用
    ret = pthread_rwlock_rdlock(&lock);
    if(ret != RW_LOCK_OK)
    {
        printf("can't get rdlock, err code %d at %s\n", ret, __FUNCTION__);
        return HASH_ADDR_TYPE_NO_FIND;
    }
    
    HASH_FIND_STR(p_device_hash, key, p_entry);
    if(p_entry == NULL)
    {
        printf("hash not found at %s\n", __FUNCTION__);
        type = HASH_ADDR_TYPE_NO_FIND;
    }
    else
    {
        //如果找到，即该子设备在云端列表和设备端hash列表中，则将标志为置1，后续subdev_address_hash_iterator会使用到
        p_entry->flag = CLOUD_DEVICE_MATCH;
        type = p_entry->type;
    }
    
    pthread_rwlock_unlock(&lock);
    printf("rwlock unlock at %s\n", __FUNCTION__);
    
    return type;
}

void subdev_address_hash_iterator(int (*topo_leave_device_cb)(char *))
{
    hash_entry_t *current_element, *tmp;
    int ret = 0;
    // 目前设置为读锁，应为这个flag只是云端单线程使用
    ret = pthread_rwlock_rdlock(&lock);
    if(ret != RW_LOCK_OK)
    {
        printf("can't get rdlock, err code %d at %s\n", ret, __FUNCTION__);
        return;
    }
    
    HASH_ITER(hh,p_device_hash,current_element,tmp)
    {
        //printf("now device name %s flag %d at %s\n",current_element->address,current_element->flag, __FUNCTION__);
        if(current_element->flag == CLOUD_DEVICE_MATCH)
        {
            //printf("flag is %d\n",current_element->flag);
            current_element->flag = CLOUD_DEVICE_NOT_MATCH;
        }
        else
        {
            //调用回调函数插入注销信息到设备端;
            //printf("flag is %d\n",current_element->flag);
            topo_leave_device_cb(current_element->address);
        }
    }

    pthread_rwlock_unlock(&lock);
    printf("rwlock unlock at %s\n", __FUNCTION__);
}



int subdev_address_hash_get_keys(unsigned short type, char list[][ADDRESS_MAX_LEN], int rows)
{
    hash_entry_t *current_element, *tmp;
    int ret = 0;
    int i = 0;
    
    ret = pthread_rwlock_rdlock(&lock);
    if(ret != RW_LOCK_OK)
    {
        //用回标准打印函数
        printf("[subdev_address_hash_get_keys]can't get rdlock, err code %d at %s\n", ret, __FUNCTION__);
        return RET_FAIL;
    }
    
    HASH_ITER(hh,p_device_hash,current_element,tmp)
    {
        
        if(current_element->type == type)
        {
            printf("[subdev_address_hash_get_keys] get device type %d, [%d]address is %s\n", type, i, current_element->address);
            
            strncpy(list[i], current_element->address, ADDRESS_MAX_LEN-1);
            i++;

            if(i == rows)  //地址数组已填充满
            {
                printf("[subdev_address_hash_get_keys]array is full\n");
                break;
            }
                
        }   
    }

    
    //ret = HASH_COUNT(p_device_hash);
    pthread_rwlock_unlock(&lock);
    
    
    return i;
}

