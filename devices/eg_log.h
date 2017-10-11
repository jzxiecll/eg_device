
#ifndef __EG_LOG_H__
#define __EG_LOG_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <wmstdio.h>


#ifdef __cplusplus
extern "C"
{
#endif

#define eg_log_printf wmprintf
#define eg_log_fflush wmstdio_flush


#define EG_LOG_LEVEL  EG_LOG_LEVEL_DEBUG
#define EG_P       eg_log_printf

#define EG_LOG_LEVEL_FATAL      (0)
#define EG_LOG_LEVEL_NOTICE     (1)
#define EG_LOG_LEVEL_INFO       (2)
#define EG_LOG_LEVEL_ERROR      (3)
#define EG_LOG_LEVEL_WARN       (4)
#define EG_LOG_LEVEL_DEBUG      (5)

#define Black   0;30
#define Red     0;31
#define Green   0;32
#define Brown   0;33
#define Blue    0;34
#define Purple  0;35
#define Cyan    0;36

void EG_HEX(char *buf,int len);


#define eg_log_fatal(format, ...) \
    do{\
        if(EG_LOG_LEVEL >= EG_LOG_LEVEL_FATAL){\
            eg_log_printf("\033[0;31m[EG_FATAL][%s][%s][%d]\n" format "\r\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
            eg_log_fflush(stdout);\
            eg_log_printf("\033[0m"); \
        }\
    }while(0)

#define eg_log_notice(format, ...) \
    do{\
        if(EG_LOG_LEVEL >= EG_LOG_LEVEL_NOTICE){\
            eg_log_printf("\033[0;36m[EG_NOTICE][%s][%s][%d]\n" format "\r\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
            eg_log_fflush(stdout);\
            eg_log_printf("\033[0m"); \
        }\
    }while(0)

#define eg_log_info(format, ...) \
    do{\
        if(EG_LOG_LEVEL >= EG_LOG_LEVEL_INFO){\
            eg_log_printf("\033[1;36m[EG_INFO][%s][%s][%d]\n" format "\r\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
            eg_log_fflush(stdout);\
            eg_log_printf("\033[0m"); \
        }\
    }while(0)

#define eg_log_error(format, ...) \
    do{\
        if(EG_LOG_LEVEL >= EG_LOG_LEVEL_ERROR){\
            eg_log_printf("\033[0;31m[EG_ERROR][%s][%s][%d]\n" format "\r\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
            eg_log_fflush(stdout);\
            eg_log_printf("\033[0m"); \
        }\
    }while(0)

#define eg_log_warm(format, ...) \
    do{\
        if(EG_LOG_LEVEL >= EG_LOG_LEVEL_WARN){\
            eg_log_printf("\033[1;33m[EG_WARN][%s][%s][%d]\n" format "\r\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
            eg_log_fflush(stdout);\
            eg_log_printf("\033[0m"); \
        }\
    }while(0)


#define eg_log_debug(format, ...) \
    do{\
        if(EG_LOG_LEVEL >= EG_LOG_LEVEL_DEBUG){\
            eg_log_printf("\033[1;32m[EG_DEBUG][%s][%s][%d]\n" format "\r\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
            eg_log_fflush(stdout);\
            eg_log_printf("\033[0m"); \
        }\
    }while(0)




#ifdef __cplusplus
}
#endif




#endif // __EG_LOG_H__





