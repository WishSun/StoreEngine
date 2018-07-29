/*************************************************************************
	> File Name: common.h
	> Author: WishSun
	> Mail: WishSun_Cn@163.com
	> Created Time: 2018年07月15日 星期日 18时37分47秒
 ************************************************************************/

#ifndef _COMMON_H
#define _COMMON_H

#define SIZE_M (1>>20)

typedef struct 
{
    char ptr[ SIZE_M ];
    char ptr_len;
}data_t;

#endif
