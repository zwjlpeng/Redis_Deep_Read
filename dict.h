/* Hash Tables Implementation.
 *
 * This file implements in memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
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

#ifndef __DICT_H
#define __DICT_H

//定义错误相关的码
#define DICT_OK 0
#define DICT_ERR 1

/* Unused arguments generate annoying warnings... */
#define DICT_NOTUSED(V) ((void) V)

//实际存放数据的地方
typedef struct dictEntry {
    void *key;
    void *val;
    struct dictEntry *next;
} dictEntry;

//要作用于哈希表上的相关函数
/*
 * dictType在哈希系统中包含了一系列可由应用程序定义的
 * 函数指针
 */
typedef struct dictType {
    unsigned int (*hashFunction)(const void *key);
    void *(*keyDup)(void *privdata, const void *key);
    void *(*valDup)(void *privdata, const void *obj);
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);
    void (*keyDestructor)(void *privdata, void *key);
    void (*valDestructor)(void *privdata, void *obj);
} dictType;

//哈希表的定义
typedef struct dict {
    //指向实际的哈希表记录(用数组+开链的形式进行保存)
    dictEntry **table;
    //type中包含一系列哈希表需要用到的函数
    dictType *type;
    //size表示哈希表的大小，为2的指数
    unsigned long size;
    //sizemask=size-1,方便哈希值根据size取模
    unsigned long sizemask;
    //used记录了哈希表中有多少记录
    unsigned long used;
    void *privdata;
} dict;

//对Hash表进行迭代遍历时使用的迭代器
typedef struct dictIterator {
    dict *ht;
    int index;
    dictEntry *entry, *nextEntry;
} dictIterator;

/* This is the initial size of every hash table */
//每个Hash表的初始大小
#define DICT_HT_INITIAL_SIZE     4

//一系列的宏定义
/* ------------------------------- Macros ------------------------------------*/
//释放Hash表中的值
#define dictFreeEntryVal(ht, entry) \
    if ((ht)->type->valDestructor) \
        (ht)->type->valDestructor((ht)->privdata, (entry)->val)
//设置hash表中的值
//如果存在ht->type->valDup函数时，将调用该指针所指向的函数
//否则的话会直接调用进行值拷贝
/*为什么要加上do-while循环可以参考我的个人博客www.yanyulin.info*/
#define dictSetHashVal(ht, entry, _val_) do { \
    if ((ht)->type->valDup) \
        entry->val = (ht)->type->valDup((ht)->privdata, _val_); \
    else \
        entry->val = (_val_); \
} while(0)

//释放键值
//如果定义了键值的析构函数，则调用该函数
#define dictFreeEntryKey(ht, entry) \
    if ((ht)->type->keyDestructor) \
        (ht)->type->keyDestructor((ht)->privdata, (entry)->key)

//进行key的复制，与dictSetHashVal一样
#define dictSetHashKey(ht, entry, _key_) do { \
    if ((ht)->type->keyDup) \
        entry->key = (ht)->type->keyDup((ht)->privdata, _key_); \
    else \
        entry->key = (_key_); \
} while(0)

//对键值进行对比，比对两个键值是否相等
//如果定义了keyCompare时，则调用keyCompare函数
#define dictCompareHashKeys(ht, key1, key2) \
    (((ht)->type->keyCompare) ? \
        (ht)->type->keyCompare((ht)->privdata, key1, key2) : \
        (key1) == (key2))

//对key进行hash取值
#define dictHashKey(ht, key) (ht)->type->hashFunction(key)
//获取键值对中的键
#define dictGetEntryKey(he) ((he)->key)
//获取键值对中的值
#define dictGetEntryVal(he) ((he)->val)
//获取hash表的大小
#define dictSlots(ht) ((ht)->size)
//获取目前hash表中有多少条记录
#define dictSize(ht) ((ht)->used)

/**
 * 创建与Hash表相关的功能函数
 */
dict *dictCreate(dictType *type, void *privDataPtr);
/**
 * 扩充hash表
 */
int dictExpand(dict *ht, unsigned long size);
/**
 * 向Hash表中增加键值
 */
int dictAdd(dict *ht, void *key, void *val);
/**
 * 从Hash表中替换掉键值 
 */
int dictReplace(dict *ht, void *key, void *val);
/**
 * 从Hash表中删除键值
 */
int dictDelete(dict *ht, const void *key);
/**
 * 从Hash表中删除键值
 */
int dictDeleteNoFree(dict *ht, const void *key);
/**
 * 释放Hash表
 */
void dictRelease(dict *ht);
/**
 * 在Hash表中查找指定的键值
 */
dictEntry * dictFind(dict *ht, const void *key);
/**
 * 重新设置Hash表中的大小
 */
int dictResize(dict *ht);
/**
 * 获取Hash表上的迭代器
 */
dictIterator *dictGetIterator(dict *ht);
/**
 * 获取遍历Hash表时的下一条记录
 */
dictEntry *dictNext(dictIterator *iter);
/**
 * 释放Hash表上的迭代器
 */
void dictReleaseIterator(dictIterator *iter);
/**
 * 从Hash表中随机获取一个键值 
 */
dictEntry *dictGetRandomKey(dict *ht);
/**
 * 打印出Hash表中的当前状态
 */
void dictPrintStats(dict *ht);
/**
 * 获取Hash函数
 */
unsigned int dictGenHashFunction(const unsigned char *buf, int len);
/**
 * 判断Hash表是否为空
 */
void dictEmpty(dict *ht);

/* Hash table types */

//其中系统定义了三种默认的type,表示最常用的三种哈希表
/**
 * 堆字符串复制键
 */
extern dictType dictTypeHeapStringCopyKey;
/**
 * 堆字符串
 */
extern dictType dictTypeHeapStrings;
/**
 * 堆字符串复制值
 */
extern dictType dictTypeHeapStringCopyKeyValue;

#endif /* __DICT_H */
