/**
 * @file types.h
 * @author 李杰
 * @brief POSIX 兼容层的数据类型定义头文件。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 此头文件定义了 POSIX兼容层所需的基本数据类型，旨在提供与标准 POSIX sys/types.h类似的类型定义，以增强代码的可移植性。
 * 它根据 acoral_posix_portable_default.h中的配置宏来有条件地启用或禁用某些类型的定义，以避免与工具链自带的定义冲突。
 *
 * @par 参考
 * 主要参考 POSIX.1-2008 标准，在线文档：
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_types.h.html
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建，包含 POSIX 相关数据类型定义及条件编译。
 * </table>
 */

#ifndef _ACORAL_POSIX_TYPES_H_
#define _ACORAL_POSIX_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @brief 引入标准整型定义
 */
#include <stdint.h>

/*
 * @brief 引入 acoral 平台内部定义的 POSIX 类型配置
 */
#include "portable/acoral_posix_portable_default.h"

/*
 * @brief 引用工具链自带的类型定义
 * 在某些情况下，可能需要引用工具链（或标准库）自带的 sys/types.h，但本文件会通过条件编译尽量避免重复定义。
 */
#include <sys/types.h>


/**
 * @brief 表示系统时钟滴答或 CLOCKS_PER_SEC 单位的系统时间。
 * @details 该类型用于记录和测量处理器或系统运行时间。
 * 其定义是否启用由 posixconfigENABLE_CLOCK_T配置宏控制。
 */
#if !defined( posixconfigENABLE_CLOCK_T ) || ( posixconfigENABLE_CLOCK_T == 1 )
    typedef uint32_t                 clock_t;
#endif

/**
 * @brief 表示时钟 ID，通常用于时钟和定时器函数中。
 * @details 区分不同的系统时钟源（如实时时钟、单调时钟等）。
 * 其定义是否启用由 posixconfigENABLE_CLOCKID_T配置宏控制。
 */
#if !defined( posixconfigENABLE_CLOCKID_T ) || ( posixconfigENABLE_CLOCKID_T == 1 )
    typedef int                      clockid_t;
#endif

/**
 * @brief 表示文件权限属性和文件类型。
 * @details 例如，用于设置或检查文件的读、写、执行权限，以及文件是目录还是普通文件等。
 * 其定义是否启用由 posixconfigENABLE_MODE_T配置宏控制。
 */
#if !defined( posixconfigENABLE_MODE_T ) || ( posixconfigENABLE_MODE_T == 1 )
    typedef int                      mode_t;
#endif

/**
 * @brief 表示进程 ID（Process ID）或进程组 ID（Process Group ID）。
 * @details 用于唯一标识系统中运行的进程。
 * 其定义是否启用由 posixconfigENABLE_PID_T配置宏控制。
 */
#if !defined( posixconfigENABLE_PID_T ) || ( posixconfigENABLE_PID_T == 1 )
    typedef int                      pid_t;
#endif

/**
 * @brief POSIX 线程属性对象。
 * @details 用于在创建线程时定义其属性，如栈大小、调度策略等。
 * 此类型在此处无条件定义为 void*占位符，具体实现可能在其他源文件。
 * 这符合 POSIX 标准中对不透明类型的处理方式。
 */
typedef void                         * pthread_attr_t;

/**
 * @brief 表示线程屏障属性对象。
 * @details 用于在创建线程屏障时设置其属性。
 * 此类型在此处无条件定义为 void* 占位符，具体实现可能在其他源文件。
 */
typedef void                         * pthread_barrierattr_t;


/**
 * @brief 表示条件变量属性对象。
 * @details 用于在创建条件变量时设置其属性。
 * 其定义是否启用由 posixconfigENABLE_PTHREAD_CONDATTR_T配置宏控制。
 */
#if !defined( posixconfigENABLE_PTHREAD_CONDATTR_T ) || ( posixconfigENABLE_PTHREAD_CONDATTR_T == 1 )
    typedef void                     * pthread_condattr_t;
#endif


/**
 * @brief 表示线程 ID。
 * @details 用于唯一标识一个 POSIX 线程。
 * 其定义是否启用由 posixconfigENABLE_PTHREAD_T配置宏控制。
 */
#if !defined( posixconfigENABLE_PTHREAD_T ) || ( posixconfigENABLE_PTHREAD_T == 1 )
    typedef void                     * pthread_t;
#endif

/**
 * @brief 表示字节数计数或错误返回值。
 * @details 常用于系统调用，如 read() 和 write() 函数的返回值，
 * 表示成功操作的字节数，或在错误时返回 -1。
 * 其定义是否启用由 posixconfigENABLE_SSIZE_T配置宏控制。
 */
#if !defined( posixconfigENABLE_SSIZE_T ) || ( posixconfigENABLE_SSIZE_T == 1 )
    typedef int                      ssize_t;
#endif

/**
 * @brief 表示以秒为单位的时间。
 * @details 通常用于 time() 等函数，表示自 Epoch（1970年1月1日00:00:00 UTC）以来的秒数。
 * 其定义是否启用由 posixconfigENABLE_TIME_T配置宏控制。
 */
#if !defined( posixconfigENABLE_TIME_T ) || ( posixconfigENABLE_TIME_T == 1 )
	typedef int64_t                  time_t;
#endif

/**
 * @brief 表示由 timer_create()返回的计时器 ID。
 * @details 用于标识通过 POSIX 定时器接口创建的计时器。
 * 其定义是否启用由 posixconfigENABLE_TIMER_T配置宏控制。
 */
#if !defined( posixconfigENABLE_TIMER_T ) || ( posixconfigENABLE_TIMER_T == 1 )
	typedef void                     * timer_t;
#endif

/**
 * @brief 表示微秒数。
 * @details 常用于微秒级别的延迟或时间间隔，例如 usleep()函数。
 * 其定义是否启用由 posixconfigENABLE_USECONDS_T配置宏控制。
 */
#if !defined( posixconfigENABLE_USECONDS_T ) || ( posixconfigENABLE_USECONDS_T == 1 )
	typedef unsigned long            useconds_t;
#endif

/**
 * @brief 表示文件大小或文件偏移量。
 * @details 常用于文件操作，如 lseek()（移动文件指针）和 ftruncate()（截断文件）。
 * 其定义是否启用由 posixconfigENABLE_OFF_T 配置宏控制。
 */
#if !defined( posixconfigENABLE_OFF_T ) || ( posixconfigENABLE_OFF_T == 1 )
	typedef long int                 off_t;
#endif




#ifdef __cplusplus
}
#endif

#endif /* ifndef _ACORAL_POSIX_TYPES_H_ */
