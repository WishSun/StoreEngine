/*************************************************************************
	> File Name: get_and_deal_data.c
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月08日 星期日 21时01分20秒
 ************************************************************************/
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <pcap.h>
#include <malloc.h>

#include "./common.h"
#include "./get_and_deal_data.h"
#include "./log.h"

/* 所有磁盘信息*/
static disk_info_t  *pDisk_info;

/* 一次最多获取的数据包大小*/
#define PACKET_SIZE (SIZE_M)

/**/
typedef struct 
{
    char ptr[ PACKET_SIZE ];
    int  ptr_len;
}get_data_t;

/* 抓取数据包*/
void getPacket(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *packet_ptr)
{
    get_data_t *pGet_data = (get_data_t *)arg;

    if( pGet_data->ptr_len + pkthdr->caplen > PACKET_SIZE )
    {
        return;
    }

    memcpy(pGet_data->ptr + pGet_data->ptr_len, packet_ptr, pkthdr->caplen);
    pGet_data->ptr_len += pkthdr->caplen;
}

/* 初始化pcap*/
pcap_t *init_pcap()
{
    char errBuf[PCAP_ERRBUF_SIZE], *devStr;

    devStr = pcap_lookupdev(errBuf);
    if( !devStr )
    {
        DBG("pcap_lookupdev : %s", errBuf);
        return NULL;
    }

    pcap_t *device = pcap_open_live( devStr, 65535, 1, 0, errBuf );
    if( !device )
    {
        DBG("pcap_open_live: %s", errBuf);
        return NULL;
    }

    /* 过滤端口*/
    //struct bpf_program filter;
    //pcap_compile(device, &filter, "dst prot 7000", 1, 0);
    //pcap_setfilter(device, &filter);
    
    return device;
}

/* 获取到处理好的数据*/
void get_dealed_data(pcap_t *device, get_data_t *pGet_data)
{
    /* 实测出来每次抓包的包大小为1000字节，要抓够1M就得1000次
     * 实测1000最快，大于1000和小于1000都比较慢
     */
    pcap_loop(device, 1000, getPacket, (u_char *)pGet_data);
}

/* 选择一个本线程所管理的磁盘id*/
int get_disk( get_and_deal_data_thread_info_t *pInfo )
{
    static uint64_t i = 0;
    i++;

    /* 轮询选择一个磁盘id*/
    return (pInfo->m_min_disk_id + ( i % pInfo->m_disk_num ));
}

void *thread_run( void *args )
{
    int i = 0;

    get_and_deal_data_thread_info_t *pRead_thread_info = (get_and_deal_data_thread_info_t *)args;

    /* 本线程所管理磁盘的最小id*/
    int min_disk_id = pRead_thread_info->m_min_disk_id;

    /* 将要被写的磁盘的id*/
    int w_disk_id = 0;
    /* 将要被写的磁盘在buffer中的下标*/
    int w_disk_index = 0;

    for( i = 0; i < pRead_thread_info->m_disk_num; i++ )
    {
        /* 从每一个磁盘空闲队列中获取一个结点，将地址赋值给
         * 处理线程的，通过来操作空间
         */
        while( ! ( pRead_thread_info->m_buffer[i] = que_pop( &pDisk_info[ min_disk_id+i ].m_free_queue ) ) )
        {
            /* 如果未获取到，等一会再继续获取*/
            usleep(10);
            continue;
        }

        /* 将缓冲区的数据长度置为0*/
        pRead_thread_info->m_buffer[i]->len = 0;
    }

    get_data_t get_data;
    pcap_t *device = init_pcap();
    while( 1 )
    {
        get_data.ptr_len = 0;

        /* 获取数据*/
        get_dealed_data(device, &get_data);
        printf("获取到 %d 字节数据\n", get_data.ptr_len);
        
        /* 获取一个即将写入数据的磁盘的id*/
        w_disk_id = get_disk( pRead_thread_info );

        /* 获取已经获取到磁盘在buffer中的结点下标*/
        w_disk_index = w_disk_id - pRead_thread_info->m_min_disk_id;

        if( (int64_t)(pRead_thread_info->m_buffer[w_disk_index]->len + get_data.ptr_len) > (int64_t)SIZE_M )
        {
            /* 如果磁盘的结点数据已经即将写满1M，那么就把结点放入数据队列(busy队列)*/
            printf("可以去写磁盘了\n");
            que_push( &pDisk_info[w_disk_id].m_busy_queue, pRead_thread_info->m_buffer[w_disk_index] );

            /* 将结点放入数据队列中后，重新获取一个空闲结点准备写数据，并将结点数据长度置为 0*/
            while( ! ( pRead_thread_info->m_buffer[w_disk_index] = que_pop( &pDisk_info[w_disk_id].m_free_queue ) ) )
            {
                /* 如果未获取到，等一会再继续获取*/
                usleep(10);
                continue;
            }
            pRead_thread_info->m_buffer[w_disk_index]->len = 0;
        }

        /* 将数据拷贝到结点中，并更新结点的数据长度*/
        memcpy(pRead_thread_info->m_buffer[w_disk_index]->buff + pRead_thread_info->m_buffer[w_disk_index]->len, get_data.ptr, get_data.ptr_len);
        pRead_thread_info->m_buffer[w_disk_index]->len += get_data.ptr_len;
    }

    return NULL;
}

/* 启动读取数据线程*/
int start_read_thread(store_engine_info_t *pStore_engine_info)
{
    int i = 0;
    pthread_t   tid;

    pDisk_info = pStore_engine_info->m_pDisk_info;

    for( i = 0; i < pStore_engine_info->m_get_and_deal_data_thread_num; i++ )
    {
        if( pthread_create( &tid, NULL, thread_run, (void *)&pRead_thread_info[i] ) )
        {
            ERR("start get_and_deal_data pthread error!\n");
            return -1;
        }
    }

    return 0;
}
