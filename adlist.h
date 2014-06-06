/* adlist.h - A generic doubly linked list implementation
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

/*
 * adlist.h与adlist.c实现的是一个普通的双向链表
 */

#ifndef __ADLIST_H__
#define __ADLIST_H__

/* Node, List, and Iterator are the only data structures used currently. */

/*
 * redis中最基本的结构用以标识链表中的结点
 * *prev标识上一个节点
 * *next标识下一个节点
 * *value标识节点的值
 */

typedef struct listNode {
    struct listNode *prev;
    struct listNode *next;
    void *value;
} listNode;

/*
 * 迭代器用于链表的遍历
 * next将要遍历的下一个元素
 * direction遍历链表的方向
 * 方向由下面的两个宏进行标识
 * AL_START_HEAD表示向前
 * AL_START_TAIL表示向后
 */

typedef struct listIter {
    listNode *next;
    int direction;
} listIter;

/*
 * redis中的双向链表
 * head标识链表的头指针
 * tail标识链表的尾结点
 * dup/free/match是三个函数指针
 * dup用于复制链表，返回值也是一个函数指针
 * free用于释放链表
 * match用于判断链表中是否存在*key的值
 * len标识链表的长度
 * listIter链表的迭代器，通过此迭代器可以对链表进行遍历
 * 至于为什么要提供这种迭代器，可以查看设计模式相关的书箱
 */

typedef struct list {
    listNode *head;
    listNode *tail;
    void *(*dup)(void *ptr);
    void (*free)(void *ptr);
    int (*match)(void *ptr, void *key);
    unsigned int len;
    listIter iter;
} list;

/* Functions implemented as macros */

/*
 * 宏函数
 */

/*
 * 获取链表的长度
 */

#define listLength(l) ((l)->len)

 /*
  * 获取链表的头结点
  */

#define listFirst(l) ((l)->head)

 /*
  * 获取链表的尾结点
  */

#define listLast(l) ((l)->tail)

 /*
  * 获取当前结点的上一个结点
  */

#define listPrevNode(n) ((n)->prev)

 /*
  * 获取链表当前节点的下一个节点
  */

#define listNextNode(n) ((n)->next)

 /*
  * 获取链表当前节点的值(注意此处获取的指针)
  */

#define listNodeValue(n) ((n)->value)

 /*
  * 给链表节点设置链表复制方法
  */

#define listSetDupMethod(l,m) ((l)->dup = (m))

 /*
  * 给链表节点设置释放链表的方法
  */

#define listSetFreeMethod(l,m) ((l)->free = (m))

/*
 * 给链表节点设置链表匹配方法
 */

#define listSetMatchMethod(l,m) ((l)->match = (m))

 /*
  * 获取链表的复制方法，此处即是获取复制方法的函数指针
  */

#define listGetDupMethod(l) ((l)->dup)

 /*
  * 获取链表节点的释放方法，此处获取的也是释放方法的函数指针
  */

#define listGetFree(l) ((l)->free)

 /*
  * 获取链表节点的匹配方法，此处获取的也是函数指针
  */

#define listGetMatchMethod(l) ((l)->match)

/* Prototypes */

/*
 * 函数原型声明
 */

 /*
  * 创建双向链表 
  */

list *listCreate(void);

 /*
  * 释放链表
  */

void listRelease(list *list);

/*
 * 头部插入节点的值
 */

list *listAddNodeHead(list *list, void *value);

/*
 * 尾部插入节点的值
 */

list *listAddNodeTail(list *list, void *value);

/*
 * 删除节点的值
 */

void listDelNode(list *list, listNode *node);

/*
 * 获取双向链表的迭代器
 */

listIter *listGetIterator(list *list, int direction);

/*
 * 返回链表的下一个结点值，此处的下一个要根据ListIter中定义的方向取
 */

listNode *listNext(listIter *iter);

/*
 * 释放链表的迭代器
 */

void listReleaseIterator(listIter *iter);

/*
 * 复制链表的函数
 */

list *listDup(list *orig);

/*
 * 从链表中搜索值为*key的节点
 */

listNode *listSearchKey(list *list, void *key);

/*
 * 返回index位置的节点
 */

listNode *listIndex(list *list, int index);

/*
 * 将链表中的迭代器变为正向迭代器
 */

void listRewind(list *list);

/*
 * 将链表中的迭代器初始方向设置为反向
 */

void listRewindTail(list *list);

/*
 * 放弃当前访问的节点
 */

listNode *listYield(list *list);

/*
 * 以下两个宏用于标识ListIter迭代链表时的方向
 */

/* Directions for iterators */
#define AL_START_HEAD 0
#define AL_START_TAIL 1

#endif /* __ADLIST_H__ */
