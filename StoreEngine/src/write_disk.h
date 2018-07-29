/*************************************************************************
	> File Name: write_disk.h
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月08日 星期日 21时00分31秒
 ************************************************************************/

#ifndef _WRITE_DISK_H
#define _WRITE_DISK_H

#include "./common.h"

/* 写磁盘的线程信息结构*/
typedef struct _write_disk_thread_info_t
{
    int m_thread_id;         /* 线程编号*/
    int m_cpu_id;            /* 绑定的cpu编号*/
    int *m_pDisk_id;         /* 该线程管理的磁盘编号集合*/
    int m_disk_num;          /* 该线程管理的磁盘个数*/
}write_disk_thread_info_t;

/* 所有写数据线程信息*/
write_disk_thread_info_t *pWrite_thread_info;

/* 开启所有写磁盘线程*/
int start_write_thread(store_engine_info_t *pStore_engine_info);
#endif
