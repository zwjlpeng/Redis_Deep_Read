/* adlist.c - A generic doubly linked list implementation
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
 * stdlib是C语言的标准库
 */

#include <stdlib.h>

/*
 * 包含声明的头文件 
 */

#include "adlist.h"

 /*
  * 包含redis自已的内存分配以及释放函数
  */

#include "zmalloc.h"

/* Create a new list. The created list can be freed with
 * AlFreeList(), but private value of every node need to be freed
 * by the user before to call AlFreeList().
 *
 * On error, NULL is returned. Otherwise the pointer to the new list. */

/*
 * 创建双向链表
 * 如果创建失败的话会返回NULL
 * 创建了链表的头结点
 * sizeof(*list)=sizeof(struct list)
 * 其余是一系列的初始化
 */

list *listCreate(void)
{
    struct list *list;
    
    if ((list = zmalloc(sizeof(*list))) == NULL)
        return NULL;
    list->head = list->tail = NULL;
    list->len = 0;
    list->dup = NULL;
    list->free = NULL;
    list->match = NULL;
    return list;
}

/* Free the whole list.
 *
 * This function can't fail. */

/*
 * 释放整个双向链表
 * 释放链表时，如果链表中定义了free函数，会调用free函数来释放结点值
 * 结点值释放后，再调用zfree函数将当前节点释放
 * 最后调用zfree将链表的头结点释放，整个链表空间即释放完成
 */

void listRelease(list *list)
{
    unsigned int len;
    listNode *current, *next;

    current = list->head;
    len = list->len;
    while(len--) {
        next = current->next;
        if (list->free) list->free(current->value);
        zfree(current);
        current = next;
    }
    zfree(list);
}

/* Add a new node to the list, to head, contaning the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */

/*
 * 从头结点增加元素，即采用的是头插入法
 * 发生错误时将返回NULL
 */

list *listAddNodeHead(list *list, void *value)
{
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    node->value = value;
    //空链表时的处理情况
    if (list->len == 0) {
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = NULL;
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    list->len++;
    return list;
}

/* Add a new node to the list, to tail, contaning the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */

/*
 * 采用尾插入法向链表中增加元素 
 * value是向链表中增加的值
 * 如果增加失败的话会返回空指针
 */

list *listAddNodeTail(list *list, void *value)
{
    listNode *node;

    if ((node = zmalloc(sizeof(*node))) == NULL)
        return NULL;
    node->value = value;
    if (list->len == 0) {
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = list->tail;
        node->next = NULL;
        list->tail->next = node;
        list->tail = node;
    }
    list->len++;
    return list;
}

/* Remove the specified node from the specified list.
 * It's up to the caller to free the private value of the node.
 *
 * This function can't fail. */

 /*
  * 从双向链表中删除结点值
  * node表示要删除的结点
  */

void listDelNode(list *list, listNode *node)
{
    if (node->prev)//判断删除节点是不是第一个节点
        node->prev->next = node->next;
    else
        list->head = node->next;
    if (node->next)//删除尾结点时的特殊处理情况
        node->next->prev = node->prev;
    else
        list->tail = node->prev;
    //如果链表结构中定义了结点释放函数，则调用注册的函数
    if (list->free) list->free(node->value);
    zfree(node);
    list->len--;
}

/* Returns a list iterator 'iter'. After the initialization every
 * call to listNext() will return the next element of the list.
 *
 * This function can't fail. */

/*
 * 获取链表的迭代器
 * direction表示的是迭代器遍历的方向
 * 如果获取失败的话，则返回NULL
 */

listIter *listGetIterator(list *list, int direction)
{
    listIter *iter;
    
    if ((iter = zmalloc(sizeof(*iter))) == NULL) return NULL;
    if (direction == AL_START_HEAD)//如果是从头向后遍历iter->next=list->head
        iter->next = list->head;
    else//如果是从后面前遍历时iter->next=list->tail
        iter->next = list->tail;
    iter->direction = direction;//定义遍历时的方向
    return iter;
}

/* Release the iterator memory */

/*
 * 释放链表的迭代器
 */

void listReleaseIterator(listIter *iter) {
    zfree(iter);
}

/* Create an iterator in the list private iterator structure */

/*
 * 将链表结构中内含的迭代器设置为向尾遍历
 */

void listRewind(list *list) {
    list->iter.next = list->head;
    list->iter.direction = AL_START_HEAD;
}

/*
 * 和上一个函数的作用相反
 * 将链表结构中内含的迭代器设置为向头遍历
 */

void listRewindTail(list *list) {
    list->iter.next = list->tail;
    list->iter.direction = AL_START_TAIL;
}

/* Return the next element of an iterator.
 * It's valid to remove the currently returned element using
 * listDelNode(), but not to remove other elements.
 *
 * The function returns a pointer to the next element of the list,
 * or NULL if there are no more elements, so the classical usage patter
 * is:
 *
 * iter = listGetItarotr(list,<direction>);
 * while ((node = listNextIterator(iter)) != NULL) {
 *     DoSomethingWith(listNodeValue(node));
 * }
 *
 * */

 /*
  * 返回迭代器所指元素
  * 同时迭代器指向下一个元素的值
  */


listNode *listNext(listIter *iter)
{
    listNode *current = iter->next;

    if (current != NULL) {
        if (iter->direction == AL_START_HEAD)
            iter->next = current->next;
        else
            iter->next = current->prev;
    }
    return current;
}

/* List Yield just call listNext() against the list private iterator */

/*
 * 放弃当前访问的节点，返回下一个节点的值
 * &list->iter表示的是获取链表中迭代器的地址
 * listNext函数调用后，迭代器会指向当前元素的下一个元素
 */

listNode *listYield(list *list) {
    return listNext(&list->iter);
}

/* Duplicate the whole list. On out of memory NULL is returned.
 * On success a copy of the original list is returned.
 *
 * The 'Dup' method set with listSetDupMethod() function is used
 * to copy the node value. Otherwise the same pointer value of
 * the original node is used as value of the copied node.
 *
 * The original list both on success or error is never modified. */ 

 /*
  * 链表的复制函数
  * 复制成功后，返回复制链表的头节点
  */

list *listDup(list *orig)
{
    list *copy;//复制链表的头指针
    listIter *iter;
    listNode *node;
    
    //创建链表的头节点
    if ((copy = listCreate()) == NULL)
        return NULL;
    //设置链表的复制函数
    copy->dup = orig->dup;
    //设置链表的释放函数
    copy->free = orig->free;
    //设置链表的匹配函数
    copy->match = orig->match;
    //获取链表的迭代器
    iter = listGetIterator(orig, AL_START_HEAD);
    //开始遍历链表，并复制链表中的元素值
    while((node = listNext(iter)) != NULL) {
        void *value;

        if (copy->dup) {
            value = copy->dup(node->value);
            if (value == NULL) {//复制失败时的情况
                listRelease(copy);//将复制一半的链表释放掉
                listReleaseIterator(iter);//释放链表的迭代器
                return NULL;
            }
        } else//这样会产生弱拷贝
            value = node->value;
        //将该值采用尾结点的方式进行插入
        if (listAddNodeTail(copy, value) == NULL) {
            //插入失败的情况下进行资源的回收
            listRelease(copy);
            listReleaseIterator(iter);
            return NULL;
        }
    }
    //复制完成后将迭代器所占用的内存空间释放掉
    listReleaseIterator(iter);
    return copy;
}

/* Search the list for a node matching a given key.
 * The match is performed using the 'match' method
 * set with listSetMatchMethod(). If no 'match' method
 * is set, the 'value' pointer of every node is directly
 * compared with the 'key' pointer.
 *
 * On success the first matching node pointer is returned
 * (search starts from head). If no matching node exists
 * NULL is returned. */

 /*
  * 返回鲢表中第一个匹配key元素的值
  * 如果链表中不存在该元素，则返回NULL
  * 时间复杂度为o(N)
  */

listNode *listSearchKey(list *list, void *key)
{
    listIter *iter;
    listNode *node;

    iter = listGetIterator(list, AL_START_HEAD);
    while((node = listNext(iter)) != NULL) {
        if (list->match) {//如果给链表设置了匹配函数的话，采用链表自身的匹配函数
            if (list->match(node->value, key)) {//如果匹配成功的话
                listReleaseIterator(iter);//释放迭代器所占用的内存空间
                return node;//返回链表当前的结点
            }
        } else {//如果未设置match函数的话
            if (key == node->value) {//直接比较指针里的地址
                listReleaseIterator(iter);//释放迭代器所占用的内存空间
                return node;//返回链表当前的结点
            }
        }
    }
    listReleaseIterator(iter);//释放迭代器所占用的内存空间
    return NULL;
}

/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimante
 * and so on. If the index is out of range NULL is returned. */

/*
 * 返回指定索引位置的元素
 * index表示索引位置，从0开始
 * 如果index超了链表的长度NULL将返回
 * 负数表示从尾开始算起，即倒数
 */

listNode *listIndex(list *list, int index) {
    listNode *n;
    //-1表示尾节点 
    //-2表示的是尾节点的下一个节点
    if (index < 0) {
        index = (-index)-1;
        n = list->tail;
        while(index-- && n) n = n->prev;
    } else {
        //从头开始进行遍历时0指向head
        //1指向头节点的下一个元素
        n = list->head;
        while(index-- && n) n = n->next;
    }
    //返回索引位置的节点
    return n;
}
