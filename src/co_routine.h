/*
* Tencent is pleased to support the open source community by making Libco available.

* Copyright (C) 2014 THL A29 Limited, a Tencent company. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License"); 
* you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at
*
*	http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, 
* software distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and 
* limitations under the License.
*/

#ifndef __CO_ROUTINE_H__
#define __CO_ROUTINE_H__

#include <stdint.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <pthread.h>
#include <netinet/in.h>

//1.struct

struct stCoRoutine_t;
struct stShareStack_t;

struct stCoRoutineAttr_t
{
	int stack_size;
	stShareStack_t*  share_stack;
	stCoRoutineAttr_t()
	{
		stack_size = 128 * 1024;
		share_stack = NULL;
	}
}__attribute__ ((packed));

struct stCoEpoll_t;
typedef int (*pfn_co_eventloop_t)(void *);
typedef void *(*pfn_co_routine_t)( void * );

//2.co_routine

int 	co_create( stCoRoutine_t **co,const stCoRoutineAttr_t *attr,void *(*routine)(void*),void *arg );
void    co_resume( stCoRoutine_t *co );
void    co_yield( stCoRoutine_t *co );
void    co_yield_ct(); //ct = current thread
void    co_release( stCoRoutine_t *co );
void    co_reset(stCoRoutine_t * co);

// add by lxk
void    co_activate( stCoRoutine_t *co );
void    co_yield_timeout( stCoRoutine_t *co , int timeout_ms );
void    co_yield_ct_timeout( int timeout_ms );

stCoRoutine_t *co_self();

int		co_poll( stCoEpoll_t *ctx, struct pollfd fds[], nfds_t nfds, int timeout_ms );

// change by lxk here
void 	co_eventloop( stCoEpoll_t *ctx, pfn_co_eventloop_t pfn, void *arg );

void co_eventloop_once(stCoEpoll_t *ctx, int timeout_ms);

//3.specific

int 	co_setspecific( pthread_key_t key, const void *value );
void *	co_getspecific( pthread_key_t key );

//4.event

stCoEpoll_t * 	co_get_epoll_ct(); //ct = current thread

//5.hook syscall ( poll/read/write/recv/send/recvfrom/sendto )

void 	co_enable_hook_sys();  
void 	co_disable_hook_sys();  
bool 	co_is_enable_sys_hook();

//6.sync
struct stCoCond_t;

stCoCond_t *co_cond_alloc();
int co_cond_free( stCoCond_t * cc );

int co_cond_signal( stCoCond_t * );
int co_cond_broadcast( stCoCond_t * );
int co_cond_timedwait( stCoCond_t *,int timeout_ms );

//7.share stack
stShareStack_t* co_alloc_sharestack(int iCount, int iStackSize);

//8.init envlist for hook get/set env
void co_set_env_list( const char *name[],size_t cnt);

void co_log_err( const char *fmt,... );

//9.add by lxk here
int co_accept(int fd, struct sockaddr *addr, socklen_t *len );
int co_register_fd(int fd);
int co_set_timeout(int fd, int read_timeout_ms, int write_timeout_ms);
int co_set_nonblock(int fd);

bool co_is_runtime_busy(); // 当运行时当前循环周期超过100毫秒时返回true

pid_t GetPid();

// 加入start_hook方法是为了在使用LD_PRELOAD=libco时getenv被提前调用导致调用到getpid出错，因此加入开关，等进程初始化完成进入代码逻辑时才打开开关
void co_start_hook();

// yield前检查当前co是否超过stacksize栈长度->返回true成功,stacksize 栈报警值 ,curstacklen返回占用栈长度
bool co_check_yield( stCoRoutine_t *co ,int stack_size,int &curstackLen);
// resume前检查当前co是否超过stacksize栈长度->返回true成功,stacksize 栈报警值 ,curstacklen返回占用栈长度
bool co_check_resume( stCoRoutine_t *co ,int stack_size,int &curstac_klen);
// 返回co占用的堆栈长度
int co_get_stack_len( stCoRoutine_t *co );
int co_get_cur_stack_len();

bool co_end(stCoRoutine_t *co);


#endif

