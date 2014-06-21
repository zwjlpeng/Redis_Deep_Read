/* anet.c -- Basic TCP socket stuff made a bit less boring
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
 * redis里的套接字相关的操作
 */
#ifndef ANET_H
#define ANET_H

#define ANET_OK 0
#define ANET_ERR -1
#define ANET_ERR_LEN 256
/**
 * 用于建立非阻塞型的网络套接字连接
 */
int anetTcpConnect(char *err, char *addr, int port);
/**
 * 用于建立阻塞型的网络套接字连接
 */
int anetTcpNonBlockConnect(char *err, char *addr, int port);
/**
 * 用于套接字的读
 */
int anetRead(int fd, char *buf, int count);
/**
 * 用于套接字的写
 */
int anetResolve(char *err, char *host, char *ipbuf);
/**
 * 建立网络套接字服务器方法
 * 是对bind()/socket()/listen等一系列操作的封装
 * 返回套接字的fd
 */
int anetTcpServer(char *err, int port, char *bindaddr);
/**
 * 用于接收连接
 */
int anetAccept(char *err, int serversock, char *ip, int *port);
/**
 * 用于套接字的写功能
 */
int anetWrite(int fd, char *buf, int count);
/**
 * 将fd设置为非阻塞型
 */
int anetNonBlock(char *err, int fd);
/**
 * 将TCP设为非延迟的，即屏蔽Nagle算法
 */
int anetTcpNoDelay(char *err, int fd);
/**
 * 开启连机监测，避免对方塔机，网络中断是fd无限期的等待
 */
int anetTcpKeepAlive(char *err, int fd);

#endif
