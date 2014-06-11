/* SDSLib, A C dynamic strings library
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
 * 字符串操作的相关库文件
 */

#ifndef __SDS_H
#define __SDS_H

//系统头文件，与类型定义相关
#include <sys/types.h>

/*
 * sds实际上就是一个char *指针
 * 你就把它当做char*就行了
 * sds(simple dynamic string)
 */

typedef char *sds;

/*
 * redis中字符串操作最重要的一个结构
 * 此结构中存放着字符串，以及字符器的长度
 * buf用来存放字符串
 * len表示字符串的实际长度
 * free表示空间中还有多少空余空间能够使用
 */

/*
 * 最后重点提醒一下，sdshdr这个结构体
 * 实际占用的内存空间为8字节
 * buf占用的内存为0字节
 */

struct sdshdr {
    long len;
    long free;
    char buf[];
};

/*
 * 开设空间，空间大小为sizeof(sdshdr)+initlen+1
 * 将init所指向的字符串copy到开设空间buf处
 * 失败时返回NULL
 * 成功时返回sdshdr->buf的地址
 */

sds sdsnewlen(const void *init, size_t initlen);

/*
 * sdsnew与sdsnew类似
 * sdsnew中initlen为strlen(init)
 * 也是进行字符串的复制
 */

sds sdsnew(const char *init);

/*
 * 创建空字符串
 * 其实只分配了1字节
 * 即创建空字符串时的内存情况为
 * sizeof(int)+sizeof(int)+1
 */

sds sdsempty();

/*
 * 获取字符串实际占用的内存空间数量
 */

size_t sdslen(const sds s);

/*
 * 字符中复制函数
 * 与zstrdup还是有区别滴
 */

sds sdsdup(const sds s);

/*
 * 释放内存空间
 */

void sdsfree(sds s);

/*
 * 返回字符串空间中余下未使用的空间
 * 实际上就是返回sdshdr中free变量的值
 */

size_t sdsavail(sds s);
sds sdscatlen(sds s, void *t, size_t len);
sds sdscat(sds s, char *t);
sds sdscpylen(sds s, char *t, size_t len);
sds sdscpy(sds s, char *t);
sds sdscatprintf(sds s, const char *fmt, ...);
sds sdstrim(sds s, const char *cset);
sds sdsrange(sds s, long start, long end);
void sdsupdatelen(sds s);
int sdscmp(sds s1, sds s2);
sds *sdssplitlen(char *s, int len, char *sep, int seplen, int *count);
void sdstolower(sds s);

#endif
