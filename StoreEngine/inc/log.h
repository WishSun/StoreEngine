/*************************************************************************
	> File Name: log.h
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月10日 星期二 05时18分18秒
 ************************************************************************/

#ifndef _LOG_H
#define _LOG_H

#include <zlog.h>

/* 该句柄在log.c文件中定义*/
extern zlog_category_t  *log_handle;

#define ALL(...)  zlog_fatal(log_handle, __VA_ARGS__)
#define INF(...)  zlog_info(log_handle, __VA_ARGS__)
#define WAR(...)  zlog_warn(log_handle, __VA_ARGS__)
#define DBG(...)  zlog_debug(log_handle, __VA_ARGS__)
#define ERR(...)  zlog_error(log_handle, __VA_ARGS__)

/* 打开日志文件*/
int open_log( char *path, char *mode );

/* 关闭日志文件*/
void close_log();
#endif
