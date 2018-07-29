/*************************************************************************
	> File Name: main.c
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月08日 星期日 11时43分33秒
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <aio.h>
#include <fcntl.h>
#include <malloc.h>

#include "./common.h"
#include "./parse_configure_file.h"
#include "./write_disk.h"
#include "./get_and_deal_data.h"
#include "./signal_deal.h"
#include "./log.h"
#include "./ctrl.h"


/* 程序配置文件路径*/
#define CONF_PATH  "../etc/store.cfg"


/* 程序是否进入守护进程标志: 
 *     0 代表进入守护进程
 *     1 代表不进入守护进程
 */
int     daemon_flag = 0;           

/* 主循环控制标志*/
int     mct_flag = 1;

/* 配置文件路径*/
char    conf_path[ PATH_MAX ] = {0};


/* 获取运行程序的参数(是否进入守护进程)*/
static int get_opt(int argc, char *argv[])
{
    int opt;

    while(( opt = getopt(argc, argv, "d") ) != -1 )
    {
        switch( opt )
        {
            case 'd':
            {
                daemon_flag = 1;
                break;
            }
            default:
            {
                return -1;
            }
        }
    }

    return 0;
}

/* 使本进程成为守护进程( 后台进程 )*/
static int daemon_instance()
{
    pid_t   pid;
    int i = 3;

    /* 创建进程出错*/
    if ( (pid = fork()) < 0 )
    {
        exit( 0 );
    }

    /* 父进程退出, 子进程即可进入后台运行*/
    else if ( pid > 0 )
    {
        exit( 0 );
    }

    /* 子进程调用 setsid() 创建会话, 使子进程成为进程组的首领( 可以脱离终端 )*/
    if ( setsid() < 0 )
    {
        exit( -1 );
    }

    /* 再创建一个子子进程，而当前进程退出( 脱离当前会话 )*/
    if ( fork() > 0 )
    {
        exit( 0 );
    }

    /* 切换工作目录为根目录( 为了防止程序运行在某个可卸载文件目录下，导致该文件目录无法被卸载 )*/
    if ( ( chdir( "/" ) ) < 0 )
    {
        exit( 0 );
    }

    /* 设置文件权限掩码。当进程创建新文件，如调用open函数时，文件权限将是 mode & 0777 */
    umask( 0 );

    /* 关闭所有文件描述符 ( getdtablesize 函数返回当前进程能打开的最大描述符数 )*/
    for( i = 0; i < getdtablesize(); i++ )
    {
        close(i);
    }

    /* 将标准输入、标准输出和标准错误都定向到 /dev/null 文件*/
    open( "/dev/null", O_RDONLY );  /* 将使用描述符 0*/
    open( "/dev/null", O_RDWR );    /* 将使用描述符 1*/
    open( "/dev/null", O_RDWR );    /* 将使用描述符 2*/

    return 0;
}

/* 获取当前运行程序的所在路径, 是为了得到配置文件的绝对路径*/
static int get_path()
{
    int ret = -1;
    char buff[ PATH_MAX ] = {0};
    char *ptr = NULL;

    /* 通过/proc/self/exe文件获取当前程序的运行绝对路径，并将路径放到 buff 中*/
    ret = readlink("/proc/self/exe", buff, PATH_MAX);
    if (ret < 0)
    {
        return -1;
    }

    /* 刚才获取的绝对路径包含了程序名，而我们需要的是当前程序所在的目录，所以不需要程序名( 找到最后一个'/', 并将其后面的部分截断 )*/
    ptr = strrchr(buff, '/');
    if ( ptr )
    {
        *ptr = '\0';
    }

    /* 将配置文件路径组装好并填充到 conf_path 中*/
    snprintf(conf_path, PATH_MAX, "%s/%s", buff, CONF_PATH);

    return 0;
}

/* 初始化写磁盘线程*/
static int init_write_disk_thread( store_engine_info_t *pStore_engine_info )
{
    int i = 0, j = 0;
    int disk_id = 0;
    
    /* 为所有写磁盘线程申请空间*/
    pWrite_thread_info = (write_disk_thread_info_t *)malloc( sizeof(write_disk_thread_info_t) * pStore_engine_info->m_write_disk_thread_num );

    /* 一个写磁盘线程管理area个磁盘*/
    int area = pStore_engine_info->m_disk_num / pStore_engine_info->m_write_disk_thread_num;

    /* 平均分配后剩余的磁盘数*/
    int left = pStore_engine_info->m_disk_num % pStore_engine_info->m_write_disk_thread_num;

    for ( i = 0; i < pStore_engine_info->m_write_disk_thread_num; i++ )
    {
        /* 为每个写磁盘线程设置线程编号和绑定的cpu编号*/
        pWrite_thread_info[i].m_thread_id = i;
        pWrite_thread_info[i].m_cpu_id = pStore_engine_info->m_pWrite_disk_cpu_array[i];

        /* 设置每个写磁盘线程所管理的磁盘数*/
        pWrite_thread_info[i].m_disk_num = area;
        if ( left > 0 )
        {
            left -= 1;
            pWrite_thread_info[i].m_disk_num = area + 1;
        }

        /* 为每个线程所管理的磁盘id号集合申请空间，并设定初值*/
        pWrite_thread_info[i].m_pDisk_id = (int *)malloc( sizeof(int) * pWrite_thread_info[i].m_disk_num);
        for ( j = 0; j < pWrite_thread_info[i].m_disk_num; j++ )
        {
            pWrite_thread_info[i].m_pDisk_id[j] = disk_id;
            disk_id++;
        }
    }

    return 0;
}

/* 初始化获取并处理数据线程*/
static int init_get_and_deal_data_thread( store_engine_info_t *pStore_engine_info )
{
    int i = 0;
    int disk_id = 0;
    
    /* 为所有获取并处理数据线程申请空间*/
    pRead_thread_info = (get_and_deal_data_thread_info_t *)malloc( sizeof(get_and_deal_data_thread_info_t) * pStore_engine_info->m_get_and_deal_data_thread_num );

    /* 一个线程管理area个磁盘*/
    int area = pStore_engine_info->m_disk_num / pStore_engine_info->m_get_and_deal_data_thread_num;

    /* 平均分配后剩余的磁盘数*/
    int left = pStore_engine_info->m_disk_num % pStore_engine_info->m_get_and_deal_data_thread_num;


    for ( i = 0; i < pStore_engine_info->m_get_and_deal_data_thread_num; i++ )
    {
        /* 为每个线程设置线程编号和绑定的cpu编号*/
        pRead_thread_info[i].m_thread_id = i;
        pRead_thread_info[i].m_cpu_id = pStore_engine_info->m_pGet_and_deal_data_cpu_array[i];

        pRead_thread_info[i].m_min_disk_id = disk_id;

        /* 设置每个线程所管理的磁盘数*/
        pRead_thread_info[i].m_disk_num = area;
        if ( left > 0 )
        {
            left--;
            pRead_thread_info[i].m_disk_num++;
        }

        /* 设置每个线程所管理的磁盘最小id*/
        pRead_thread_info[i].m_min_disk_id = disk_id;
        disk_id += pRead_thread_info[i].m_disk_num;

        /* 为线程处理数据缓冲区申请空间，因为是一个线程管理多个磁盘，
         * 所以为每个磁盘申请一块处理缓冲指针，真正的缓冲区是在每个
         * 磁盘结构中的free队列中
         */
        pRead_thread_info[i].m_buffer = ( node_info_t ** )malloc( sizeof( node_info_t* ) * pRead_thread_info[i].m_disk_num );

    }

    return 0;
}

/* 初始化信号处理线程*/
static int init_signal_deal_thread( store_engine_info_t *pStore_engine_info )
{
    return 0;
}

/* 初始化磁盘*/
static int init_disk( store_engine_info_t *pStore_engine_info )
{
    int i = 0, j = 0;

    pStore_engine_info->m_pDisk_info = (disk_info_t *)malloc( sizeof(disk_info_t) * pStore_engine_info->m_disk_num );
    if( pStore_engine_info->m_pDisk_info == NULL )
    {
        return -1;
    }

    /* 初始化每个磁盘信息结构中的各个字段*/
    for( i = 0; i < pStore_engine_info->m_disk_num; i++ )
    {
        pStore_engine_info->m_pDisk_info[i].m_disk_id = i;
        pStore_engine_info->m_pDisk_info[i].m_seg_type = pStore_engine_info->m_seg_type;
        pStore_engine_info->m_pDisk_info[i].m_seg_time = pStore_engine_info->m_seg_time;
        pStore_engine_info->m_pDisk_info[i].m_seg_size = pStore_engine_info->m_seg_size;
        

        pStore_engine_info->m_pDisk_info[i].m_file_fd = -1;

        pStore_engine_info->m_pDisk_info[i].m_w_flag = 1;   /* 磁盘设置为可写*/
        pStore_engine_info->m_pDisk_info[i].m_pMy_aiocb = (struct aiocb *)malloc( sizeof(struct aiocb) ); 
        pStore_engine_info->m_pDisk_info[i].m_cur_fsize = 0;
        strcpy( pStore_engine_info->m_pDisk_info[i].m_path, pStore_engine_info->m_ppDisk_path[i] );

        /* 初始化队列, 并为free队列申请足够的空间*/
        que_init(&pStore_engine_info->m_pDisk_info[i].m_free_queue);
        que_init(&pStore_engine_info->m_pDisk_info[i].m_busy_queue);
        for( j = 0; j < pStore_engine_info->m_queue_node_num; j++ )
        {
            void *temp = malloc(sizeof(node_info_t));
            que_push( &pStore_engine_info->m_pDisk_info[i].m_free_queue, temp );
        }
        
    }

    return 0;
}


/* 初始化数据*/
static int init_data( char *conf_path, store_engine_info_t *pStore_engine_info )
{
    if( conf_path == NULL || pStore_engine_info == NULL )
    {
        printf( "init_data arguments error!\n" );
    }

    /* 打开配置文件*/
    if( open_conf( conf_path ) < 0 )
    {
        return -1;
    }

    /* 获取磁盘块数*/
    if( get_val_single( "base.disk_num", &pStore_engine_info->m_disk_num, TYPE_INT ) < 0 )
    {
        return -1;
    }

    /* 获取读取数据线程个数*/
    if( get_val_single( "base.get_data_thread_num", &pStore_engine_info->m_get_and_deal_data_thread_num, TYPE_INT ) < 0 )
    {
        return -1;
    }

    /* 获取写磁盘线程个数*/
    if( get_val_single( "base.write_disk_thread_num", &pStore_engine_info->m_write_disk_thread_num, TYPE_INT ) < 0 )
    {
        return -1;
    }

    /* 获取信号处理线程个数*/
    if( get_val_single( "base.signal_deal_thread_num", &pStore_engine_info->m_signal_deal_thread_num, TYPE_INT ) < 0 )
    {
        return -1;
    }


    /* 获取写磁盘线程可用cpu数组*/
    pStore_engine_info->m_pWrite_disk_cpu_array = (int *)malloc( sizeof(int) * pStore_engine_info->m_write_disk_thread_num );
    if( get_val_array( "base.write_disk_cpu", (void **)&pStore_engine_info->m_pWrite_disk_cpu_array, pStore_engine_info->m_write_disk_thread_num, TYPE_INT ) < 0 )
    {
        return -1;
    }

    /* 获取读取数据线程可用cpu数组*/
    pStore_engine_info->m_pGet_and_deal_data_cpu_array = (int *)malloc( sizeof(int) * pStore_engine_info->m_get_and_deal_data_thread_num);
    if( get_val_array( "base.get_data_cpu", (void **)&pStore_engine_info->m_pGet_and_deal_data_cpu_array, pStore_engine_info->m_get_and_deal_data_thread_num, TYPE_INT ) < 0 )
    {
        return -1;
    }

    /* 获取信号处理线程可用cpu数组*/
    pStore_engine_info->m_pSignal_deal_cpu_array = (int *)malloc( sizeof(int) * pStore_engine_info->m_signal_deal_thread_num);
    if( get_val_array( "base.signal_deal_cpu", (void **)&pStore_engine_info->m_pSignal_deal_cpu_array, pStore_engine_info->m_signal_deal_thread_num, TYPE_INT ) < 0 )
    {
        return -1;
    }
    

    /* 获取磁盘路径数组( 即所有磁盘 )*/
    pStore_engine_info->m_ppDisk_path = (char **)malloc( sizeof(char *) * pStore_engine_info->m_disk_num );
    if( get_val_array( "base.disk_path", (void **)pStore_engine_info->m_ppDisk_path, pStore_engine_info->m_disk_num, TYPE_STRING ) < 0 )
    {
        return -1;
    }

    /* 获取磁盘文件分割类型*/
    if( get_val_single( "base.seg_type", &pStore_engine_info->m_seg_type, TYPE_INT ) < 0 )
    {
        return -1;
    }

    /* 获取磁盘文件时间分割间隔*/
    if( get_val_single( "base.seg_time", &pStore_engine_info->m_seg_time, TYPE_LONG ) < 0 )
    {
        return -1;
    }

    /* 获取磁盘文件分割大小*/
    if( get_val_single( "base.seg_size", &pStore_engine_info->m_seg_size, TYPE_LONG ) < 0 )
    {
        return -1;
    }
    /* 文件分割大小单位为兆 ( M )*/
    pStore_engine_info->m_seg_size *= SIZE_M;

    /* 获取磁盘写入队列长度*/
    if( get_val_single( "base.queue_num", &pStore_engine_info->m_queue_node_num, TYPE_INT ) < 0 )
    {
        return -1;
    }

    /* 获取管道控制文件路径*/
    if( get_val_single( "base.log_fifo_path.fifo_file", pStore_engine_info->m_fifo_file_path, TYPE_STRING ) < 0 )
    {
        return -1;
    }

    /* 获取日志文件路径*/
    if( get_val_single( "base.log_fifo_path.log_file", pStore_engine_info->m_log_file_path, TYPE_STRING ) < 0 )
    {
        return -1;
    }

    /* 初始化磁盘*/
    if( init_disk(pStore_engine_info) < 0 )
    {
        return -1;
    }
    
    /* 初始化写磁盘线程*/
    if( init_write_disk_thread( pStore_engine_info) < 0 )
    {
        return -1;
    }

    /* 初始化信号处理线程*/
    if( init_signal_deal_thread( pStore_engine_info) < 0 )
    {
        return -1;
    }

    /* 初始化获取并处理数据线程*/
    if( init_get_and_deal_data_thread( pStore_engine_info) < 0 )
    {
        return -1;
    }

    /* 关闭配置文件并释放资源*/
    close_conf();
    return 0;
}

/* 打印获取到的配置信息*/
void print_data(store_engine_info_t *pStore_engine_info)
{
    int i = 0;
    printf( "disk_num ;[%d]\n", pStore_engine_info->m_disk_num );
    for( i = 0; i < pStore_engine_info->m_disk_num; i++ )
    {
        printf( "\tdisk_path:[%s]\n", pStore_engine_info->m_ppDisk_path[i] );
    }

    printf( "get_and_deal_data_thread_num: [%d]\n", pStore_engine_info->m_get_and_deal_data_thread_num);
    for( i = 0; i < pStore_engine_info->m_get_and_deal_data_thread_num; i++)
    {
        printf("\tget_and_deal_data_thread_cpu_id:[%d]\n", pStore_engine_info->m_pGet_and_deal_data_cpu_array[i]);
    }

    printf( "write_disk_thread_num:[%d]\n", pStore_engine_info->m_write_disk_thread_num );
    for( i = 0; i < pStore_engine_info->m_write_disk_thread_num; i++ )
    {
        printf( "\twrite_disk_thread_cpu_id:[%d]\n", pStore_engine_info->m_pWrite_disk_cpu_array[i] );
    }

    printf( "signal_deal_thread_num:[%d]\n", pStore_engine_info->m_signal_deal_thread_num );
    for( i = 0; i < pStore_engine_info->m_signal_deal_thread_num; i++ )
    {
        printf( "\tsignal_deal_thread_cpu_id:[%d]\n", pStore_engine_info->m_pSignal_deal_cpu_array[i] );
    }

    printf( "seg_type:[%d]\n", pStore_engine_info->m_seg_type);
    printf( "seg_time:[%ld]\n", pStore_engine_info->m_seg_time );
    printf( "seg_size:[%ld]\n", pStore_engine_info->m_seg_size );
    printf( "queue_node_num:[%d]\n", pStore_engine_info->m_queue_node_num);
    printf( "fifo_file:[%s]\n", pStore_engine_info->m_fifo_file_path );
    printf( "log_file:[%s]\n", pStore_engine_info->m_log_file_path );

}

/* stop-main命令的回调函数*/
void stop_main_func(int argc, char *argv[])
{
    int flag = atoi(argv[1]);

    printf("recv cmd:[%s]\n", argv[0]);

    if( flag == 0 )
    {
        mct_flag = 0;
    }
    else
    {
        mct_flag = 1;
    }
}

/* 启动管控模块*/
static int start_ctrl( store_engine_info_t *pStore_engine_info )
{
    if( ctrl_init(pStore_engine_info->m_fifo_file_path) < 0 )
    {
        return -1;
    }

    /* 添加管控命令*/
    ctrl_node_add("stop-main", "stop main pthread while 0-stop 1-start", stop_main_func );

    return 0;
}

int main(int argc, char *argv[])
{
    /* 日志输出类型*/
    char log_type[ 8 ] = {0};

    /* 解析程序参数*/
    if( get_opt(argc, argv) < 0 )
    {
        printf( "parse option error!\n" );
    }

    /* 如果程序为后台运行模式，即没有 -d (debug 调试)选项，则使进程成为守护进程*/
    if( ! daemon_flag )
    {
        daemon_instance();
        strcpy( log_type, "f_cat" );     /* 将日志信息写到文件中*/
    }
    else
    {
        strcpy( log_type, "o_cat" );     /* 将日志信息写到屏幕上*/
    }

    /* 获取到配置文件的绝对路径conf_path*/
    if( get_path() < 0 )
    {
        return -1;
    }

    /* 定义存储引擎系统结构*/
    store_engine_info_t store_engine_info;
    memset( &store_engine_info, 0x00, sizeof( store_engine_info_t ) );
    

    /* 初始化数据*/
    if( init_data( conf_path, &store_engine_info) < 0 )
    {
        return -1;
    }

    /* 打印获取到的配置信息*/
    print_data(&store_engine_info );

    /* 打开日志文件*/
    if( open_log(store_engine_info.m_log_file_path, log_type) )
    {
        return -1;
    }

    /* 屏蔽所有信号*/
    if( block_allsig( MASK_SIG ) )
    {
        return -1;
    }

    /* 启动管控模块*/
    if( start_ctrl( &store_engine_info ) < 0)
    {
        return -1;
    }

    /* 启动信号处理模块*/
    if( start_signal_thread( &store_engine_info ) < 0 )
    {
        return -1;
    }

    /* 启动写盘模块*/
    if( start_write_thread( &store_engine_info ) < 0 )
    {
        return -1;
    }

    /* 启动获取数据模块*/
    if( start_read_thread( &store_engine_info ) < 0 )
    {
        return -1;
    }

    while( 1 )
    {
        while( mct_flag == 0 )
        {
            sleep(1);
        }
        DBG("sunxiwang");
        sleep(1);
    }

    return 0;
}
