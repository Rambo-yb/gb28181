#include "log.h"
#if defined(USE_EASYLOGGER)
#include "elog.h"
#endif

int log_init(char* file, int size, int cnt, int lvl) {
#if defined(USE_EASYLOGGER)
    if (elog_init(file, size, cnt) == ELOG_ALREADY_INIT) {
        return 0;
    }
    elog_set_filter_lvl(lvl);

    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL& ~ELOG_FMT_LVL & ~ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_TIME | ELOG_FMT_FUNC | ELOG_FMT_LINE);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_TIME | ELOG_FMT_FUNC | ELOG_FMT_LINE);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_TIME | ELOG_FMT_FUNC | ELOG_FMT_LINE);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_TIME | ELOG_FMT_FUNC | ELOG_FMT_LINE);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~ELOG_FMT_LVL & ~ELOG_FMT_TAG);
    elog_start();
#endif

    return 0;
}