/*************************************************************************
	> File Name: client.c
	> Author: 
	> Mail: 
	> Created Time: 2018年01月23日 星期二 08时28分18秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#include "./common.h"

int main(void)
{
    int sock_fd;
    struct sockaddr_in  serv_addr;

    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    //配置服务器地址和端口
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8000);
    serv_addr.sin_addr.s_addr = inet_addr("192.168.43.205");
    
    
    //连接服务器
    if(connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        exit(1);
    }

    char buff[1024]= {0};
    int i = 0;
    int ret;

    while(1)
    {
        if(send(sock_fd, buff, 1024, 0) == -1)
        {
            perror("send");
            exit(1);
        }
        if((ret = recv(sock_fd, buff, 1024, 0)) == 0)
        {
            printf("server closed\n");
            exit(0);
        }
        else if(ret == -1)
        {
            perror("recv");
            exit(1);
        }
    }

    return 0;
}
