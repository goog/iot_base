#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "protocol_convert.h"
#include "gw_define.h"


#define PROTOCOL_RET_OK 0
#define PROTOCOL_RET_FAIL (-1)
#define PROTOCOL_INI_LINE_BUF_LEN 100   // read ini one line buffer length
#define PROTOCOL_NUMBER_BUF_LEN 10    // store int string
#define PROTOCOL_INI_SECTION_LEN 15
#define PROTOCOL_MAX_ENDPOINT_ID_SIZE (3+1)          // buf len is 4
#define PROTOCOL_MAX_ENDPOINT_ID 255    // 最大多路号
#define PROTOCOL_DEFAULT_ENDPOINT_ID 1  // default endpoint id is 1
#define PROTOCOL_MIN_DEVICE_TYPE 1001  // device type range
#define PROTOCOL_MAX_DEVICE_TYPE 9999
#define PROTOCOL_MAX_PROPERTY_LEN 30  // 插入链表时属性名长度限制



// shoud config same with ini
#define LIGHT_SECTION_NAME "light"   
#define EPOWER_SECTION_NAME "electric_power"  
#define CURTAIN_SECTION_NAME "environment"    
#define SECURITY_SECTION_NAME "security"  
#define MISC_SECTION_NAME "misc"

#define PROTOCOL_ERR(format, args...) printf("[protocol,%s:%d] "format, __FUNCTION__, __LINE__, ##args)


static int protocol_read_ini_to_list(protocol_property_convert_context_t *context, const char *filename);



int protocol_init(protocol_property_convert_context_t *context, const char *filename)
{
    int ret = PROTOCOL_RET_FAIL;
    
    PARAM_CHECK_NULL(context, PROTOCOL_RET_FAIL);      

    context->light_property_list_head = NULL;
    context->electric_power_property_list_head = NULL;
    context->environment_property_list_head = NULL;
    context->security_property_list_head = NULL;
    context->misc_property_list_head = NULL;
    ret = protocol_read_ini_to_list(context, filename);
    return ret;
}



void protocol_destroy(protocol_property_convert_context_t *context)
{      

    property_entry_node_t *node = NULL;
    property_entry_node_t *next_node = NULL;

    if(context == NULL)
    {
        PROTOCOL_ERR("parameter is null\n");
        return;
    }
    
    
    node = context->light_property_list_head;
    while(node)
    {
        next_node = node->next; // keep next node pointer
        SAFE_FREE_MEM(node->p_cloud_property_name);
        SAFE_FREE_MEM(node->p_rex_device_property_name);
        SAFE_FREE_MEM(node);

        node = next_node;
    }
    context->light_property_list_head = NULL; 


    node = context->electric_power_property_list_head;
    while(node)
    {
        next_node = node->next; // keep next node pointer
        SAFE_FREE_MEM(node->p_cloud_property_name);
        SAFE_FREE_MEM(node->p_rex_device_property_name);
        SAFE_FREE_MEM(node);

        node = next_node;
    }
    context->electric_power_property_list_head = NULL; 
    

    node = context->environment_property_list_head;
    while(node)
    {
        next_node = node->next; // keep next node pointer
        SAFE_FREE_MEM(node->p_cloud_property_name);
        SAFE_FREE_MEM(node->p_rex_device_property_name);
        SAFE_FREE_MEM(node);

        node = next_node;
    }
    context->environment_property_list_head = NULL; 

}


/**@brief insert name strings to list 将ini里面的属性名称对 插入链表
* @param[in/out]  list_head, the address of one list head
* @param[in]  cloud_property_name, property name from cloud
* @param[in]  rex_property_name, property name need by rex sdk
* @return function return 
* - 0  success
* - -1 failed
**/
static int protocol_insert_list(property_entry_node_t **list_head, char *cloud_property_name, char *rex_property_name)
{
    property_entry_node_t *node = NULL;

    char *cloud_name_buf = NULL;  // cloud endpoint property name
    char *rex_name_buf = NULL;
    int cloud_name_len = 0;
    int rex_name_len = 0;

    PARAM_CHECK_NULL(list_head, PROTOCOL_RET_FAIL);   
    PARAM_CHECK_NULL(cloud_property_name, PROTOCOL_RET_FAIL);
    PARAM_CHECK_NULL(rex_property_name, PROTOCOL_RET_FAIL);

    cloud_name_len = strlen(cloud_property_name);
    rex_name_len = strlen(rex_property_name);

	// string length check
    if(cloud_name_len > PROTOCOL_MAX_PROPERTY_LEN || rex_name_len > PROTOCOL_MAX_PROPERTY_LEN)
    {
        PROTOCOL_ERR("property name length too long\n");
        return PROTOCOL_RET_FAIL;
    }

    if(cloud_name_len <= 0 || rex_name_len <= 0)
    {
        PROTOCOL_ERR("property name length invalid\n");
        return PROTOCOL_RET_FAIL;
    }

    node = malloc(sizeof(property_entry_node_t));
    PARAM_CHECK_NULL(node, PROTOCOL_RET_FAIL);

    cloud_name_buf = malloc(cloud_name_len+1);
    if(cloud_name_buf == NULL)
    {
        PROTOCOL_ERR("malloc fail\n");

        free(node);
        return PROTOCOL_RET_FAIL;
    }

    memset(cloud_name_buf, 0, cloud_name_len+1);
    strncpy(cloud_name_buf, cloud_property_name, cloud_name_len);
    node->p_cloud_property_name = cloud_name_buf;

    rex_name_buf = malloc(rex_name_len+1);
    if(rex_name_buf == NULL)
    {
        PROTOCOL_ERR("malloc fail\n");

        free(node);
        free(cloud_name_buf);
        return PROTOCOL_RET_FAIL;
    }

    memset(rex_name_buf, 0, rex_name_len+1);
    strncpy(rex_name_buf, rex_property_name, rex_name_len);
    node->p_rex_device_property_name = rex_name_buf;  // write value to node

    node->next = NULL;

    // insert node to link list
    if(*list_head == NULL)  //判断链表当前是否还没有节点
    {
        *list_head = node;  // list head 指向链表的第一个节点
    }
    else                // insert node after head
    {
        node->next = (*list_head)->next;
        (*list_head)->next = node;
    }

    return PROTOCOL_RET_OK;
}

