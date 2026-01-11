/**
 * @file time.h
 * @author 李杰
 * @brief POSIX 兼容层的时间类型定义头文件。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 此头文件定义了与 POSIX 时间管理相关的核心数据类型、常量、宏和函数。
 *
 * @par 参考
 * 主要参考 POSIX.1-2008 标准中的 time.h 定义，在线文档：
 * http://pubs.opengroup.org/onlinepubs/9699919999/basedefs/time.h.html
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建，定义时间相关的基本类型、常量、结构体和函数。
 * </table>
 */

#ifndef _ACORAL_POSIX_TIME_H_
#define _ACORAL_POSIX_TIME_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ACORAL+POSIX includes. */
#include "acoral_sys/types.h"
#include "signal.h"


/**
 * @name 时间单位换算常量
 * @brief 定义用于时间单位之间转换的常量，方便计算秒、微秒和纳秒之间的关系。
 */
#define MICROSECONDS_PER_SECOND    ( 1000000LL )                                   /**< 每秒微秒数 */
#define NANOSECONDS_PER_SECOND     ( 1000000000LL )                                /**< 每秒纳秒数 */
#define NANOSECONDS_PER_TICK       ( NANOSECONDS_PER_SECOND / CFG_TICKS_PER_SEC ) /**< 每个RTOS时钟节拍对应的纳秒数 */


/**
 * @name 时钟标识符
 * @brief 指定用于计时或获取时间信息的时钟源。
 */
#define CLOCK_REALTIME     0     /**< 实时时钟，表示系统当前真实时间 */
#define CLOCK_MONOTONIC    1     /**< 单调时钟，表示不可回退的持续时间 */

/**
 * @name 转换 clock() 函数返回值为秒的换算因子
 * @brief 定义 clock()函数返回的 clock ticks与秒之间的换算关系。
 */
#define CLOCKS_PER_SEC    ( ( clock_t ) CFG_TICKS_PER_SEC )



/**
 * @name 指示时间为绝对时间的标志
 * @brief 用于定时器函数，指示所提供的时间是绝对时间点（相对于时钟源的纪元），而非相对延迟。
 */
#define TIMER_ABSTIME    0x01


#if !defined( posixconfigENABLE_TIMESPEC ) || ( posixconfigENABLE_TIMESPEC == 1 )

/**
 * @brief 表示一个时间段
 */
    struct timespec
    {
        time_t tv_sec; /**< 秒 */
        long tv_nsec;  /**< 纳秒 */
    };
#endif

#if !defined( posixconfigENABLE_ITIMERSPEC ) || ( posixconfigENABLE_ITIMERSPEC == 1 )

/**
 * @brief 定时器时间结构
 */
    struct itimerspec
    {
        struct timespec it_interval; /**< 定时器周期（重复间隔） */
        struct timespec it_value;    /**< 定时器到期时间 */
    };
#endif


clock_t clock( void );
int clock_getcpuclockid( pid_t pid,
                         clockid_t * clock_id );
int clock_getres( clockid_t clock_id,
                  struct timespec * res );
int clock_gettime( clockid_t clock_id,
                   struct timespec * tp );
int clock_settime( clockid_t clock_id,
                   const struct timespec * tp );
int clock_nanosleep( clockid_t clock_id,
                     int flags,
                     const struct timespec * rqtp,
                     struct timespec * rmtp );
int nanosleep( const struct timespec * rqtp,
               struct timespec * rmtp );




int timer_create( clockid_t clockid,
                  struct sigevent * evp,
                  timer_t * timerid );
int timer_delete( timer_t timerid );
int timer_getoverrun( timer_t timerid );
int timer_gettime( timer_t timerid,
                   struct itimerspec * value );
int timer_settime( timer_t timerid,
                   int flags,
                   const struct itimerspec * value,
                   struct itimerspec * ovalue );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _ACORAL_POSIX_TIME_H_ */
