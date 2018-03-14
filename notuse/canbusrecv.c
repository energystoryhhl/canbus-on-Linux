
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
#include <sys/epoll.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "main.h"


int epfd; //epoll机制的描述符

struct epoll_event can_epoll_event; //用来设置监听事件的结构体


/*epoll监听池初始化
返回值：epoll池 描述符
参数：需要监听的文件描述符
*/
int epoll_init(int fd)
{
    int epfd = epoll_create(0);//创建监听池
    can_epoll_event.events = EPOLLIN|EPOLLET; //监听事件为 ： 是否可读 出发方式：边缘触发
    can_epoll_event.data.fd = fd;//监听的文件描述符
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&can_epoll_event);
    return epfd;
}

int canrecv_init(char *interface)
{
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;
    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);//创建 SocketCAN 套接字
    strcpy(ifr.ifr_name, interface );
    ioctl(s, SIOCGIFINDEX, &ifr);//指定 can0 设备
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(s, (struct sockaddr *)&addr, sizeof(addr)); //将套接字与 can0 绑定
}

