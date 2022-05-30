#ifndef __LOG_UTIL_H
#define __LOG_UTIL_H


#define GW_LOG_MAX_FILE_NAME 128


typedef enum
{
    GW_DEBUG=0,
    GW_INFO=1,
    GW_ERR=2,
    GW_FATAL=3,
    GW_NONE=4,
} local_log_level_t;


void log_printf(int level, const char *func, int line, const char *fmt, ...);



/**@brief print log to stdout and file
* @param[in]  level, log level
* @param[in]  fmt, format string
* @param[in]  ..., variadic function arguments
* @note gw_log(GW_DEBUG, "log begin")
**/
#define gw_log(level, fmt, ...) log_printf(level, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)


/**@brief to set log module configure options
* @param[in]  level, gateway log level 
* @param[in]  filename,  the full path of log file
* @param[in]  max_log_size, max file size
* @return function return 
* - 0  success
* - -1 failed
**/
int gw_log_setting(local_log_level_t level, const char *filename, int max_log_size);



/**@brief stop log output
**/
void gw_disable_log();

#endif
