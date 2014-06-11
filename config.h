#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * 如果是apple机时要包含的头文件
 */
#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif

/* test for malloc_size() */
#ifdef __APPLE__
#include <malloc/malloc.h>
//下面这个宏定义表示有malloc_size这个函数
#define HAVE_MALLOC_SIZE 1
//将redis_malloc_size宏定义为malloc_size函数
#define redis_malloc_size(p) malloc_size(p)
#endif

/* define redis_fstat to fstat or fstat64() */
#if defined(__APPLE__) && !defined(MAC_OS_X_VERSION_10_6)
//MAC机下的stat函数原型
#define redis_fstat fstat64
#define redis_stat stat64
#else
//其他操作系统下对应的stat函数
#define redis_fstat fstat
#define redis_stat stat
#endif

/* test for backtrace() */
#if defined(__APPLE__) || defined(__linux__)
//返回函数的调用栈，一般用于调试（在mac以及linux下才有）
#define HAVE_BACKTRACE 1
#endif

#endif
