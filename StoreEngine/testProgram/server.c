/*************************************************************************
	> File Name: server.c
	> Author: 
	> Mail: 
	> Created Time: 2018年01月23日 星期二 05时05分50秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <pthread.h>

#include "./common.h"


//处理与客户端通信的线程函数
void *clientRoute(void *args) 
{
    //分离线程
    pthread_detach(pthread_self());

    //获取与客户端通信的套接字描述符
    int cli_fd = *(int *)args;

    char buff[1024] = {0};
    int ret;
    while(1)
    {
        if((ret = recv(cli_fd, buff, 1024, 0)) == 0)
        {
            printf("client closed!\n");
            close(cli_fd);
            pthread_exit(NULL);
        }
        else if(ret == -1)
        {
            if(errno == EAGAIN || errno == EINTR)
            {//当由于client_fd为准备好而出错或被信号打断而出错，不是真正出错
                 continue;
            }
            close(cli_fd);
            continue;
        }

        printf("已收到 %d 个字节\n", ret);
        //将接收到的内容返送给客户端
        if(send(cli_fd, buff, 1024, 0) == -1)
        {
            perror("send"); 
            pthread_exit(NULL);
        }
    }
}

int main(void)
{
    int listen_fd;

    //创建服务器监听套接字,用于监听客户端的连接请求
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }


    //配置服务器端口和地址
    struct sockaddr_in serv_addr;

    //指定使用IPv4协议族
    serv_addr.sin_family = AF_INET;

    //htons将短整型数据从主机字节序转换为网络字节序
    serv_addr.sin_port = htons(8000);

    //inet_addr将字符串主机地址(我的是小端字节序，即低地址存低位)转换为网络字节序(大端字节序，即低地址存高位)
    serv_addr.sin_addr.s_addr = inet_addr("192.168.43.205");

    //为服务器绑定地址和端口
    if(bind(listen_fd, (struct sockaddr*)(&serv_addr),sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    //在指定端口和地址上监听客户端连接, 最大监听客户端数为10
    if(listen(listen_fd, 10) == -1)
    {
        perror("listen");
        exit(1);
    }

    while(1)
    {
        int len = sizeof(struct sockaddr_in);
        struct sockaddr_in cli_addr;
        int client_fd;

        //len用于指定客户端地址信息的长度, accept用它来确定接收客户端地址数据的大小
        //accept函数执行完之后，len中存放实际接收的客户端地址的大小
        if((client_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &len)) == -1)
        {
            perror("accept"); 
            exit(1);
        }

        //inet_ntoa将网络字节序地址转为字符串， ntohs将网络字节序端口转为整型数据
        printf("new connect ip:[%s:%d]\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));


        //创建一个与客户端通信的线程，将通信套接字描述符通过参数传递进去
        pthread_t cli_tid;
        if(pthread_create(&cli_tid, NULL, clientRoute, &client_fd) != 0)
        {
            perror("pthread_create");
            close(client_fd);
        }
    }


    //关闭监听套接字
    close(listen_fd);
    
    return 0;
}
