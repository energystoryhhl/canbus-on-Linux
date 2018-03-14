#ifndef _CANBUSSEND_H_
#define	_CANBUSSEND_H_

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

#include <linux/can.h>
#include <linux/can/raw.h>


int canbussend(void *arg);



/*工作*/
typedef struct sendtask
{

    int (*canbussend)(void *arg);     //工作队列执行的任务
    void * arg;                        //工作队列参数
    struct sendtask *next;          //下一个工作
} Cthread_sendtask;

/*工作队列*/
typedef struct sendworkqueue
{
    pthread_mutex_t sendqueue_lock; //互斥锁
    pthread_cond_t  sendqueue_ready;//
    pthread_t *sndthreadid;         //发送线程id

    /*链表结构，线程池中所有等待任务*/
    Cthread_sendtask *sendqueue_head;

    /*是否销毁线程池*/
    int sendshutdown;
    // pthread_t *threadid;

    /*当前等待的任务数*/
    int cur_sendtask_size;

} Cthread_sendworkqueue;


#endif // _CANBUSSEND_H_


