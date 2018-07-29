/*************************************************************************
	> File Name: signal_deal.h
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月09日 星期一 12时29分14秒
 ************************************************************************/

#ifndef _SIGNAL_DEAL_H
#define _SIGNAL_DEAL_H
#include "./common.h"

enum __sig_deal_type
{
    MASK_SIG = 0,   /* 屏蔽所有信号*/
    UNMASK_SIG      /* 对所有信号解除屏蔽*/
};

typedef struct _signal_deal_thread_info_t
{
    int m_thread_id;
    int m_cpu_id;
}signal_deal_thread_info_t;


/* 所有信号处理线程信息*/
signal_deal_thread_info_t *pSignal_thread_info;

/* 屏蔽/解除屏蔽本线程所有信号*/
int block_allsig(int flag);

/* 开启所有处理信号线程*/
int start_signal_thread(store_engine_info_t *pStore_engine_info);
#endif
