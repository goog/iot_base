#include "log_util.h"

int main()
{

    gw_log_setting(GW_INFO, "/var/log/test.log", 1024*1024);
    gw_log(GW_DEBUG, "log begin");
    gw_log(GW_INFO, "log device online");

}