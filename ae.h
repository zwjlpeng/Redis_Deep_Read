/* A simple event-driven programming library. Originally I wrote this code
 * for the Jim's event-loop (Jim is a Tcl interpreter) but later translated
 * it in form of a library for easy reuse.
 *
 * Copyright (c) 2006-2009, Salvatore Sanfilippo <antirez at gmail dot com>
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

/**
 * redis里的事件驱动模型 
 * Redis的事件模型在不同的操作系统中提供了不同的实现,ae_epoll.h/ae_epool.c
 * 为epool的实现,ae_select.h/ae_select.c是select的实现
 * ae_kqueue.h/ae_kqueue.c是bsd的kqueue的实现
 */

/**
 * Redis提供两种基本事件：FileEvent/TimeEvent,前者是基于操作系统的异步
 * 机制(epoll/kqueue)实现的文件事件，后者是Redis自已实现的定时器
 */
#ifndef __AE_H__
#define __AE_H__

/**
 * redis 1.0里面只使用了select函数进行异步操作，这也决定了
 * 其最大能够响应的文件事件个数
 * 同时select函数在windows下与linux下也是有区别的
 * 在windows下第一个参数是可以进行忽略不计的
 */
/**
 * 事件循环结构体
 */
 /**
  * redis 1.0里面只使用了select函数，
  * 即使用操作系统提供的异步操作
  * 定时器是在文件事件之后进行处理
  */
struct aeEventLoop;

/* Types and data structures */
//定义函数指针
/**
 * aeFileProc表示的是文件事件的处理句柄
 * aeTimeProc为定时器事件的处理句柄 
 * aeEventFinalizerProc为事件结束时调用的析构函数
 */
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);

/* File event structure */
/**
 * 基于操作系统异步机制实现的文件事件
 */
typedef struct aeFileEvent {
    int fd;
    /**
     * AE_READABLE|AE_WRITEABLE_AE_EXCEPTION中的一个
     * 表示要监听的事件类型
     */
    int mask; /* one of AE_(READABLE|WRITABLE|EXCEPTION) */
    //文件事件相应的处理函数
    aeFileProc *fileProc;
    aeEventFinalizerProc *finalizerProc;
    void *clientData;
    //下一个文件事件
    struct aeFileEvent *next;
} aeFileEvent;

/* Time event structure */
/**
 * redis自已定义的定时器事件
 * 其实现是一个链表，其中的每一个结点是一个Timer
 * when_sec与when_ms指定了定时器发生的时间
 * timeProc为响应函数
 * finalizerProc为删除定时器的析构函数
 */
typedef struct aeTimeEvent {
    //定时器的id
    long long id; /* time event identifier. */
    long when_sec; /* seconds */
    long when_ms; /* milliseconds */
    aeTimeProc *timeProc;
    aeEventFinalizerProc *finalizerProc;
    //定义了该定时器有的数据情况
    void *clientData;
    struct aeTimeEvent *next;
} aeTimeEvent;

/* State of an event based program */
/**
 * 事件循环结构体
 */
typedef struct aeEventLoop {
    //用于标识下一个定时器
    long long timeEventNextId;
    //文件事件
    aeFileEvent *fileEventHead;
    //定时器事件
    aeTimeEvent *timeEventHead;
    //stop用于停止事件轮询
    int stop;
} aeEventLoop;

/* Defines */
/**
 * 一系列的宏定义
 */
//正确码
#define AE_OK 0
//错误码
#define AE_ERR -1

//表事属于读事件
#define AE_READABLE 1
//表示属于写事件
#define AE_WRITABLE 2
//表示发生了异常事件
#define AE_EXCEPTION 4
//表示文件事件
#define AE_FILE_EVENTS 1
//表示定时事件
#define AE_TIME_EVENTS 2
//表示所有的事件
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)
//状态标志
#define AE_DONT_WAIT 4
//没事事件标记
#define AE_NOMORE -1

/* Macros */
//未使用标记
#define AE_NOTUSED(V) ((void) V)

/* Prototypes */
/**
 * 创建事件循环
 */
aeEventLoop *aeCreateEventLoop(void);
/**
 * 释放事件循环
 */
void aeDeleteEventLoop(aeEventLoop *eventLoop);
/**
 * 停止事件
 */
void aeStop(aeEventLoop *eventLoop);
/**
 * 创建文件事件
 */
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc);
/**
 * 删除文件事件
 */
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);
/**
 * 创建定时器事件
 */
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc);
/**
 * 删除定时器事件
 */
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);
/**
 * 对事件进行处理
 */
int aeProcessEvents(aeEventLoop *eventLoop, int flags);
/**
 * 等待事件
 */
int aeWait(int fd, int mask, long long milliseconds);
/**
 * 开始运行事件
 */
void aeMain(aeEventLoop *eventLoop);

#endif
