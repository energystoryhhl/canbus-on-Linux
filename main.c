#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "main.h"

Cthread_sendworkqueue * sndqueue;   //全局发送队列结构体
char * interface = "can0";          //指定设备
struct epoll_event *events;         //用来进行监听的结构体数组

//char **recv_set;
//recv_set[0] = "candump";

void handle(char * mse_in)
{
    snd_add_task(mse_in,sndqueue);  //收到消息 进行发送
}

void menu()
{
    char msg_in[50] = "0";
    int first_flag = 1;
    while(1)
    {
        if(1==first_flag)
        {
            printf("int put the canbus meg!\n");
            first_flag = 0;
        }
        printf(">>>");
        scanf("%s",msg_in);
        #ifdef DEBUG
        //printf("in put mes is:%s\n",msg_in);
        #endif // DEBUG
        handle(msg_in);
    }
}

void canrecv_thread()
{
    //canrecv(recvset);
}

int main(int argc,char **argv)
{
    char* recvset[2];
    recvset[0] = "candump";
    recvset[1] = "can0";
    if(argc >= 2)
    {
        interface = argv[1];
        recvset[1]  = argv[1];
        printf("setting interface is %s\n",argv[1]);
    }
    sndqueue = (Cthread_sendworkqueue *)sendworkqueue_init();   //实例化一个工作队列！
    snd_thread_create(sndqueue);                                //创建发送线程

//    pthread_t canrecv_thread;                                   //创建接收线程
//    pthread_create(&canrecv_thread,NULL,(void*(*)(void*))canrecv_thread,NULL);

    pthread_t menu_thread;                                                  //菜单线程的创建
    pthread_create(&menu_thread,NULL,(void*(*)(void*))menu,NULL);
    #ifdef RCV
    canrecv(recvset);
    #endif // debug


//    pthread_join(canrecv_thread,NULL);
    pthread_join(menu_thread,NULL);

    return 0;
}











