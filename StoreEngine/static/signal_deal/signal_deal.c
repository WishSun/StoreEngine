/*************************************************************************
	> File Name: signal_deal.c
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月09日 星期一 12时29分23秒
 ************************************************************************/

#include <pthread.h>
#include <unistd.h>
#include <aio.h>
#include <signal.h>
#include "../../inc/signal_deal.h"
#include "../../inc/log.h"

/* 屏蔽/解除屏蔽本线程所有信号*/
int block_allsig(int flag)
{
    sigset_t   signal_set;

    if( sigemptyset( &signal_set ) < 0 )
    {
        return -1;
    }
    if( sigfillset( &signal_set ) < 0 )
    {
        return -1;
    }

    if( flag == MASK_SIG )
    {
        if( pthread_sigmask( SIG_BLOCK, &signal_set, NULL ) != 0 )
        {
            return -1;
        }    
    }
    else
    {
        if( pthread_sigmask( SIG_UNBLOCK, &signal_set, NULL ) != 0 )
        {
            return -1;
        }    
    }

    return 0;
}


/* aio信号处理*/
void aio_write_sig_deal(int signo, siginfo_t *info, void *ctext )
{
    disk_info_t *pD_info;
    if( signo == SIG_RETURN )
    {
        pD_info = (disk_info_t *)info->si_ptr;

        /* 写入成功*/
        if( aio_error( pD_info->m_pMy_aiocb ) == 0 )
        {
            if( pD_info->m_pMy_aiocb->aio_nbytes != aio_return(pD_info->m_pMy_aiocb) )
            {/* 写入的数据不完整, 则再继续写*/
                while(aio_write(pD_info->m_pMy_aiocb) < 0)
                {
                    usleep(10);
                    continue;
                }
            }
            else
            {/* 写入数据完整，则将正在写的数据块缓存归还free队列*/
                que_push( &pD_info->m_free_queue, (void *)pD_info->m_pNode_info );

                /* 更新文件大小*/
                pD_info->m_cur_fsize += pD_info->m_pMy_aiocb->aio_nbytes;

                /* 指示写线程可以再去busy队列拿数据去写磁盘了*/
                pD_info->m_w_flag = 1;
            }
        }
        else
        {/* 写入不成功，那就继续写*/
            while( aio_write(pD_info->m_pMy_aiocb) < 0 )
            {
                usleep(10);
                continue;
            }
        }
    }
}

void *signal_thread_run(void *arg)
{
    struct sigaction sig_act;
    DBG("INTO signal_thread_run");

    if( sigemptyset(&sig_act.sa_mask) < 0 )
    {
        return NULL;
    }
    sig_act.sa_flags = SA_SIGINFO;
    sig_act.sa_sigaction = aio_write_sig_deal;

    if( sigaction(SIG_RETURN, &sig_act, NULL) )
    {
        return NULL;
    }

    block_allsig(UNMASK_SIG);

    while(1)
    {
        sleep(10);
    }

    return NULL;
}

/* 开启所有处理信号线程*/
int start_signal_thread(store_engine_info_t *pStore_engine_info)
{
    pthread_t tid;
    int i = 0;

    for( i = 0; i < pStore_engine_info->m_signal_deal_thread_num; i++ )
    {
        if( pthread_create(&tid, NULL, signal_thread_run, NULL) < 0 )
        {
            ERR("start signal thread error!");
            return -1;
        }
    }

    return 0;
}

