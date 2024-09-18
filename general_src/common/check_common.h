#ifndef __CHECK_COMMON_H__
#define __CHECK_COMMON_H__

#include "log.h"

#define ERR_OPT_NULL

#define CHECK_POINTER(x, err_opt)   \
    if (x == NULL) {    \
        LOG_ERR("%s is NULL", #x); \
        err_opt; \
    }    
    
#define CHECK_BOOL(x, err_opt)   \
    if (!x) {    \
        LOG_ERR("%s is false", #x); \
        err_opt; \
    }

#define CHECK_LT(x, y, err_opt)   \
    if (x < y) {    \
        LOG_ERR("%s:%d < %s:%d", #x, x, #y, y);   \
        err_opt; \
    }

#define CHECK_GE(x, y, err_opt)   \
    if (x >= y) {    \
        LOG_ERR("%s:%d >= %s:%d", #x, x, #y, y);   \
        err_opt; \
    }

#define CHECK_EQ(x, y, err_opt)   \
    if (x != y) {    \
        LOG_ERR("%s:%d != %s:%d", #x, x, #y, y);   \
        err_opt; \
    }

#endif