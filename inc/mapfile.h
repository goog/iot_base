#ifndef __MAPFILE_H
#define __MAPFILE_H
#include <stdint.h>
#include <pthread.h>


/**
 * @file mapfile.h
 * @author jay
 * @date 2020.12.31
 * @brief mapfile module usage for quick reference.
 */

#define MAPFILE_OK 0
#define MAPFILE_FAIL (-1)



#define TABLE_COL_SIZE    (384)  // hash index range
#define TABLE_ROW_SIZE    (2)    // zipper length

#define ITEM_MAX_KEY_LEN     96 /* The max key length for key-value item */
#define ITEM_MAX_VAL_LEN     128 /* The max value length for key-value item */
#define ITEM_MAX_LEN         sizeof(kv_item_t)

//#define KV_FILE_NAME         "linkkit_kv.bin"

#define kv_err(...)               do{printf("[common] %s ", __FUNCTION__);printf(__VA_ARGS__);printf("\r\n");}while(0)

typedef struct kv {
    char key[ITEM_MAX_KEY_LEN];
    uint8_t value[ITEM_MAX_VAL_LEN];
    int value_len;
} kv_item_t;

typedef struct kv_file_s {
    const char *filename;
    pthread_mutex_t lock;
} kv_file_t;





/**@brief hashmap create mapfile context(kv_file_t struct)
* @param[in]  filename, hashmap file full path
* @return function return mapfile context pointer
**/

kv_file_t *mapfile_kv_open(const char *filename);


/**@brief hashmap file add key-value
* @param[in]  file, file context pointer. returned by mapfile_kv_open function
* @param[in]  key, key string pointer. string length stricted by ITEM_MAX_KEY_LEN
* @param[in]  val, value buffer pointer
* @param[in]  len, buffer size
* @return function return 
* - 0  success
* - -1 failed
**/
//int mapfile_kv_set(kv_file_t *file, const char *key, const void *val, int len, int sync);
int mapfile_kv_set(kv_file_t *file, const char *key, const void *val, int value_len);


/**@brief hashmap file get value by key
* @param[in]  file, file context pointer, returned by mapfile_kv_open
* @param[in]  key, key string pointer. string length stricted by macro define
* @param[out]  val, value buffer pointer
* @param[out]  buffer_len, value's memory size
* @return function return 
* - 0  success
* - -1 failed
**/
int mapfile_kv_get(kv_file_t *file, const char *key, void *val, int *buffer_len);


/**@brief hashmap file delete key-value
* @param[in]  file, file context pointer,returned by mapfile_kv_open function
* @param[in]  key, key string pointer. string length stricted by macro define
* @return function return 
* - 0  success
* - -1 failed
**/

int mapfile_kv_del(kv_file_t *file, const char *key);


/**@brief get kv item
* @param[in]  fd, return by open() system call
* @param[out]  buf, the buffer to store two kv_item_t
* @param[in]  location, the colume number
* @return function return 
* - 0  success
* - -1 failed
**/
int mapfile_kv_item_get(int fd, void *buf, int location);

/**@brief empty kv_file
* @param[in]  hash_kv, file, file context pointer,returned by mapfile_kv_open function 
* - 0  empty success
* - -1 empty failed
**/
int mapfile_kv_emp(kv_file_t * hash_kv);

#endif
