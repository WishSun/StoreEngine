/*************************************************************************
	> File Name: common.h
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月08日 星期日 21时18分21秒
 ************************************************************************/

#ifndef _COMMON_H
#define _COMMON_H

#include "./queue.h"

/* 最大路径长度*/
#define PATH_MAX  1024

/* aio通知信号*/
#define SIG_RETURN (SIGRTMIN+4)

/* 单位K、M、G所对应的字节数*/
#define SIZE_K    (1<<10)
#define SIZE_M    (1<<20)
#define SIZE_G    (1<<30)


typedef struct _node_info_t
{
    int len;            /* 数据的长度*/
    char buff[SIZE_M];  /* 一次最多写 1M*/
}node_info_t;

/* 磁盘信息结构*/
typedef struct _disk_info_t
{
    volatile int    m_w_flag;     /* 当前磁盘是否正在写数据，决定了当前是否可以从队列中获取数据*/
    int             m_file_fd;
    int             m_disk_id;
    int             m_seg_type;   /* 文件分割类型*/
    int64_t         m_seg_time;   /* 指定时间间隔*/
    int64_t         m_seg_size;   /* 指定文件大小*/
    int64_t         m_cur_fsize;  /* 当前文件大小*/
    int64_t         m_cur_ftime;  /* 文件写入起始时间*/
    que_t           m_free_queue; /* 空闲队列*/
    que_t           m_busy_queue; /* 忙队列*/
    node_info_t     *m_pNode_info;/* 当前正在写的数据结点*/ 
    struct aiocb    *m_pMy_aiocb;
    char            m_path[ PATH_MAX ];
}disk_info_t;

/* 存储引擎系统结构信息*/
typedef struct _store_engine_info_t
{
    int     m_disk_num;                          /* 磁盘个数*/
    int     m_get_and_deal_data_thread_num;      /* 获取并处理数据线程个数*/
    int     m_write_disk_thread_num;             /* 写磁盘线程个数*/
    int     m_signal_deal_thread_num;            /* 处理信号线程个数*/
    int     *m_pGet_and_deal_data_cpu_array;     /* 获取并处理数据线程可绑定的cpu数组*/
    int     *m_pWrite_disk_cpu_array;            /* 写磁盘线程可绑定的cpu数组*/
    int     *m_pSignal_deal_cpu_array;           /* 处理信号线程可绑定cpu数组*/

    int     m_seg_type;                          /* 文件分割方式, 0-时间，1-大小*/
    int64_t m_seg_time;                          /* 文件分割的间隔时间*/
    int64_t m_seg_size;                          /* 文件分割的大小*/

    int     m_queue_node_num;                    /* 队列的结点个数*/
    char    m_fifo_file_path[ PATH_MAX ];        /* 管道控制文件路径*/
    char    m_log_file_path[ PATH_MAX ];         /* 日志文件路径*/

    char    **m_ppDisk_path;                     /* 磁盘挂载路径( 是一个字符串数组 )*/
    disk_info_t  *m_pDisk_info;                  /* 所有磁盘信息*/

}store_engine_info_t;

#endif
