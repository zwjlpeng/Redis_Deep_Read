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

/*
 * redis通过自已的方式来部署与管理内存
 * redis下层采用的是C语言来编写的，因此
 * redis提供的内存分配其实也就是对C语言中
 * 基本的内存分配函数如malloc/free/calloc/realloc
 * 的封装
 */

#ifndef _ZMALLOC_H
#define _ZMALLOC_H

/*
 * 此函数用于分配指定大小的内存空间
 * 若分配失败的话则返回NULL
 * 分配成功的话，返回空间的首地址
 */

void *zmalloc(size_t size);

/*
 * 此函数用于重新分配内存空间，若ptr指向地址
 * 处有size个空间的话，则ptr不变
 * 若不足size个空间的话，会重新分配内存空间
 * 返回新空间的首地址，将将原有数据拷贝到新
 * 空间，同时释放旧空间
 */

void *zrealloc(void *ptr, size_t size);

/*
 * 用于释放ptr指向的内存空间
 */

void zfree(void *ptr);

/*
 * 用于字符串的拷贝
 */

char *zstrdup(const char *s);

/*
 * 获取实际分配的内存空间大小
 */

size_t zmalloc_used_memory(void);

#endif /* _ZMALLOC_H */
