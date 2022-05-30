#ifndef __PROTOCOL_CONVERT_H
#define __PROTOCOL_CONVERT_H


// property name pair , used for exchange name
typedef struct property_entry_node
{
    char *p_cloud_property_name;          // cloud property name
    char *p_rex_device_property_name;     // device property name
    struct property_entry_node *next;          // link to next node
} property_entry_node_t;



// protocol conversion share context, include product property name list heads
typedef struct
{
    property_entry_node_t *light_property_list_head;    
    property_entry_node_t *environment_property_list_head;  
    property_entry_node_t *electric_power_property_list_head;  
    property_entry_node_t *security_property_list_head;  
    property_entry_node_t *misc_property_list_head;
} protocol_property_convert_context_t;




/**@brief initialize protocol conversion context, read ini to link list
* @param[in]   context, protocol_property_convert_context_t pointer
* @param[in]   filename, file full path
* @return function return 
* - 0  success
* - -1 failed
**/
int protocol_init(protocol_property_convert_context_t *context, const char *filename);


/**@brief free all protocol conversion link list memory
* @param[in]   context, protocol_property_convert_context_t pointer
**/
void protocol_destroy(protocol_property_convert_context_t *context);




/**@brief format convert from cloud to rex device, adapt to rex sdk, output new json and device endpoint id
* @param[in]  context, protocol share context pointer
* @param[in]   cmd, command from cloud
* @param[in]   dev_type, rex device type
* @param[out]  endpoint_id, store device endpoint id
* @param[out]  json_buf, converted json output
* @param[in]  buf_size, the size of json_buf
* @return function return 
* - 0  success
* - -1 failed
* @note  {"switch1":1} to {"State":"1"}
**/
int protocol_conv_to_device(protocol_property_convert_context_t *context, const char *cmd, int dev_type, 
                                             unsigned char *endpoint_id, char *json_buf, int buf_size);

/**@brief format convert from rex device to cloud, adapt to cloud, output new json
* @param[in]  context, protocol share context pointer
* @param[in]   cmd, command from device
* @param[in]   dev_type, rex device type
* @param[out]  json_buf, converted json output
* @param[in]  buf_size, the size of json_buf
* @return function return 
* - 0  success
* - -1 failed
* @note {"State":"1"} and "EndpointId": "1" to {"switch1":1}
**/


int protocol_conv_to_cloud(protocol_property_convert_context_t *context, const char *cmd, int dev_type, 
                                             char *endpointId, char *json_buf, int buf_size);


#endif
