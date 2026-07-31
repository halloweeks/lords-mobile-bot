#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

/* Simple logging macros */
#define LOGE(fmt, ...) log_error(fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) log_info(fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) log_warn(fmt, ##__VA_ARGS__)

#ifdef _DEBUG_
    #define LOGD(fmt, ...) log_debug(fmt, ##__VA_ARGS__)
#else
    #define LOGD(fmt, ...) ((void)0)
#endif

/* Function declarations */
void log_error(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);

#ifdef _DEBUG_
	void log_debug(const char *fmt, ...);
#endif

#endif