/* A simple event-driven programming library. Originally I wrote this code
 * for the Jim's event-loop (Jim is a Tcl interpreter) but later translated
 * it in form of a library for easy reuse.
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __AE_H__
#define __AE_H__

#include "monotonic.h"

#define AE_OK 0
#define AE_ERR -1

/**
 * @brief 
 * 
 * No events registered.
 * 事件类型 未注册
 */
#define AE_NONE 0
/**
 * @brief 
 * 
 * Fire when descriptor is readable.
 * 事件类型 读
 */
#define AE_READABLE 1
/**
 * @brief 
 * 
 * Fire when descriptor is writable.
 * 事件类型 写
 */
#define AE_WRITABLE 2
/**
 * @brief 
 * 
 * With WRITABLE, never fire the event if the
 * READABLE event already fired in the same event
 * loop iteration. Useful when you want to persist
 * things to disk before sending replies, and want
 * to do that in a group fashion.
 */
#define AE_BARRIER 4

/**
 * @brief 
 * 
 * 监控的事件源的类型：网络
 */
#define AE_FILE_EVENTS (1<<0)
/**
 * @brief 
 * 
 * 监控的事件源的类型：定时器
 */
#define AE_TIME_EVENTS (1<<1)
/**
 * @brief 
 * 
 * 监控的事件源的类型：全部
 */
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)
/**
 * @brief 
 * 
 * 监控的事件源的类型：非阻塞标识（在select()时，设置timeout为0）
 */
#define AE_DONT_WAIT (1<<2)
/**
 * @brief 
 * 
 * 监控的事件源的类型：sleep 之前调用
 */
#define AE_CALL_BEFORE_SLEEP (1<<3)
/**
 * @brief 
 * 
 * 监控的事件源的类型：sleep 之后调用
 */
#define AE_CALL_AFTER_SLEEP (1<<4)

#define AE_NOMORE -1
#define AE_DELETED_EVENT_ID -1

/* Macros */
#define AE_NOTUSED(V) ((void) V)

struct aeEventLoop;

/* Types and data structures */
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);
typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);

/**
 * @brief 
 * 
 * File event structure
 * 客户都fd读写事件
 */
typedef struct aeFileEvent {
    int mask; /* one of AE_(READABLE|WRITABLE|BARRIER) */
    aeFileProc *rfileProc;
    aeFileProc *wfileProc;
    void *clientData;
} aeFileEvent;

/* Time event structure */
typedef struct aeTimeEvent {
	/**
	 * @brief 
	 * 
	 * time event identifier.
	 * 本定时器对象的id
	 */
    long long id;

	/**
	 * @brief 
	 * 
	 * 定时器执行时间
	 */
    monotime when;

	/**
	 * @brief 
	 * 
	 * 定时回调函数
	 */
    aeTimeProc *timeProc;

	/**
	 * @brief 
	 * 
	 * 定时器销毁时，调用的清理函数
	 */
    aeEventFinalizerProc *finalizerProc;

	/**
	 * @brief 
	 * 
	 * 定时器回调函数参数
	 */
    void *clientData;

	/**
	 * @brief 
	 * 
	 * 链表 上一个定时器
	 */
    struct aeTimeEvent *prev;

	/**
	 * @brief 
	 * 
	 * 链表 下一个定时器
	 */
    struct aeTimeEvent *next;
    int refcount; /* refcount to prevent timer events from being
  		   * freed in recursive time event calls. */
} aeTimeEvent;

/* A fired event */
typedef struct aeFiredEvent {
    int fd;
    int mask;
} aeFiredEvent;

/* State of an event based program */
typedef struct aeEventLoop {
    int maxfd;   /* highest file descriptor currently registered */
    int setsize; /* max number of file descriptors tracked */

	/**
	 * @brief 
	 * 
	 * 定时器事件id，自增，用于给新创建定时器编号
	 */
    long long timeEventNextId;
    aeFileEvent *events; /* Registered events */
    aeFiredEvent *fired; /* Fired events */

	/**
	 * @brief 
	 * 
	 * 定时器事件的链表头
	 */
    aeTimeEvent *timeEventHead;

	/**
	 * @brief 
	 * 
	 * 事件循环while退出条件
	 */
    int stop;
    void *apidata; /* This is used for polling API specific data */
    aeBeforeSleepProc *beforesleep;
    aeBeforeSleepProc *aftersleep;
    int flags;
} aeEventLoop;

/* Prototypes */
aeEventLoop *aeCreateEventLoop(int setsize);

/**
 * @brief 
 * 
 * 销毁事件循环，退出redis进程
 * @param eventLoop 
 */
void aeDeleteEventLoop(aeEventLoop *eventLoop);
void aeStop(aeEventLoop *eventLoop);
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData);
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);
int aeGetFileEvents(aeEventLoop *eventLoop, int fd);
void *aeGetFileClientData(aeEventLoop *eventLoop, int fd);
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc);
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);
int aeProcessEvents(aeEventLoop *eventLoop, int flags);
int aeWait(int fd, int mask, long long milliseconds);

/**
 * @brief 
 * 
 * 定时/网络读写事件多路复用监听
 * 调用后，遍历eventLoop下面挂载的网络事件链表、定时器链表。
 * 使用定时器链表中最近的超市时间，作为select()的最后一个参数，去监听全部的客户端fd，
 * 处理客户端事件后，查询定时器事件并处理。
 * 继续while (!eventLoop->stop)循环，直到stop==true退出循环
 * @param eventLoop 
 */
void aeMain(aeEventLoop *eventLoop);
char *aeGetApiName(void);
void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep);
void aeSetAfterSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *aftersleep);
int aeGetSetSize(aeEventLoop *eventLoop);
int aeResizeSetSize(aeEventLoop *eventLoop, int setsize);
void aeSetDontWait(aeEventLoop *eventLoop, int noWait);

#endif
