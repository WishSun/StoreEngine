/*************************************************************************
	> File Name: ctrl.c
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月11日 星期三 01时41分07秒
 ************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "./ctrl.h"
#include "./log.h"

static ctrl_node_t *pCtrl_head = NULL;;
static int argcs = 0;
static char *argvs[32];

/* 向命令控制链表中添加结点*/
void ctrl_node_add(char *key, char *info, bc_func_t func)
{
    ctrl_node_t *p_node = NULL;
    ctrl_node_t *pNew_node = NULL;

    if( !key || !func )
    {
        return;
    }

    /* 先申请头结点*/
    if( NULL == pCtrl_head )
    {
        pCtrl_head = (ctrl_node_t *)malloc(sizeof(ctrl_node_t));
        memset( pCtrl_head, 0x00, sizeof(ctrl_node_t) );
        pCtrl_head->m_pNext = NULL;
    }

    /* 找到尾结点*/
    for(p_node = pCtrl_head; p_node->m_pNext != NULL; p_node = p_node->m_pNext)
    {
        ;
    }

    pNew_node = (ctrl_node_t *)malloc( sizeof(ctrl_node_t) );
    memset( pNew_node, 0x00, sizeof( ctrl_node_t ) );
    strncpy( pNew_node->m_key, key, 32 );

    if( info )
    {
        strncpy( pNew_node->m_info, info, 128 );
    }

    pNew_node->m_bc_func = func;
    pNew_node->m_pNext = NULL;

    /* 将新节点插入到尾部*/
    p_node = pNew_node;
}

/* 解析读取到的管道消息*/
int parse_param(char *buff)
{
    char *ptr = buff;
    int i = 0, j = 0;
    static char argv[32][32];

    if( !buff )
    {
        return -1;
    }

    while( *ptr != '\0' )
    {
        /* 说明可能有参数*/
        if( *ptr == ' ' )
        {
            if( j > 0 )
            {
                /* 给前一个参数结尾*/
                argv[i][j] = '\0';
                argvs[i] = argv[i];
                i++;

                /* j 清0，下一个参数开始*/
                j = 0;
                memset( argv[i], 0x00, 32 );
            }
            ptr++;
            continue;
        }

        /* 如果命令的某个参数的长度超过32个字符，出错*/
        if( j > 31 )  return -1;
        /* 如果命令的参数个数超过32个，出错*/
        if( i > 31 )  return -1;

        /* 为第 i 下标个参数赋值，是一个以\0结尾的字符串*/
        argv[i][j++] = *ptr++;
        if( *ptr == '\0' && j > 0 )
        {
            argv[i][j] = '\0';
            argvs[i] = argv[i];
            i++;
        }
    }

    argvs[i] = NULL;

    /* 记录参数个数*/
    argcs = i;

    return 0;
}

/* 获取命令结点*/
ctrl_node_t* get_ctrl_node(char *pKey)
{
    ctrl_node_t *p_node = NULL;

    if( !pKey )
    {
        return NULL;
    }

    for( p_node = pCtrl_head; p_node != NULL; p_node = p_node->m_pNext )
    {
        if( ! strcmp(p_node->m_key, pKey) )
        {
            break;
        }
    }

    return p_node;
}

/* 打印管控模块所支持的命令信息*/
void ctrl_usage()
{
    ctrl_node_t *p_node = NULL;

    ERR("\n");
    ERR("---------------------------------------\n");
    ERR("name\t\texplain\n");
    ERR("---------------------------------------\n");
    for( p_node = pCtrl_head->m_pNext; p_node != NULL; p_node = p_node->m_pNext )
    {
        ERR("%s\t\t%s\n", p_node->m_key, p_node->m_info);
    }
}

/* 管控线程函数*/
void *ctrl_start(void *arg)
{
    char *fifo_file_path = (char *)arg;

    /* 将线程分离*/
    pthread_detach( pthread_self() );

    /* 创建管道文件*/
    if( mkfifo(fifo_file_path, 0666) < 0 )
    {
        if(errno != EEXIST)
        {
            perror("mkfifo error:");
            return NULL;
        }
    }

    /* 阻塞打开管道文件*/
    int fd = open( fifo_file_path, O_RDWR );
    int ret;
    char buff[1024];
    ctrl_node_t *pCtrl_node = NULL;

    while( 1 )
    {
        /* 阻塞读取管道消息*/
        if( (ret = read(fd, buff, 1024)) <= 0 )
        {
            return NULL;
        }
        buff[ret - 1] = '\0';

        /* 解析读取到的管道消息, 得到命令和其参数列表*/
        if( parse_param(buff) < 0 )
        {
            ERR("Format of parameters is wrong!-[%s]\n", buff);
            continue;
        }

        /* 获取命令结点*/
        if( (pCtrl_node = get_ctrl_node( argvs[0] )) == NULL )
        {
            ctrl_usage();
            ERR("Invalid parameters :[%s]\n", argvs[0]);
            continue;
        }

        /* 调用命令所对应的回调函数*/
        pCtrl_node->m_bc_func( argcs, argvs );
    }
}

/* 管控模块初始化*/
int ctrl_init(char *fifo_file_path)
{
    pthread_t tid;

    /* 创建管控线程*/
    if( pthread_create( &tid, NULL, ctrl_start, (void *)fifo_file_path ) )
    {
        ERR("init ctrl failed!");
        return -1;
    }

    return 0;
}

