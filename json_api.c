#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "cJSON.h"
#include "gw_define.h"
#include "log_util.h"

//#define RET_FAIL -1
//#define RET_OK 0

#define JSON_CONF_FILE_MAX_SIZE (10*1024)  // allow max file size

//获取文件大小
static size_t json_get_file_size(const char *filepath)
{
    struct stat filestat;
    /*check input para*/
    if(NULL == filepath)
    {
        gw_print_log(GW_ERR, "file path null\n");
        return 0;
    }
        
    
    memset(&filestat,0,sizeof(struct stat));
    /*get file information*/
    if(0 == stat(filepath,&filestat))
        return filestat.st_size;
    else
        return 0;
}



int json_read_file(const char *filepath, cJSON **json_conf_root)
{
    /*check input para*/
    if(NULL == filepath)
    {
        gw_print_log(GW_ERR, "file path null\n");
        return RET_FAIL;
    }
    /*get file size*/
    size_t size = json_get_file_size(filepath);
    if(0 == size)
        return RET_FAIL;

    // limit malloc max size
    if(size > JSON_CONF_FILE_MAX_SIZE)
        return RET_FAIL;
        
    /*malloc memory*/
    char *buf = malloc(size+1);
    if(NULL == buf)
        return RET_FAIL;
    memset(buf,0,size+1);
    
    /*read string from file*/
    FILE *fp = fopen(filepath,"r");
    if(fp == NULL)
    {
        gw_print_log(GW_ERR, "file open error\n");
        free(buf);
        return RET_FAIL;
    }
    
    size_t readSize = fread(buf,1,size,fp);
    if(readSize != size)
    {
        /*read error*/
        free(buf);

        fclose(fp);

        gw_print_log(GW_ERR, "read size error\n");
        return RET_FAIL;
    }

    
    fclose(fp);

    // parse json string to json struct for long term use
    *json_conf_root = cJSON_Parse(buf);
    

    if(buf != NULL)
        free(buf);
        
    return RET_OK;
}



int json_get_sub_value(cJSON *json_root, const char *key,
                                 const char *sub_key, char *value_buf, int size)
{

    PARAM_CHECK_NULL(json_root, RET_FAIL);
    PARAM_CHECK_NULL(key, RET_FAIL);
    PARAM_CHECK_NULL(sub_key, RET_FAIL);
    PARAM_CHECK_NULL(value_buf, RET_FAIL);

    if(strlen(sub_key) == 0)
    {
        return RET_FAIL;
    }
    
    cJSON *target_object = NULL, *sub_key_object = NULL;
    target_object = cJSON_GetObjectItem(json_root, key);

    memset(value_buf, 0, size);
    if(target_object)
    {
        sub_key_object = cJSON_GetObjectItem(target_object, sub_key);
        if(sub_key_object)
        {
            //gw_print_log(GW_DEBUG, "** %s\n", sub_key_object->valuestring);
            strncpy(value_buf, sub_key_object->valuestring, size - 1);
            
        }
        else
        {
            return RET_FAIL;
        }
        
    }
    else
    {
        gw_print_log(GW_ERR, "main key do not exist\n");
        return RET_FAIL;
    }

    
    return RET_OK;
}

