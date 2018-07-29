/*************************************************************************
	> File Name: write_disk.c
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月08日 星期日 21时00分20秒
 ************************************************************************/

#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <aio.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include "../../inc/write_disk.h"
#include "../../inc/log.h"

/* 所有磁盘信息*/
static disk_info_t *pDisk_info;


/* 创建新的文件，并对磁盘信息进行更新*/
int change_file( disk_info_t *pD_info )
{
    DBG("创建新文件！");
    time_t _time;
    char new_file_name[PATH_MAX] = {0};

    if( pD_info->m_file_fd > 0 )
    {
        close(pD_info->m_file_fd);
    }

    _time = time(NULL);
    sprintf(new_file_name, "%s/%ld.dat", pD_info->m_path, _time);

    while( (pD_info->m_file_fd = open(new_file_name, O_CREAT | O_WRONLY, 0664)) < 0 )
    {
        usleep(10);
        continue;
    }

    pD_info->m_cur_fsize = 0;
    pD_info->m_cur_ftime = (int64_t)_time;

    return 0;
}

/* 写磁盘线程函数*/
void *write_thread_run( void *arg )
{
    write_disk_thread_info_t *w_thr_info = NULL;
    w_thr_info = (write_disk_thread_info_t *)arg;

    int i = 0, disk_id;
    time_t _time;

    while( 1 )
    {
        /* 获取当前时间*/
        _time = time(NULL);

        for( i = 0; i < w_thr_info->m_disk_num; i++ )
        {
            disk_id = w_thr_info->m_pDisk_id[i];

            /* 当前磁盘正在有数据写入，当前不能继续写入，应该等待
             * 正在写的数据写入完成
             */
            if( pDisk_info[disk_id].m_w_flag == 0 )
            {
                continue;
            }

            /* 循环获取每个磁盘数据队列中的数据*/
            if( ! (pDisk_info[i].m_pNode_info = que_pop( &pDisk_info[i].m_busy_queue ) ) )
            {
               continue; 
            }

            /* 对文件大小或者文件间隔时间进行检查，若超标则创建新文件*/
            if( pDisk_info[disk_id].m_seg_type == 0 )
            {/* time*/
                if( (_time - pDisk_info[disk_id].m_cur_ftime) > pDisk_info[disk_id].m_seg_time )
                {
                    change_file( &pDisk_info[disk_id] );
                }
            }
            else
            {/* size*/
                if( pDisk_info[disk_id].m_cur_fsize + pDisk_info[disk_id].m_pNode_info->len > pDisk_info[disk_id].m_seg_size )
                {
                    change_file( &pDisk_info[disk_id] );
                }
            }

            pDisk_info[disk_id].m_w_flag = 0;

            /* 指定aio要往哪个文件描述符里边写入数据*/
            if( pDisk_info[disk_id].m_file_fd == -1 )
            {
                change_file( &pDisk_info[disk_id] );
            }
            pDisk_info[disk_id].m_pMy_aiocb->aio_fildes = pDisk_info[disk_id].m_file_fd;

            
            /* 对要写入的数据地址进行赋值*/
            pDisk_info[disk_id].m_pMy_aiocb->aio_buf = pDisk_info[disk_id].m_pNode_info->buff;

            /* 要写入的数据长度*/
            pDisk_info[disk_id].m_pMy_aiocb->aio_nbytes = pDisk_info[disk_id].m_pNode_info->len;

            /* 从文件哪个位置开始写入数据*/
            pDisk_info[disk_id].m_pMy_aiocb->aio_offset = pDisk_info[disk_id].m_cur_fsize;

            /* 设置使用信号方式来通知进程，系统已经完成数据的写入过程*/
            pDisk_info[disk_id].m_pMy_aiocb->aio_sigevent.sigev_notify = SIGEV_SIGNAL;

            /* 使用SIG_RETURN这个自定义信号来通知*/
            pDisk_info[disk_id].m_pMy_aiocb->aio_sigevent.sigev_signo = SIG_RETURN;

            /* 使用信号通知进程事件触发的时候，顺便携带一个参数过去*/
            pDisk_info[disk_id].m_pMy_aiocb->aio_sigevent.sigev_value.sival_ptr = &pDisk_info[disk_id];

            /* 这个不设置TM就aio_write调用失败！！？*/
            pDisk_info[disk_id].m_pMy_aiocb->aio_reqprio = 1;

            /* 开始写入*/
            while( aio_write( pDisk_info[disk_id].m_pMy_aiocb ) < 0 )
            {
                if( errno == EINVAL )
                {
                    perror("aio_write");
                    exit(0);
                }    
                usleep(10);
                continue;
            }
        }
    }

    return NULL;
}

/* 开启所有写磁盘线程*/
int start_write_thread(store_engine_info_t *pStore_engine_info)
{
    pthread_t  tid;
    int i = 0;
    pDisk_info = pStore_engine_info->m_pDisk_info;

    for( i = 0; i < pStore_engine_info->m_write_disk_thread_num; i++ )
    {
        if( pthread_create( &tid, NULL, write_thread_run, (void *)&pWrite_thread_info[i] ) < 0  )
        {
            ERR("start write thread error!");
            return -1;
        }
    }

    return 0;
}
