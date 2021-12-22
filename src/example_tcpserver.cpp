#include "co_routine.h"

#include <stdio.h>
#include <stdlib.h>
#include <stack>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#ifdef __FreeBSD__
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#endif


struct stTcpServerEnv
{
    int listenfd;
};

struct stTcpClientEnv
{
    stCoRoutine_t * co;
    int clientfd;
    char buf[1024 * 16];
};


static int SetNonBlock(int iSock)
{
    int iFlags;

    iFlags = fcntl(iSock, F_GETFL, 0);
    iFlags |= O_NONBLOCK;
    iFlags |= O_NDELAY;
    int ret = fcntl(iSock, F_SETFL, iFlags);
    return ret;
}

static void SetAddr(const char *pszIP,const unsigned short shPort,struct sockaddr_in &addr)
{
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(shPort);
    int nIP = 0;
    if( !pszIP || '\0' == *pszIP
        || 0 == strcmp(pszIP,"0") || 0 == strcmp(pszIP,"0.0.0.0")
        || 0 == strcmp(pszIP,"*")
            )
    {
        nIP = htonl(INADDR_ANY);
    }
    else
    {
        nIP = inet_addr(pszIP);
    }
    addr.sin_addr.s_addr = nIP;

}

static int CreateTcpSocket(const unsigned short shPort /* = 0 */,const char *pszIP /* = "*" */,bool bReuse /* = false */)
{
    int fd = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
    if( fd >= 0 )
    {
        if(shPort != 0)
        {
            if(bReuse)
            {
                int nReuseAddr = 1;
                setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&nReuseAddr,sizeof(nReuseAddr));
            }
            struct sockaddr_in addr ;
            SetAddr(pszIP,shPort,addr);
            int ret = bind(fd,(struct sockaddr*)&addr,sizeof(addr));
            if( ret != 0)
            {
                close(fd);
                return -1;
            }
        }
    }
    return fd;
}

void * client_coroutine(void * argv)
{
    co_enable_hook_sys();

    stTcpClientEnv * env = (stTcpClientEnv*)argv;
    for (;;)
    {
        // sleep 1 second
        struct pollfd pf = { 0 };
        pf.fd = env->clientfd;
        pf.events = (POLLIN|POLLERR|POLLHUP);
        co_poll( co_get_epoll_ct(),&pf,1,1000);

        int ret = read( env->clientfd, env->buf,sizeof(env->buf) );
        if( ret > 0 )
        {
            // printf("server recv fd %d message:%s\n", env->clientfd, env->buf);
            ret = write( env->clientfd, env->buf,ret );
        }
        if( ret <= 0 )
        {
            // printf("readwrite_routine fd: %d error\n", env->clientfd);

            // accept_routine->SetNonBlock(fd) cause EAGAIN, we should continue
            if (ret < 0 && errno == EAGAIN)
                continue;

            close( env->clientfd );
            //shutdown( fd, SHUT_RDWR );

            printf("readwrite_routine fd: %d closed\n", env->clientfd);

            break;
        }
    }
    delete env;
    return nullptr;
}

void * accept_routine(void * argv )
{
    co_enable_hook_sys();
    stTcpServerEnv * env = (stTcpServerEnv *)argv;

    printf("accept_routine\n");
    fflush(stdout);
    for(;;)
    {
        struct sockaddr_in addr; //maybe sockaddr_un;
        memset( &addr,0,sizeof(addr) );
        socklen_t len = sizeof(addr);

        int fd = co_accept(env->listenfd, (struct sockaddr *)&addr, &len);
        if ( fd < 0 )
        {
            struct pollfd pf = { 0 };
            pf.fd = env->listenfd;
            pf.events = (POLLIN|POLLERR|POLLHUP);
            co_poll( co_get_epoll_ct(),&pf,1,10 );
            continue;
        }
        printf("accept fd %d\n", fd);
        SetNonBlock( fd );

        stTcpClientEnv * clientEnv = new stTcpClientEnv;
        memset(clientEnv,0,sizeof(clientEnv));
        clientEnv->clientfd = fd;
        co_create( &clientEnv->co, NULL, client_coroutine, clientEnv);
        co_resume( clientEnv->co);
    }

    return nullptr;
}

int main(int argc, char **argv)
{
    co_start_hook();

    if(argc<2)
    {
        printf("Usage: example_tcpserver [IP] [PORT]\n");
        return -1;
    }
    const char *ip = argv[1];
    int port = atoi( argv[2] );

    int listenfd = CreateTcpSocket( port,ip,true );
    listen( listenfd,1024 );
    if(listenfd == -1)
    {
        printf("Port %d is in use\n", port);
        return -1;
    }
    printf("listen %d %s:%d\n",listenfd,ip,port);

    SetNonBlock( listenfd );

    stShareStack_t* share_stack= co_alloc_sharestack(1, 1024 * 128);
    stCoRoutineAttr_t attr;
    attr.stack_size = 0;
    attr.share_stack = share_stack;

    stTcpServerEnv * env = new stTcpServerEnv;
    env->listenfd = listenfd;

    stCoRoutine_t *accept_co = NULL;
    //co_create( &accept_co,NULL,accept_routine,0 );
    co_create( &accept_co,&attr,accept_routine,env );
    co_resume( accept_co );

    co_eventloop( co_get_epoll_ct(),0,0 );

    return 0;
}