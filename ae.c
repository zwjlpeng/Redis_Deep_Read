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

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "ae.h"
#include "zmalloc.h"

/**
 * 创建事件循环
 */
aeEventLoop *aeCreateEventLoop(void) {
    aeEventLoop *eventLoop;
    //生成事件循环
    eventLoop = zmalloc(sizeof(*eventLoop));
    if (!eventLoop) return NULL;
    //文件事件头
    eventLoop->fileEventHead = NULL;
    //定时器事件头
    eventLoop->timeEventHead = NULL;
    //定时器事件的下个标识id
    eventLoop->timeEventNextId = 0;
    //停止位标记为0
    eventLoop->stop = 0;
    //返回相应的事件循环
    return eventLoop;
}

/**
 * 删除事件循环
 */
void aeDeleteEventLoop(aeEventLoop *eventLoop) {
    zfree(eventLoop);
}

/**
 * 设置事件循环中的停止位
 * 表示停止该事件
 */
void aeStop(aeEventLoop *eventLoop) {
    eventLoop->stop = 1;
}

/**
 * 创建文件事件
 * eventLoop事件循环句柄 
 * fd文件描述符
 * mask表示事件的类型
 * proc表示处理该事件的函数
 * clientData事件相关的数据参数 
 * finalizerProc事件处理完成后相应的处理机制
 */
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc)
{
    //文件事件指针
    aeFileEvent *fe;
    //生成aeFileEvent相应的结构体空间
    fe = zmalloc(sizeof(*fe));
    //如果内存空间分配失败的情奖品下
    //返回错误
    if (fe == NULL) return AE_ERR;
    //设置文件描述符
    fe->fd = fd;
    //设置文件的掩码
    fe->mask = mask;
    //设置文件事件的处理函数
    fe->fileProc = proc;
    //设置文件事件处理完成后要调用的相应函数
    fe->finalizerProc = finalizerProc;
    //设置文件事件携带的相关数据
    fe->clientData = clientData;
    //将文件事件插入事件循环中
    //采用的是头插入法
    fe->next = eventLoop->fileEventHead;
    eventLoop->fileEventHead = fe;
    //返回成功标志位
    return AE_OK;
}

/**
 * 删除文件事件
 */
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask)
{
    aeFileEvent *fe, *prev = NULL;

    fe = eventLoop->fileEventHead;
    while(fe) {
        if (fe->fd == fd && fe->mask == mask) {
            if (prev == NULL)
                eventLoop->fileEventHead = fe->next;
            else
                prev->next = fe->next;
            if (fe->finalizerProc)
                fe->finalizerProc(eventLoop, fe->clientData);
            zfree(fe);
            return;
        }
        prev = fe;
        fe = fe->next;
    }
}

/**
 * 获取当前的时间
 * seconds变量存放秒
 * milliseconds存放毫秒
 */
static void aeGetTime(long *seconds, long *milliseconds)
{
    struct timeval tv;
    /**
     * 获取当前的时间
     */
    gettimeofday(&tv, NULL);
    *seconds = tv.tv_sec;
    *milliseconds = tv.tv_usec/1000;
}

//向当前时间里面增加毫秒数
//其实目的就是对定时器事件进行延迟
static void aeAddMillisecondsToNow(long long milliseconds, long *sec, long *ms) {
    long cur_sec, cur_ms, when_sec, when_ms;
    //获取当前的时间
    aeGetTime(&cur_sec, &cur_ms);
    when_sec = cur_sec + milliseconds/1000;
    when_ms = cur_ms + milliseconds%1000;
    if (when_ms >= 1000) {
        when_sec ++;
        when_ms -= 1000;
    }
    *sec = when_sec;
    *ms = when_ms;
}

/**
 * 创建定时器事件
 * eventLoop事件循环
 * milliseconds事件的秒数
 * 定时器事件的处理函数
 */
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc)
{
    long long id = eventLoop->timeEventNextId++;
    aeTimeEvent *te;
    te = zmalloc(sizeof(*te));
    //如果失败的情况下，返回AE_ERR
    if (te == NULL) return AE_ERR;
    te->id = id;
    //定时器事件在多少时间之后启动
    aeAddMillisecondsToNow(milliseconds,&te->when_sec,&te->when_ms);
    te->timeProc = proc;
    te->finalizerProc = finalizerProc;
    te->clientData = clientData;
    te->next = eventLoop->timeEventHead;
    eventLoop->timeEventHead = te;
    //返回定时器事件的标识
    return id;
}

/**
 * 删除定时器事件
 */
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id)
{
    aeTimeEvent *te, *prev = NULL;
    //定时器事件的循环头
    te = eventLoop->timeEventHead;
    while(te) {
        if (te->id == id) {
            if (prev == NULL)
                eventLoop->timeEventHead = te->next;
            else
                prev->next = te->next;
            if (te->finalizerProc)
                te->finalizerProc(eventLoop, te->clientData);
            zfree(te);
            return AE_OK;
        }
        prev = te;
        te = te->next;
    }
    return AE_ERR; /* NO event with the specified ID found */
}

/* Search the first timer to fire.
 * This operation is useful to know how many time the select can be
 * put in sleep without to delay any event.
 * If there are no timers NULL is returned.
 *
 * Note that's O(N) since time events are unsorted. */
/**
 * 搜寻需要激活的定时器
 * 返回最近需要激活的定时器
 */
static aeTimeEvent *aeSearchNearestTimer(aeEventLoop *eventLoop)
{
    aeTimeEvent *te = eventLoop->timeEventHead;
    //指向最近的一个需要激活的定时器
    aeTimeEvent *nearest = NULL;
    while(te) {
        if (!nearest || te->when_sec < nearest->when_sec ||
                (te->when_sec == nearest->when_sec &&
                 te->when_ms < nearest->when_ms))
            nearest = te;
        te = te->next;
    }
    return nearest;
}

/* Process every pending time event, then every pending file event
 * (that may be registered by time event callbacks just processed).
 * Without special flags the function sleeps until some file event
 * fires, or when the next time event occurrs (if any).
 *
 * If flags is 0, the function does nothing and returns.
 * if flags has AE_ALL_EVENTS set, all the kind of events are processed.
 * if flags has AE_FILE_EVENTS set, file events are processed.
 * if flags has AE_TIME_EVENTS set, time events are processed.
 * if flags has AE_DONT_WAIT set the function returns ASAP until all
 * the events that's possible to process without to wait are processed.
 *
 * The function returns the number of events processed. */
/**
 * 开始处理事件
 */
 /**
  * FD_ZERO 将set清零使集合中不含任何fd
  * FD_SET将fd加入set集合中
  * FD_CLR将fd从集合中删除
  * FD_ISSET在调用select函数后，用FD_ISSET
  * 来检测fd在fdset集合中的状态是否变化返回，当检测到fd状态发生改变时
  * 返回真，否则返回假
  */
int aeProcessEvents(aeEventLoop *eventLoop, int flags)
{
    int maxfd = 0, numfd = 0, processed = 0;
    /**
     * fd_set实际上是一个long型数组
     * 各种文件集
     */
    fd_set rfds, wfds, efds;
    /**
     * 获取文件事件
     */
    aeFileEvent *fe = eventLoop->fileEventHead;
    //定时事件
    aeTimeEvent *te;
    long long maxId;
    AE_NOTUSED(flags);

    /* Nothing to do? return ASAP */
    if (!(flags & AE_TIME_EVENTS) && !(flags & AE_FILE_EVENTS)) return 0;

    //将三个文件描述符集清零
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    //检测文件事件
    /* Check file events */
    if (flags & AE_FILE_EVENTS) {
        while (fe != NULL) {
            if (fe->mask & AE_READABLE) FD_SET(fe->fd, &rfds);
            if (fe->mask & AE_WRITABLE) FD_SET(fe->fd, &wfds);
            if (fe->mask & AE_EXCEPTION) FD_SET(fe->fd, &efds);
            //maxfd中记录的是大文件描述符
            if (maxfd < fe->fd) maxfd = fe->fd;
            //记录的是文件描述符数
            numfd++;
            //将其向下移动一位
            fe = fe->next;
        }
    }
    /* Note that we want call select() even if there are no
     * file events to process as long as we want to process time
     * events, in order to sleep until the next time event is ready
     * to fire. */
    if (numfd || ((flags & AE_TIME_EVENTS) && !(flags & AE_DONT_WAIT))) {
        int retval;
        aeTimeEvent *shortest = NULL;
        struct timeval tv, *tvp;
        /**
         * 用于设置AE_DONT_WAIT是否是block/non-block
         */
        if (flags & AE_TIME_EVENTS && !(flags & AE_DONT_WAIT))
            //搜寻即譔被调用的定时器
            shortest = aeSearchNearestTimer(eventLoop);
        if (shortest) {
            long now_sec, now_ms;

            /* Calculate the time missing for the nearest
             * timer to fire. */ 
            aeGetTime(&now_sec, &now_ms);
            tvp = &tv;
            tvp->tv_sec = shortest->when_sec - now_sec;
            if (shortest->when_ms < now_ms) {
                tvp->tv_usec = ((shortest->when_ms+1000) - now_ms)*1000;
                tvp->tv_sec --;
            } else {
                tvp->tv_usec = (shortest->when_ms - now_ms)*1000;
            }
        } else {
            /* If we have to check for events but need to return
             * ASAP because of AE_DONT_WAIT we need to se the timeout
             * to zero */
             //非阻塞方式的情况下
            if (flags & AE_DONT_WAIT) {
                tv.tv_sec = tv.tv_usec = 0;
                tvp = &tv;
            } else {
                /* Otherwise we can block */
                tvp = NULL; /* wait forever */
            }
        }
        //返回值
        retval = select(maxfd+1, &rfds, &wfds, &efds, tvp);
        if (retval > 0) {
            fe = eventLoop->fileEventHead;
            while(fe != NULL) {
                int fd = (int) fe->fd;

                if ((fe->mask & AE_READABLE && FD_ISSET(fd, &rfds)) ||
                    (fe->mask & AE_WRITABLE && FD_ISSET(fd, &wfds)) ||
                    (fe->mask & AE_EXCEPTION && FD_ISSET(fd, &efds)))
                {
                    int mask = 0;

                    if (fe->mask & AE_READABLE && FD_ISSET(fd, &rfds))
                        mask |= AE_READABLE;
                    if (fe->mask & AE_WRITABLE && FD_ISSET(fd, &wfds))
                        mask |= AE_WRITABLE;
                    if (fe->mask & AE_EXCEPTION && FD_ISSET(fd, &efds))
                        mask |= AE_EXCEPTION;
                    //调用相应原处理函数
                    fe->fileProc(eventLoop, fe->fd, fe->clientData, mask);
                    //处理个数进行增加
                    processed++;
                    /* After an event is processed our file event list
                     * may no longer be the same, so what we do
                     * is to clear the bit for this file descriptor and
                     * restart again from the head. */
                    fe = eventLoop->fileEventHead;
                    FD_CLR(fd, &rfds);
                    FD_CLR(fd, &wfds);
                    FD_CLR(fd, &efds);
                } else {
                    fe = fe->next;
                }
            }
        }
    }
    /* Check time events */
    if (flags & AE_TIME_EVENTS) {
        te = eventLoop->timeEventHead;
        maxId = eventLoop->timeEventNextId-1;
        while(te) {
            long now_sec, now_ms;
            long long id;
            /**
             * 就不可能执行到此步的
             */
            if (te->id > maxId) {
                te = te->next;
                continue;
            }
            aeGetTime(&now_sec, &now_ms);
            if (now_sec > te->when_sec ||
                (now_sec == te->when_sec && now_ms >= te->when_ms))
            {
                int retval;

                id = te->id;
                //开始处理定时事件
                retval = te->timeProc(eventLoop, id, te->clientData);
                /* After an event is processed our time event list may
                 * no longer be the same, so we restart from head.
                 * Still we make sure to don't process events registered
                 * by event handlers itself in order to don't loop forever.
                 * To do so we saved the max ID we want to handle. */
                if (retval != AE_NOMORE) {
                    aeAddMillisecondsToNow(retval,&te->when_sec,&te->when_ms);
                } else {
                    aeDeleteTimeEvent(eventLoop, id);
                }
                te = eventLoop->timeEventHead;
            } else {
                te = te->next;
            }
        }
    }
    //返回已经处理的事件个数
    return processed; /* return the number of processed file/time events */
}

/* Wait for millseconds until the given file descriptor becomes
 * writable/readable/exception */
//等待
/**
 * redis里提供的同步事件处理API
 */
int aeWait(int fd, int mask, long long milliseconds) {
    struct timeval tv;
    fd_set rfds, wfds, efds;
    int retmask = 0, retval;
    /**
     * tv中存放的是要等待的是时间
     */
    tv.tv_sec = milliseconds/1000;
    tv.tv_usec = (milliseconds%1000)*1000;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    if (mask & AE_READABLE) FD_SET(fd,&rfds);
    if (mask & AE_WRITABLE) FD_SET(fd,&wfds);
    if (mask & AE_EXCEPTION) FD_SET(fd,&efds);
    if ((retval = select(fd+1, &rfds, &wfds, &efds, &tv)) > 0) {
        if (FD_ISSET(fd,&rfds)) retmask |= AE_READABLE;
        if (FD_ISSET(fd,&wfds)) retmask |= AE_WRITABLE;
        if (FD_ISSET(fd,&efds)) retmask |= AE_EXCEPTION;
        //返回哪一个文件描述符现在可以使用
        return retmask;
    } else {
        return retval;
    }
}
/**
 *  MAIN函数
 */
void aeMain(aeEventLoop *eventLoop)
{
    eventLoop->stop = 0;
    while (!eventLoop->stop)
        aeProcessEvents(eventLoop, AE_ALL_EVENTS);
}
