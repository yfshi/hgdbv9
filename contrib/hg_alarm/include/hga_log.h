#ifndef PG_LOG_H
#define PG_LOG_H

#ifdef FRONTEND
/* FRONTEND */
#include "common/logging.h"
#include "logger_fe.h"
#define hga_log_error(...) do { \
        pg_log_error(__VA_ARGS__); \
        LoggerLog(PG_LOG_ERROR, __VA_ARGS__); \
    } while(0)
#define hga_log_warning(...) do { \
        pg_log_warning(__VA_ARGS__); \
        LoggerLog(PG_LOG_WARNING, __VA_ARGS__); \
    } while(0)
#define hga_log_info(...) do { \
        pg_log_info(__VA_ARGS__); \
        LoggerLog(PG_LOG_INFO, __VA_ARGS__); \
    } while(0)
#define hga_log_fatal(...) do { \
        pg_log_fatal(__VA_ARGS__); \
        LoggerLog(PG_LOG_FATAL, __VA_ARGS__); \
        exit(1); \
    } while(0)
#define hga_log_debug(...) do { \
        pg_log_debug(__VA_ARGS__); \
    } while(0)

#else
/* BACKEND */
#include "utils/elog.h"
#define hga_log_error(...)      elog(ERROR, __VA_ARGS__)
#define hga_log_fatal(...)      elog(ERROR, __VA_ARGS__) /* backends cannot report fatal error */
#define hga_log_warning(...)    elog(WARNING, __VA_ARGS__)
#define hga_log_info(...)       elog(INFO, __VA_ARGS__)
#define hga_log_debug(...)      do { \
        elog(DEBUG5, __VA_ARGS__); \
    } while(0)
#endif

#endif  /* PG_LOG_H */