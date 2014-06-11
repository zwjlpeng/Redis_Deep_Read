/* zmalloc - total amount of allocated memory aware version of malloc()
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
//系统C语言标准库
#include <stdlib.h>
//系统标准库(提供的字符串相关操作的函数)
#include <string.h>
//自定的配置的头文件   
#include "config.h"

//目前已经使用的内存空间量
static size_t used_memory = 0;

//申请size大小的空间
void *zmalloc(size_t size) {
    //从这里可以看出的是不旦申请了，还多申请了sizeof(size_t)个空间
    void *ptr = malloc(size+sizeof(size_t));
    //如果申请失败的情况下，直接返回NULL
    if (!ptr) return NULL;
#ifdef HAVE_MALLOC_SIZE
    //redis_malloc_size用于Apple下获取ptr指向的空间大小
    used_memory += redis_malloc_size(ptr);
    return ptr;
#else
    //前一个字节用于存放分配的内存空间大小
    *((size_t*)ptr) = size;
    //由于申请了size+sizeof(size_t)个空间
    //因此内存空间的使用量又增加了
    used_memory += size+sizeof(size_t);
    //返回的位置向前移动了sizeof(size_t)个空间
    return (char*)ptr+sizeof(size_t);
#endif
}

/*
 * 重新分配内存空间大小，失败时返回NULL
 * 成功时，返回分配成功时的地址
 */

void *zrealloc(void *ptr, size_t size) {
//在Apple下会定义了HAVE_MALLOC_SIZE
#ifndef HAVE_MALLOC_SIZE
    //返回存放数据的实际地址
    void *realptr;
#endif
    //以前分配的空间的大小
    size_t oldsize;
    void *newptr;
    //如果没有分配内存空间时，直接使用zmalloc分配内存
    //并返回内存空间的首地址
    if (ptr == NULL) return zmalloc(size);
#ifdef HAVE_MALLOC_SIZE
    //获取重新分配前的内存空间大小
    oldsize = redis_malloc_size(ptr);
    newptr = realloc(ptr,size);
    //分配失败的情况下
    if (!newptr) return NULL;
    //记录重新分配后所占用的内存空间大小
    used_memory -= oldsize;
    used_memory += redis_malloc_size(newptr);
    return newptr;
#else
    realptr = (char*)ptr-sizeof(size_t);
    //前sizeof(size_t)存放着分配的内存空间大小
    oldsize = *((size_t*)realptr);
    //重新分配内存空间的大小
    newptr = realloc(realptr,size+sizeof(size_t));
    if (!newptr) return NULL;
    //记录分配的内存空间大小
    *((size_t*)newptr) = size;
    used_memory -= oldsize;
    used_memory += size;
    //返回的地址
    return (char*)newptr+sizeof(size_t);
#endif
}

/*
 * redis内存空间的释放函数
 */

void zfree(void *ptr) {
//如果是apple机的话
//就会定义HAVE_MALLOC_SIZE
#ifndef HAVE_MALLOC_SIZE
    void *realptr;
    size_t oldsize;
#endif
//ptr=NULL表示ptr压根就没指向任何空间
    if (ptr == NULL) return;
//如果是在Apple机的情况下
#ifdef HAVE_MALLOC_SIZE
    used_memory -= redis_malloc_size(ptr);
    free(ptr);
#else
    //如果不是在Apple机的情况下
    //realptr指向内存空间的前sizeof(size_t)
    //个字节存放的是分配的内存空间大小
    realptr = (char*)ptr-sizeof(size_t);
    oldsize = *((size_t*)realptr);
    used_memory -= oldsize+sizeof(size_t);
    free(realptr);
#endif
}

/*
 * redis中提供的字符串复制函数
 */

char *zstrdup(const char *s) {
    //获取字符串的长度，因为C语言中字符串
    //以\0结尾，因此获取字符串长度时需要
    //+1
    size_t l = strlen(s)+1;
    //分配l个内存空间
    //实际分配空间大小为sizeof(size_t)+l
    char *p = zmalloc(l);
    //调用C语言的函数memcpy进行内存的复制
    memcpy(p,s,l);
    return p;
}

//返回使用的内存空间的大小
size_t zmalloc_used_memory(void) {
    return used_memory;
}
