/*************************************************************************
	> File Name: get_and_deal_data.h
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月08日 星期日 21时01分11秒
 ************************************************************************/

#ifndef _GET_AND_DEAL_DATA_H
#define _GET_AND_DEAL_DATA_H

#include "./common.h"

/* 获取并处理数据线程结构信息*/
typedef struct _get_and_deal_data_thread_info_t
{
    int     m_thread_id;    /* 线程编号*/
    int     m_cpu_id;       /* 线程绑定的cpu编号*/
    int     m_min_disk_id;  /* 线程所管理的磁盘的最小id, 可以根据它和管理的磁盘个数确定所管理的所有磁盘的id*/
    int     m_disk_num;     /* 线程管理的磁盘个数*/
    node_info_t  **m_buffer;/* 线程处理缓冲集合(结点数和所管理磁盘个数相同)*/

}get_and_deal_data_thread_info_t;

/* 所有读数据线程信息*/
get_and_deal_data_thread_info_t *pRead_thread_info;

/* 启动读取数据线程*/
int start_read_thread(store_engine_info_t *pStore_engine_info);
#endif
