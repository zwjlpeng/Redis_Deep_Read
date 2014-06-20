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

#include "fmacros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>

#include "dict.h"
#include "zmalloc.h"

/* ---------------------------- Utility funcitons --------------------------- */

/**
 * 打印出系统的崩溃信息
 * C语言中的可变参数的使用va_start va_end
 */
static void _dictPanic(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "\nDICT LIBRARY PANIC: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n\n");
    va_end(ap);
}

/* ------------------------- Heap Management Wrappers------------------------ */
/** 
 *  堆分配函数
 */
static void *_dictAlloc(size_t size)
{
    void *p = zmalloc(size);
    if (p == NULL)
        _dictPanic("Out of memory");
    return p;
}

/**
 * 堆释放函数
 */
static void _dictFree(void *ptr) {
    zfree(ptr);
}

/* -------------------------- private prototypes ---------------------------- */

static int _dictExpandIfNeeded(dict *ht);
static unsigned long _dictNextPower(unsigned long size);
static int _dictKeyIndex(dict *ht, const void *key);
static int _dictInit(dict *ht, dictType *type, void *privDataPtr);

/* -------------------------- hash functions -------------------------------- */

/* Thomas Wang's 32 bit Mix Function */
/**
 * 求Hash的键值，可以促使均匀分布
 */
unsigned int dictIntHashFunction(unsigned int key)
{
    key += ~(key << 15);
    key ^=  (key >> 10);
    key +=  (key << 3);
    key ^=  (key >> 6);
    key += ~(key << 11);
    key ^=  (key >> 16);
    return key;
}

/**
 * 直接将整数key作为Hash的键值
 */
/* Identity hash function for integer keys */
unsigned int dictIdentityHashFunction(unsigned int key)
{
    return key;
}

/* Generic hash function (a popular one from Bernstein).
 * I tested a few and this was the best. */
//Hash函数(通用目的Hash函数)
unsigned int dictGenHashFunction(const unsigned char *buf, int len) {
    unsigned int hash = 5381;

    while (len--)
        hash = ((hash << 5) + hash) + (*buf++); /* hash * 33 + c */
    return hash;
}

/* ----------------------------- API implementation ------------------------- */

/* Reset an hashtable already initialized with ht_init().
 * NOTE: This function should only called by ht_destroy(). */
/**
 * 重设Hash表
 * 各种值都设置为0
 */
static void _dictReset(dict *ht)
{
    ht->table = NULL;
    ht->size = 0;
    ht->sizemask = 0;
    ht->used = 0;
}

/**
 * 创建一个新的Hash表
 * type为可用于Hash表上面的相应函数
 */
/* Create a new hash table */
dict *dictCreate(dictType *type,
        void *privDataPtr)
{
    dict *ht = _dictAlloc(sizeof(*ht));

    _dictInit(ht,type,privDataPtr);
    return ht;
}

/* Initialize the hash table */
/**
 * 初始化Hash表
 */
int _dictInit(dict *ht, dictType *type,
        void *privDataPtr)
{
    //对Hash表进行初始化
    _dictReset(ht);
    //初始化能够作用于Hash中的相应函数集
    ht->type = type;
    //初始化hashtable的私有数据段
    ht->privdata = privDataPtr;
    //返回初始化成功
    return DICT_OK;
}

/* Resize the table to the minimal size that contains all the elements,
 * but with the invariant of a USER/BUCKETS ration near to <= 1 */
/**
 * DICT_HT_INITIAL_SIZE=4表示的是Hash表的初始大小
 * 重新调整Hash表的大小
 * 从这里可以看出Hash表的最小的大注为4
 */
int dictResize(dict *ht)
{
    int minimal = ht->used;

    if (minimal < DICT_HT_INITIAL_SIZE)
        minimal = DICT_HT_INITIAL_SIZE;
    return dictExpand(ht, minimal);
}

/* Expand or create the hashtable */
/**
 * 创建Hash表，Hash表的大小为size
 */
int dictExpand(dict *ht, unsigned long size)
{
    dict n; /* the new hashtable */
    //重设Hash表的大小，大小为2的指数
    unsigned long realsize = _dictNextPower(size);

    /* the size is invalid if it is smaller than the number of
     * elements already inside the hashtable */
    //如果大小比当原Hash表中记录数目还要小的话，则出错
    if (ht->used > size)
        return DICT_ERR;
    //初始化
    _dictInit(&n, ht->type, ht->privdata);
    n.size = realsize;
    //保证为素数
    n.sizemask = realsize-1;
    n.table = _dictAlloc(realsize*sizeof(dictEntry*));
    /* Initialize all the pointers to NULL */
    //将所有的指针初始为空
    memset(n.table, 0, realsize*sizeof(dictEntry*));

    /* Copy all the elements from the old to the new table:
     * note that if the old hash table is empty ht->size is zero,
     * so dictExpand just creates an hash table. */
     //使用的内存记录数
    n.used = ht->used;
    for (i = 0; i < ht->size && ht->used > 0; i++) {
        dictEntry *he, *nextHe;

        if (ht->table[i] == NULL) continue;
        //采用的是桶状链表形式
        /* For each hash entry on this slot... */
        he = ht->table[i];
        //在遍历的过程中对每一个元素又求取出在新Hash表中相应的位置 
        while(he) {
            unsigned int h;
            nextHe = he->next;
            /* Get the new element index */
            //dictHashKey(ht,he->key)获取相应的键值key,实际上就是取模运算
            //求取在新hash表中的元素的位置
            h = dictHashKey(ht, he->key) & n.sizemask;
            //采用的是头插入法进行的插入
            he->next = n.table[h];
            n.table[h] = he;
            ht->used--;
            /* Pass to the next element */
            he = nextHe;
        }
    }
    //断言原Hash表中已经没有记录了
    assert(ht->used == 0);
    //将原Hash表进行释放 
    _dictFree(ht->table);
 
    /* Remap the new hashtable in the old */
    //将新Hash表作为值进行赋值
    *ht = n;
    //返回创建成功
    return DICT_OK;
}

/* Add an element to the target hash table */
/**
 * 向Hash表中增加元素
 * 增加元素的键为key,值为vals
 */
int dictAdd(dict *ht, void *key, void *val)
{
    int index;
    dictEntry *entry;

    /* Get the index of the new element, or -1 if
     * the element already exists. */
    if ((index = _dictKeyIndex(ht, key)) == -1)
        return DICT_ERR;

    /* Allocates the memory and stores key */
    //分配内存空间
    entry = _dictAlloc(sizeof(*entry));
    //将其放入相应的slot里面
    //采用的是头插入法进行插入
    entry->next = ht->table[index];
    ht->table[index] = entry;

    /* Set the hash entry fields. */
    dictSetHashKey(ht, entry, key);
    dictSetHashVal(ht, entry, val);
    //使用的记录数进行+1操作
    ht->used++;
    //返回OK标记
    return DICT_OK;
}

/* Add an element, discarding the old if the key already exists */
//向hash表中增加一个元素，如果Hash表中已经有该元素的话
//则将该元素进行替换掉
int dictReplace(dict *ht, void *key, void *val)
{
    dictEntry *entry;

    /* Try to add the element. If the key
     * does not exists dictAdd will suceed. */
    if (dictAdd(ht, key, val) == DICT_OK)
        return DICT_OK;
    /* It already exists, get the entry */
    //如果已经存在的话，则获取相应的位置 
    entry = dictFind(ht, key);
    /* Free the old value and set the new one */
    //将原Hash表中该entry的值进行释放
    //避免内存泄露
    dictFreeEntryVal(ht, entry);
    //给该节点设置新值
    dictSetHashVal(ht, entry, val);
    //返回成功标记
    return DICT_OK;
}

/**
 * 从Hash表中删除指定的key
 */
/* Search and remove an element */
static int dictGenericDelete(dict *ht, const void *key, int nofree)
{
    unsigned int h;
    dictEntry *he, *prevHe;

    if (ht->size == 0)
        return DICT_ERR;
    /**
     *  返回key对应的dictEntry
     */
    h = dictHashKey(ht, key) & ht->sizemask;
    he = ht->table[h];

    prevHe = NULL;
    while(he) {
        if (dictCompareHashKeys(ht, key, he->key)) {
            /* Unlink the element from the list */
            if (prevHe)
                prevHe->next = he->next;
            else
                ht->table[h] = he->next;
            if (!nofree) {
                //如果需要释放的情况下，则进行相应的释放操作
                dictFreeEntryKey(ht, he);
                dictFreeEntryVal(ht, he);
            }
            _dictFree(he);
            //记录数相应的进行减小
            ht->used--;
            return DICT_OK;
        }
        //进行相应的赋值操作
        prevHe = he;
        he = he->next;
    }
    //返回错误
    return DICT_ERR; /* not found */
}

/**
 * 释放指定的键值
 */
int dictDelete(dict *ht, const void *key) {
    return dictGenericDelete(ht,key,0);
}

/**
 * 从Hash表中删除key对应的记录，但是不
 * 删除相应的key以及value
 */
int dictDeleteNoFree(dict *ht, const void *key) {
    return dictGenericDelete(ht,key,1);
}

/* Destroy an entire hash table */
/**
 * 释放整个Hash表
 */
int _dictClear(dict *ht)
{
    unsigned long i;
    /* Free all the elements */
    /**
     * 将所有的元素进行释放 
     */
    for (i = 0; i < ht->size && ht->used > 0; i++) {
        dictEntry *he, *nextHe;
        //表标桶状结构中没有链表元素
        if ((he = ht->table[i]) == NULL) continue;
        //循环进行遍历
        while(he) {
            nextHe = he->next;
            //释放键
            dictFreeEntryKey(ht, he);
            //释放值 
            dictFreeEntryVal(ht, he);
            //释放结构体
            _dictFree(he);
            //记录数作相应的减法
            ht->used--;
            he = nextHe;
        }
    }
    /* Free the table and the allocated cache structure */
    //释放整个Hash表
    _dictFree(ht->table);
    /* Re-initialize the table */
    //重新初始化整个Hash表，Hash结构还是要保留的
    _dictReset(ht);
    return DICT_OK; /* never fails */
}

/* Clear & Release the hash table */
/**
 * 释放Hash表
 * 整个Hash表连同Hash结构都将释放 
 */
void dictRelease(dict *ht)
{
    //清除Hash表中的数据
    _dictClear(ht);
    //对空间进行释放 
    _dictFree(ht);
}

/**
 *  从HashTable中查找key的相应的dictEntry
 */
dictEntry *dictFind(dict *ht, const void *key)
{
    dictEntry *he;
    unsigned int h;
    //如果Hash表的大小为0,则直接返回NULL
    if (ht->size == 0) return NULL;
    h = dictHashKey(ht, key) & ht->sizemask;
    he = ht->table[h];
    while(he) {
        if (dictCompareHashKeys(ht, key, he->key))
            return he;
        he = he->next;
    }
    return NULL;
}

/** 
 * 获取Hash表中的相应的迭代器
 */
dictIterator *dictGetIterator(dict *ht)
{
    //给迭代器分配内存空间
    dictIterator *iter = _dictAlloc(sizeof(*iter));
    //对迭代器进行相应的初始化
    iter->ht = ht;
    iter->index = -1;
    iter->entry = NULL;
    iter->nextEntry = NULL;
    return iter;
}

/**
 * 对Hashtable进行迭代遍历操作
 */
dictEntry *dictNext(dictIterator *iter)
{
    while (1) {
        if (iter->entry == NULL) {
            iter->index++;
            //如果遍历的index大于整个Hashtable数组的大小时
            //说明已经遍历完成，直接跳出
            if (iter->index >=
                    (signed)iter->ht->size) break;
            iter->entry = iter->ht->table[iter->index];
        } else {
            //遍历到下一个元素
            iter->entry = iter->nextEntry;
        }
        if (iter->entry) {
            /* We need to save the 'next' here, the iterator user
             * may delete the entry we are returning. */
            //返回遍历过程中的下一个元素
            iter->nextEntry = iter->entry->next;
            //返回当前遍历的元素
            return iter->entry;
        }
    }
    return NULL;
}

/**
 * 释放迭代器把指向的空间
 */
void dictReleaseIterator(dictIterator *iter)
{
    _dictFree(iter);
}

/* Return a random entry from the hash table. Useful to
 * implement randomized algorithms */
/**
 * 从Hashtable中获取随机的key
 */
dictEntry *dictGetRandomKey(dict *ht)
{
    dictEntry *he;
    unsigned int h;
    int listlen, listele;
    //如果整个HashTable中压根没有记录时
    //直接返回NULL
    if (ht->used == 0) return NULL;
    //否则随机选择一个HashTable里面的slot
    do {
        h = random() & ht->sizemask;
        he = ht->table[h];
    } while(he == NULL);

    /* Now we found a non empty bucket, but it is a linked
     * list and we need to get a random element from the list.
     * The only sane way to do so is to count the element and
     * select a random index. */
    //计算出处于这个slot里面的元素数目
    listlen = 0;
    while(he) {
        he = he->next;
        listlen++;
    }
    //从整个slot链表中选择元素的位置
    listele = random() % listlen;
    he = ht->table[h];
    //指针指向该链表的位置 
    while(listele--) he = he->next;
    return he;
}

/* ------------------------- private functions ------------------------------ */

/* Expand the hash table if needed */
/**
 * 判断Hash表的大小是否需要扩充
 */
static int _dictExpandIfNeeded(dict *ht)
{
    /* If the hash table is empty expand it to the intial size,
     * if the table is "full" dobule its size. */
    //如果目前的hashtable的大小为0，则将大小设置为4
    if (ht->size == 0)
        return dictExpand(ht, DICT_HT_INITIAL_SIZE);
    //如果hash表里数据记录数已经与hashtable的大小相同的话，则将大小扩充为2倍
    if (ht->used == ht->size)//里面的记录数已经达到了hashtable的大小时，则需要进行扩充空间
        return dictExpand(ht, ht->size*2);
    return DICT_OK;
}

/* Our hash table capability is a power of two */
/**
 * Hash表的大小为2的指数幂
 */
static unsigned long _dictNextPower(unsigned long size)
{
    //将i设置为hash表的初始大小即为4
    unsigned long i = DICT_HT_INITIAL_SIZE;
    //返回2的指数幂中与size大小最接近且比size大小的值
    if (size >= LONG_MAX) return LONG_MAX;
    while(1) {
        if (i >= size)
            return i;
        i *= 2;
    }
}

/* Returns the index of a free slot that can be populated with
 * an hash entry for the given 'key'.
 * If the key already exists, -1 is returned. */
/**
 * 返回key在hash表的位置，如果key在Hash表里已经存在，
 * 则返回-1
 */
static int _dictKeyIndex(dict *ht, const void *key)
{
    unsigned int h;
    dictEntry *he;
    /**
     * 判断是否需要扩充Hash表
     */
    /* Expand the hashtable if needed */
    if (_dictExpandIfNeeded(ht) == DICT_ERR)
        return -1;
    /* Compute the key hash value */
    /**
     * 获取Hash表中key对应元素位置【即Hash表中的位置】 
     */
    h = dictHashKey(ht, key) & ht->sizemask;
    /* Search if this slot does not already contain the given key */
    he = ht->table[h];
    while(he) {
        //如果已经存在了话，即键值相等的话
        if (dictCompareHashKeys(ht, key, he->key))
            return -1;
        he = he->next;
    }
    //否则的话，就返回相应的slot位置 
    return h;
}

/**
 * 清空HashTable
 * 清空后HashTable进行了重新的初始化
 */
void dictEmpty(dict *ht) {
    _dictClear(ht);
}

/**
 * 打印出HashTable表中数据的当前状态
 * maxchainlen最大链表的长度
 * chainlen
 * slot表示Hashtable中已使用的桶数
 */
#define DICT_STATS_VECTLEN 50
void dictPrintStats(dict *ht) {
    unsigned long i, slots = 0, chainlen, maxchainlen = 0;
    unsigned long totchainlen = 0;
    unsigned long clvector[DICT_STATS_VECTLEN];

    //如果Hashtable中为0记录数，则打印出空Hashtable
    if (ht->used == 0) {
        printf("No stats available for empty dictionaries\n");
        return;
    }
    //对clvector数组进行相应的初始化
    for (i = 0; i < DICT_STATS_VECTLEN; i++) clvector[i] = 0;
    for (i = 0; i < ht->size; i++) {
        dictEntry *he;

        if (ht->table[i] == NULL) {
            //从这里可以看出clvector[0]记录Hashtable中的空槽数
            clvector[0]++;
            continue;
        }
        //否则的知槽数++
        slots++;
        /* For each hash entry on this slot... */
        chainlen = 0;
        he = ht->table[i];
        //算出槽中的链表长度
        while(he) {
            chainlen++;
            he = he->next;
        }
        //如果小于50的话，则clvector相应的记录增加
        //例如clvector[2]=10,表示Hashtable中槽里面链表长度为2的有10个
        //超过50的全部放在clvector[49]里面
        clvector[(chainlen < DICT_STATS_VECTLEN) ? chainlen : (DICT_STATS_VECTLEN-1)]++;
        if (chainlen > maxchainlen) maxchainlen = chainlen;
        totchainlen += chainlen;
    }
    //将状态信息打印出来
    printf("Hash table stats:\n");
    //hashtable的大小
    printf(" table size: %ld\n", ht->size);
    //hashtable中存在的记录数
    printf(" number of elements: %ld\n", ht->used);
    //hashtalbe中已使用的槽数
    printf(" different slots: %ld\n", slots);
    //hashtable中最大键的长度
    printf(" max chain length: %ld\n", maxchainlen);
    //hashtable中平均链表的长度
    printf(" avg chain length (counted): %.02f\n", (float)totchainlen/slots);
    //和上一个值理论上来讲应该是一样的
    printf(" avg chain length (computed): %.02f\n", (float)ht->used/slots);
    printf(" Chain length distribution:\n");
    //打印出链表中的各个槽里面的链表的长度记录
    for (i = 0; i < DICT_STATS_VECTLEN-1; i++) {
        if (clvector[i] == 0) continue;
        printf("   %s%ld: %ld (%.02f%%)\n",(i == DICT_STATS_VECTLEN-1)?">= ":"", i, clvector[i], ((float)clvector[i]/ht->size)*100);
    }
}

/* ----------------------- StringCopy Hash Table Type ------------------------*/

/**
 * Hash函数(字符串复制相关的Hash表函数)
 */
static unsigned int _dictStringCopyHTHashFunction(const void *key)
{
    return dictGenHashFunction(key, strlen(key));
}
/**
 * 键值复制相关的函数
 */
static void *_dictStringCopyHTKeyDup(void *privdata, const void *key)
{
    int len = strlen(key);
    char *copy = _dictAlloc(len+1);
    /**
     * #define DICT_NOTUSED(V) ((void) V);
     */
    DICT_NOTUSED(privdata);
    //进行键值的复制操作
    memcpy(copy, key, len);
    //在字符串未尾加了\0标识字符串的结束
    copy[len] = '\0';
    return copy;
}
 
/**
 * HashTable中值复制操作
 */
static void *_dictStringKeyValCopyHTValDup(void *privdata, const void *val)
{  
    //获取值的长度
    int len = strlen(val);
    //分配内存空间
    char *copy = _dictAlloc(len+1);
    DICT_NOTUSED(privdata);
    //进行内存复制的操作
    memcpy(copy, val, len);
    copy[len] = '\0';
    return copy;
}

/**
 * 键值的比较函数
 * 比较key1与key2的值是否相等
 */
static int _dictStringCopyHTKeyCompare(void *privdata, const void *key1,
        const void *key2)
{
    DICT_NOTUSED(privdata);

    return strcmp(key1, key2) == 0;
}

/**
 * HashTable的析构函数
 */
static void _dictStringCopyHTKeyDestructor(void *privdata, void *key)
{
    DICT_NOTUSED(privdata);
    //释放key所占用的内存空间
    _dictFree((void*)key); /* ATTENTION: const cast */
}

/**
 * HashTable中释放值
 */
static void _dictStringKeyValCopyHTValDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
    //释放值所占用的内存空间
    _dictFree((void*)val); /* ATTENTION: const cast */
}

/**
 * Redis提供的三种自定义的Hash的Type函数类型 
 */
dictType dictTypeHeapStringCopyKey = {
    _dictStringCopyHTHashFunction,        /* hash function */
    _dictStringCopyHTKeyDup,              /* key dup */
    NULL,                               /* val dup */
    _dictStringCopyHTKeyCompare,          /* key compare */
    _dictStringCopyHTKeyDestructor,       /* key destructor */
    NULL                                /* val destructor */
};

/* This is like StringCopy but does not auto-duplicate the key.
 * It's used for intepreter's shared strings. */
dictType dictTypeHeapStrings = {
    _dictStringCopyHTHashFunction,        /* hash function */
    NULL,                               /* key dup */
    NULL,                               /* val dup */
    //关键字的比较
    _dictStringCopyHTKeyCompare,          /* key compare */
    //关键字的释放 
    _dictStringCopyHTKeyDestructor,       /* key destructor */
    NULL                                /* val destructor */
};

/* This is like StringCopy but also automatically handle dynamic
 * allocated C strings as values. */
dictType dictTypeHeapStringCopyKeyValue = {
    _dictStringCopyHTHashFunction,        /* hash function */
    _dictStringCopyHTKeyDup,              /* key dup */
    _dictStringKeyValCopyHTValDup,        /* val dup */
    _dictStringCopyHTKeyCompare,          /* key compare */
    _dictStringCopyHTKeyDestructor,       /* key destructor */
    _dictStringKeyValCopyHTValDestructor, /* val destructor */
};
