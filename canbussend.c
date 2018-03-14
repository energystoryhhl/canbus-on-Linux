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

#include <net/if.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "lib.h"
#include "main.h"

extern Cthread_sendworkqueue *sndqueue;
extern char * interface ;

int canbus_send(char *interface,char *snd_msg)
{
    #ifdef DEBUG
    printf("***msg is :%s\n",snd_msg);
    printf("***interface is :%s\n",interface);
    #endif // DEBUG


	int s;                       /* can raw socket */
	int required_mtu;           //传输内容的最大传输内容
	int mtu;
	int enable_canfd = 1;       //开启关闭flag位
	struct sockaddr_can addr;   //can总线的地址 同socket编程里面的 socketaddr结构体 用来设置can外设的信息
	struct canfd_frame frame;   //要发送的buffer
	struct ifreq ifr;           //接口请求结构体

	/* check command line options  //检测命令行参数是不是3个
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <device> <can_frame>.\n", argv[0]);
		return 1;
	}
    */

	/* parse CAN frame */
	required_mtu = parse_canframe(snd_msg, &frame);//检验要发送的can报文是否符合标准
	if (!required_mtu){
		fprintf(stderr, "\nWrong CAN-frame format! Try:\n\n");
		fprintf(stderr, "    <can_id>#{R|data}          for CAN 2.0 frames\n");
		fprintf(stderr, "    <can_id>##<flags>{data}    for CAN FD frames\n\n");
		fprintf(stderr, "<can_id> can have 3 (SFF) or 8 (EFF) hex chars\n");
		fprintf(stderr, "{data} has 0..8 (0..64 CAN FD) ASCII hex-values (optionally");
		fprintf(stderr, " separated by '.')\n");
		fprintf(stderr, "<flags> a single ASCII Hex value (0 .. F) which defines");
		fprintf(stderr, " canfd_frame.flags\n\n");
		fprintf(stderr, "e.g. 5A1#11.2233.44556677.88 / 123#DEADBEEF / 5AA# / ");
		fprintf(stderr, "123##1 / 213##311\n     1F334455#1122334455667788 / 123#R ");
		fprintf(stderr, "for remote transmission request.\n\n");
		return 1;
	}

    /* 创建socket套接字
    PF_CAN 为域位 同网络编程中的AF_INET 即ipv4协议
    SOCK_RAW使用的协议类型 SOCK_RAW表示原始套接字 报文头由自己创建
    CAN_RAW为使用的具体协议 为can总线协议
    */
	/* open socket */
	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("socket");
		return 1;
	}

	/*接口请求结构用于套接字ioctl的所有接口
    ioctl的一定参数的定义开始ifr_name。
    其余部分可能是特定于接口的。*/
	strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);   //给借口请求结构体赋予名字
	ifr.ifr_name[IFNAMSIZ - 1] = '\0';              //字符串结束位
	ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name); //指定网络接口名称字符串作为参数；若该接口存在，则返回相应的索引，否则返回0 就是检验该接口是否存在！
	if (!ifr.ifr_ifindex) {
		perror("if_nametoindex");
		return 1;
	}

	addr.can_family = AF_CAN;                       //协议类型
	addr.can_ifindex = ifr.ifr_ifindex;             //can总线外设的具体索引 类似 ip地址

	if (required_mtu > CAN_MTU) {                   //Maximum Transmission Unit最大传输单元超过了

		/* check if the frame fits into the CAN netdevice */
		if (ioctl(s, SIOCGIFMTU, &ifr) < 0) {       //获取can借口最大传输单元
			perror("SIOCGIFMTU");
			return 1;
		}
		mtu = ifr.ifr_mtu;

		if (mtu != CANFD_MTU) {                     //和canbus最大传输单元不符合 就报错
			printf("CAN interface is not CAN FD capable - sorry.\n");
			return 1;
		}

		/* interface is ok - try to switch the socket into CAN FD mode */
		if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,       //用于任意类型、任意状态套接口的设置选项值
			       &enable_canfd, sizeof(enable_canfd))){
			printf("error when enabling CAN FD support\n");
			return 1;
		}

		/* ensure discrete CAN FD length values 0..8, 12, 16, 20, 24, 32, 64 */
		frame.len = can_dlc2len(can_len2dlc(frame.len));
	}

	/* disable default receive filter on this RAW socket */
	/* This is obsolete as we do not read from the socket at all, but for */
	/* this reason we can remove the receive list in the Kernel to save a */
	/* little (really a very little!) CPU usage.                          */
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0); //禁用过滤

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) { //绑定套接字 就是 套接字和canbus外设进行绑定
		perror("bind");
		return 1;
	}

	/* send frame */
	if (write(s, &frame, required_mtu) != required_mtu) {       //发送数据！
		perror("write");
		return 1;
	}

	//close(s);                                                   //关闭套接字

	return 0;
}

/*canbus发送初始化
返回值：工作队列结构体指针
*/
Cthread_sendworkqueue * sendworkqueue_init()
{

    Cthread_sendworkqueue *sendqueue = (Cthread_sendworkqueue *) malloc (sizeof (Cthread_sendworkqueue)); //为工作队列分配内存
    pthread_mutex_init (&(sendqueue->sendqueue_lock), NULL);             //初始化互斥锁
    pthread_cond_init (&(sendqueue->sendqueue_ready), NULL);             //初始化条件变量
    sendqueue->sendqueue_head = NULL;                                    //初始化工作队列头
    sendqueue->cur_sendtask_size = 0;
    sendqueue->sndthreadid =(pthread_t *) malloc (sizeof (pthread_t));   //创建线程id

    return sendqueue;
}

/*加入数据发送任务
参数：要发送的数据
*/
int snd_add_task (void *arg,Cthread_sendworkqueue * sndqueue)
{
    Cthread_sendtask *new_task = (Cthread_sendtask *) malloc (sizeof (Cthread_sendtask));//构造一个新任务
    //new_task->canbussend = canbussend;      //执行的任务
    new_task->arg = arg;                    //参数
    new_task->next = NULL;

    pthread_mutex_lock (&(sndqueue->sendqueue_lock));
    /*将任务加入到等待队列中*/
    Cthread_sendtask *member = sndqueue->sendqueue_head;
    if (member != NULL)
    {
        while (member->next != NULL)        //while循环查找任务末尾
            member = member->next;
        member->next = new_task;
    }
    else
    {
        sndqueue->sendqueue_head = new_task;
    }

    sndqueue->cur_sendtask_size++;
    pthread_mutex_unlock (&(sndqueue->sendqueue_lock)); //工作队列数量++

    pthread_cond_signal (&(sndqueue->sendqueue_ready)); //通知线程ok

    return 0;
}

void * snd_thread(void *arg)
{
    #ifdef DEBUG
    printf ("***SEND PTHREAD:starting thread 0x%x\n", pthread_self ());
    #endif // DEBUG
    while (1)
    {
        pthread_mutex_lock (&(sndqueue->sendqueue_lock));

        while (sndqueue->cur_sendtask_size == 0 && !sndqueue->sendshutdown) //如果没有任务就等待
        {
            #ifdef DEBUG
            printf ("***SEND PTHREAD 0x%x is waiting\n", pthread_self ());
            #endif // DEBUG


            pthread_cond_wait (&(sndqueue->sendqueue_ready), &(sndqueue->sendqueue_lock)); //等待！
        }

        /*线程池要销毁了*/
        if (sndqueue->sendshutdown)
        {
            pthread_mutex_unlock (&(sndqueue->sendqueue_lock));
            printf ("***SEND PTHREAD 0x%x  will exit\n", pthread_self ());
            pthread_exit (NULL);
        }
        #ifdef DEBUG
        printf ("***SEND PTHREAD 0x%x  is starting to work\n", pthread_self ());
        #endif // DEBUG

        //开始处理任务
        sndqueue->cur_sendtask_size--;
        Cthread_sendtask *task = sndqueue->sendqueue_head;
        sndqueue->sendqueue_head = task->next;

        pthread_mutex_unlock (&(sndqueue->sendqueue_lock));

        //调用回调函数，执行任务
        canbus_send(interface,task->arg);

        free (task);
        task = NULL;
    }
    /*这一句应该是不可达的*/
    pthread_exit (NULL);

}

//创建发送线程
int snd_thread_create(Cthread_sendworkqueue * sndqueue)
{
   pthread_create(sndqueue->sndthreadid, NULL, snd_thread, NULL);
   return 0;
}













