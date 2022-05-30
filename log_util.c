#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include "log_util.h"
//#include <sys/statvfs.h>
#include "gw_define.h"



#define TIME_BUF_SIZE 64    
#define BUF_SIZE 224    
#define WRITE_BUF_SIZE 1024

#define OTHER_STRING_LEN 40 
#define FUNCTION_STRING_LEN 56


//#define LOG_WRITE_TO_STDOUT 0  //日志写到标准输出
//#define LOG_WRITE_TO_FILE 1    //日志写到文件
   

typedef struct
{
    volatile local_log_level_t log_level;   // gateway local log level
    FILE *fp;  // log file pointer
    char *buffer;   //data buffer to write log file
    int max_size; // file max size limit
    int file_size;  // current log file size
} gw_log_setting_t;



// write log file mutex
static pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

static gw_log_setting_t log_setting = {0};





/**@brief to set log module configure options
* @param[in]  level, gateway log level 
* @param[in]  filename,  the full path of log file
* @param[in]  max_log_size, max file size
* @return function return 
* - 0  success
* - -1 failed
**/
int gw_log_setting(local_log_level_t level, const char *filename, int max_log_size)
{
    
    struct stat st;
    FILE *fp = NULL;
    if(filename == NULL)
    {
        return RET_FAIL;
    }

    if(strlen(filename) > GW_LOG_MAX_FILE_NAME-1)
    {
        return RET_FAIL;
    }

    //gw_set_local_level(level);
    log_setting.log_level = level;  // set log level
    

    if(access(filename, F_OK) != 0)
    {
        fp = fopen(filename, "w");  // create file if not exist
        if(fp == NULL)
        {
            perror("create file fail ");
            return RET_FAIL;
        }
        else
        {
            fclose(fp);
        }
            
    }
    

    log_setting.fp = fopen(filename, "r+");
    if(log_setting.fp == NULL)
    {
        printf("Error: %s\n", strerror(errno));
        return RET_FAIL;
    }

    //get file size
    stat(filename, &st);
    log_setting.file_size = st.st_size > 0 ? st.st_size:0;
    log_setting.max_size = max_log_size;
    
    log_setting.buffer = (char *)malloc(WRITE_BUF_SIZE);
    if(log_setting.buffer == NULL)
    {
        printf("malloc fail\n");
        return RET_FAIL;
    }
    
    return RET_OK;
}




void gw_disable_log()
{
    
    log_setting.log_level = GW_NONE;
}

/**@brief full pwrite
* @param[in]  fd, file descriptor
* @param[in]  buf, data buffer
* @param[in]  count, the size to write
* @param[in]  offset, file absolute offset
* @return function return the number of bytes written, -1 if failed
**/
static int full_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    size_t total = 0;  // already written bytes
    size_t ret = 0;

    const char *p = NULL;
    for(p = buf; total < count;)
    {
        ret = pwrite(fd, p+total, count-total, offset+total);
        if(ret == -1)
        {
            if(errno != EINTR)
                return -1;
        }
        else
        {
            total += ret;  //partial write
        }
    }

    return total;
}

/**@brief log printf api with full parameters with function name and line number
* @param[in]  level, log level
* @param[in]  func, function name
* @param[in]  line, line number
* @param[in]  fmt, format string limit to max 128
* @param[in]  ..., variable arguments
**/
void log_printf(int level, const char *func, int line, const char *fmt, ...)
{
    static int log_offset = 0;  // log file offset, to write position
    va_list ap;
    
    char *p_level = NULL;  // log level string
    char time_buf[TIME_BUF_SIZE] = {0};  // time string buffer
    char buf[BUF_SIZE] = {0};  
    time_t now;
    struct timeval tv;
    struct tm tm_buf;
    long int file_size = 0;
    FILE *fp = log_setting.fp; // get file pointer
    int written_bytes = 0;
    int byte_number = 0;
    
    
    
    if(fmt == NULL)
        return;

    if(level < log_setting.log_level)
        return;

    if(strlen(func) >= FUNCTION_STRING_LEN)
    {
        printf("function too long\n");
        return;
    }

    if(strlen(fmt) >= BUF_SIZE - OTHER_STRING_LEN - FUNCTION_STRING_LEN)
    {
        printf("fmt too long\n");
        return;
    }
    

    if(level == GW_DEBUG)
        p_level = "DEBUG";
    else if(level == GW_INFO)
        p_level = "INFO";
    else if(level == GW_ERR)
        p_level = "ERR";
    else if(level == GW_FATAL)
        p_level = "FATAL";
    else
    {
        //printf("log level parameter error\n");
        return;
    }



    va_start(ap, fmt);

    gettimeofday(&tv, NULL);
    now = tv.tv_sec;  // get the second
    
    //strftime(time_buf, TIME_BUF_SIZE, "%Y/%m/%d %H:%M:%S", localtime_r(&now, &tm_buf));
    localtime_r(&now, &tm_buf);

    snprintf(time_buf, TIME_BUF_SIZE, "%02d-%02d %02d:%02d:%02d.%06ld",
             tm_buf.tm_mon+1, tm_buf.tm_mday, tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, tv.tv_usec);

    snprintf(buf, BUF_SIZE, "[%s](%s) %s(%d):%s\n", p_level, time_buf, func, line, fmt);

    //ouput log to stdout
    vprintf(buf, ap);
    
    
    if(fp != NULL)
    {
        pthread_mutex_lock(&file_mutex); 
        //printf("log_offset %d, orignal file size %d\n", log_offset, log_setting.file_size);

        written_bytes = vsnprintf(log_setting.buffer, WRITE_BUF_SIZE, buf, ap);
        if(written_bytes > 0 && written_bytes < WRITE_BUF_SIZE)
        {
            
            if(log_setting.file_size + log_offset + written_bytes >= log_setting.max_size)
            {
                //printf("rewind\n");
                log_offset = 0;
                log_setting.file_size = 0;  // set file_size to 0
            }

            
            byte_number = full_pwrite(fileno(fp), log_setting.buffer, written_bytes, log_setting.file_size+log_offset);
            if(byte_number == -1)
            {
                printf("pwrite fail,Error: %s\n", strerror(errno));
            }  
            else
            {
                log_offset += byte_number;
            }

            
        }
        else
        {
            printf("vsnprintf error %d\n", written_bytes);
        }

        pthread_mutex_unlock(&file_mutex);
    }
    
    va_end(ap);

}


