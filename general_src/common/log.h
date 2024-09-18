#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

#if defined(USE_EASYLOGGER)

#include "elog.h"

#define LOG_ERR(format, ...) \
    log_e(format, ##__VA_ARGS__)
    
#define LOG_WRN(format, ...) \
    log_w(format, ##__VA_ARGS__)
    
#define LOG_INFO(format, ...) \
    log_i(format, ##__VA_ARGS__)

#define LOG_DEBUG(format, ...) \
    log_d(format, ##__VA_ARGS__)

#define LOG_HEX(data, size, wide) \
    elog_hexdump("", wide, data, size);

#else

#define LOG_ERR(format, ...) \
    printf("\e[0;31m[%s:%d %s]"format"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
    
#define LOG_WRN(format, ...) \
    printf("\e[0;33m[%s:%d %s]"format"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
    
#define LOG_INFO(format, ...) \
    printf("\e[0m[%s:%d %s]"format"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define LOG_DEBUG(format, ...) \
    printf("\e[0;32m[%s:%d %s]"format"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define LOG_HEX(data, size, wide) \
    do {    \
        for (int i = 0; i * wide < size; i++) { \
            for (int j = 0; j < wide && i*wide + j < size; j++) {   \
                printf("%02x ", data[i*wide+j]);    \
            }   \
            printf("\n");   \
        }   \
    }while(0);
#endif

int log_init(char* file, int size, int cnt, int lvl);

#endif