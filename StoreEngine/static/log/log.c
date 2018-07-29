/*************************************************************************
	> File Name: log.c
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月10日 星期二 05时21分52秒
 ************************************************************************/

#include <stdio.h>
#include "../../inc/log.h"

zlog_category_t  *log_handle = NULL;

/* 打开日志文件*/
int open_log( char *path, char *mode )
{
    if( path == NULL || mode == NULL )
    {
        return -1;
    }

    if( zlog_init(path) != 0 )
    {
        printf( "log system init failed!\n" );
        return -1;
    }

    if( (log_handle = zlog_get_category(mode)) == NULL )
    {
        printf( "init log handle error!\n" );
        return -1;
    }

    return 0;
}

/* 关闭日志文件*/
void close_log()
{
    zlog_fini(); 
}
