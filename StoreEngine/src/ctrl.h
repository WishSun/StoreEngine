/*************************************************************************
	> File Name: ctrl.h
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月11日 星期三 01时33分16秒
 ************************************************************************/

#ifndef _CTRL_H
#define _CTRL_H

/* 定义回调函数类型*/
typedef void (*bc_func_t)(int argc, char *argv[]);

/* 命令结点结构*/
typedef struct _ctrl_node_t
{
    char                 m_key[32];  /* 命令*/
    char                 m_info[128];/* 命令说明*/
    bc_func_t            m_bc_func;  /* 命令回调函数*/
    struct _ctrl_node_t  *m_pNext;   /* 下一个命令结点*/
}ctrl_node_t;

/* 向命令控制链表中添加结点*/
void ctrl_node_add(char *key, char *info, bc_func_t func);

/* 管控模块初始化*/
int ctrl_init(char *fifo_file_path);
#endif
