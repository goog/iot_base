#ifndef __JSON_API_H
#define __JSON_API_H
#include "cJSON.h"



/**@brief read json file to cJSON object
* @param[in]  filepath, json file full path
* @param[in/out]  json_conf_root, *json_conf_root used to store pointer value
* @return function return 
* - 0  success
* - -1 failed
*/
int json_read_file(const char *filepath, cJSON **json_conf_root);


int json_get_sub_value(cJSON *json_root, const char *key,
                              const char *sub_key, char *value_buf, int size);

#endif