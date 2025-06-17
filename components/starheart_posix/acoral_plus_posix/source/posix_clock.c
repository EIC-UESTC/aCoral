/**
 * @file posix_clock.c
 * @author 李杰
 * @brief time.h 中时钟和时间函数的实现。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 本文件提供了 POSIX time.h中定义的时钟和时间相关函数的具体实现。
 *
 * @par 参考
 * 主要参考 POSIX.1-2008 标准中的 time.h 定义，在线文档：
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/time.html
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建。
 * </table>
 */

/* C标准库头文件  */
#include <stddef.h>
#include <string.h>

/* posix 兼容层头文件 */
#include "acoral_posix.h"

/* snprintf 的声明。没有包含头文件 stdio.h，因为在某些平台上它会引入符号冲突。 */
extern int snprintf( char * s,
                     size_t n,
                     const char * format,
                     ... );


/**
 * @brief 报告CPU使用的时间
 *
 * @return clock_t 当前CPU时间的使用量，或 -1 表示不支持。
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock.html
 *
 */
clock_t clock( void )
{
    /* This function is currently unsupported. It will always return -1. */
    return ( clock_t ) -1;
}


/**
 * @brief 获取指定进程（或线程）的 CPU 时间时钟 ID (CPU-time clock ID)。
 *
 * @param pid 要获取其CPU时钟ID的进程/线程ID。
 * @param clock_id 指向 clockid_t 变量的指针，用于存储获取到的时钟ID。
 *
 * @return 0 成功。
 * 		   EPERM 表示当前函数未实现。
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_getcpuclockid.html
 */
int clock_getcpuclockid( pid_t pid,
                         clockid_t * clock_id )
{
    /* 为了避免编译器出现未使用参数的警告，显式将参数标记为未使用  */
    ( void ) pid;
    ( void ) clock_id;

    /* 当前该函数未实现，统一返回错误码 EPERM（不允许的操作） */
    return EPERM;
}


/**
 * @brief 获取指定时钟的分辨率 (resolution)。分辨率是指该时钟能够区分的最小时间间隔。
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_getres.html
 *
 * @para clockid_t clock_id:指定要查询分辨率的时钟的 ID。常见的时钟 ID 有：
 * 	     CLOCK_REALTIME: 系统范围的实时时钟。
 * 	     CLOCK_MONOTONIC: 系统启动后单调递增的时钟。
 * 	     struct timespec *res:一个指向 timespec 结构体的指针。如果函数成功，并且 res 不为 NULL，则此结构体将被填充为指定时钟的分辨率。
 * 	             如果 res 为 NULL，函数仅检查 clock_id 的有效性。
 *
 * @retval 0: 成功。
 * 		  -1: 发生错误，并设置 errno。常见的错误码有：
 * 		   EINVAL: clock_id 无效或不受系统支持。
 * 		   EFAULT: res 指向的内存无效 (在某些实现中，如果 res 非 NULL 但无效)。
 *
 */
int clock_getres( clockid_t clock_id,
                  struct timespec * res )
{
	if (CLOCK_MONOTONIC != clock_id)
	{
		errno = EINVAL;
		return -1;
	}

    /* 将 RTOS 的 tick 分辨率转换为 timespec */
    if (NULL != res)
    {
        res->tv_sec = 0;

        /* RTOS 一个时钟节拍（tick）对应的最小单位时间（纳秒） */
        res->tv_nsec = (long)NANOSECONDS_PER_TICK;
    }

    return 0;
}


/**
 * @brief 获取指定时钟的当前时间
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_gettime.html
 *
 * @param clock_id 指定要查询时间的时钟 ID。
 * @param tp 指向 timespec 结构体的指针，用于存储获取到的当前时间。
 *
 * @note 忽略 clock_id 参数
 * @note 不检查 ticks 类型的溢出。
 *
 * @retval 0: 成功，*tp 被填充为当前时间。
 * 		  -1: 发生错误，并设置 errno。常见的错误码有：
 * 		  EINVAL: clock_id 无效或不受系统支持。
 * 		  EFAULT: tp 指向的内存无效。
 */
int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	if (CLOCK_MONOTONIC != clock_id)
	{
		errno = EINVAL;
		return -1;
	}

    if (NULL == tp)
    {
    	errno = EINVAL;
    	return -1;
    }

    /* 获取当前 Tick（无溢出处理） */
    uint32_t ticks = acoral_get_ticks();

    /* 计算纳秒总数 */
    uint64_t total_ns = (uint64_t)ticks * NANOSECONDS_PER_TICK;

    /* 转换为 timespec 结构体 */
    utils_nanoseconds_to_timespec(total_ns, tp);

    return 0;
}



/**
 * @brief 设置指定时钟的当前时间
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_settime.html
 *
 * @param clock_id 指定要设置时间的时钟 ID。
 * @param tp 指向 timespec 结构体的指针，其中包含要设置的新时间。
 *
 * @retval  0: 成功设置时间。
 * 			-1: 发生错误，并设置 errno。常见的错误码有：
 * 			EINVAL: clock_id 无效，或者 tp 指向的时间值无效 (例如 tv_nsec 超出范围)，或者尝试设置一个不允许被设置的时钟。
 * 			EFAULT: tp 指向的内存无效。
 */
int clock_settime( clockid_t clock_id,
                   const struct timespec * tp )
{
	if (CLOCK_MONOTONIC != clock_id)
	{
		errno = EINVAL;
		return -1;
	}

	if(NULL == tp)
	{
		errno = EINVAL;
		return -1;
	}

	/* 将时间换算为tick */
	acoral_time ticks = (acoral_u64)tp->tv_sec * CLOCKS_PER_SEC + ((acoral_u64)tp->tv_nsec * CLOCKS_PER_SEC) / 1000000000ULL;

	acoral_set_ticks(ticks);

	return 0;
}



/**
 * @brief 使用指定时钟进行高精度睡眠。
 * 用于使当前线程/任务暂停执行一段指定的时间，或者暂停执行直到某个特定的绝对时间点。
 * 它提供了比 nanosleep 和 sleep 更灵活和精确的控制。
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_nanosleep.html
 *
 * @para clockid_t clock_id:指定用于计时的时钟源。
 *       在 RTOS 中，CLOCK_MONOTONIC 通常映射到系统的心跳时钟 (tick counter)，
 *       而 CLOCK_REALTIME 需要 RTC (Real-Time Clock)硬件支持。
 * @para int flags:控制rqtp参数的解释方式。
 * 		 0: 表示 rqtp 指定的是一个相对时间间隔。线程将从当前时间点开始，睡眠 rqtp 所指定的时间长度。
 * 		 TIMER_ABSTIME: 表示 rqtp 指定的是一个绝对时间点。线程将睡眠直到 clock_id 指定的时钟到达 rqtp 所表示的时间。
 * 		   如果 rqtp 指定的时间已经过去，函数会立即返回。
 * @para const struct timespec *rqtp:指向一个 timespec 结构体，用于指定期望的睡眠时间。
 *       如果 flags 为 0，rqtp 表示相对睡眠时长。
 *       如果 flags 为 TIMER_ABSTIME，rqtp 表示绝对唤醒时间。
 * @para struct timespec *rmtp:
 * 		 如果睡眠被信号中断 (interrupted by a signal)，并且 rmtp 不为 NULL，则此指针指向的 timespec 结构体将被填充为剩余未睡眠的时间 (仅当 flags 为 0 时有意义，即相对睡眠)。
 * 		 如果睡眠正常完成，rmtp 的内容未定义（通常是0或者不被修改）。
 * 		 如果 flags 是 TIMER_ABSTIME，即使被中断，rmtp 也不会被设置。
 * 		 如果 rmtp 为 NULL，则不返回剩余时间。
 *
 * @retval 0:成功完成睡眠
 * @retval 非零错误码 (errno)：发生错误。常见的错误码有：
 * 		   EINTR: 睡眠被信号中断。
 *         EINVAL: clock_id 无效，或者 tv_nsec 字段不在 0 到 999,999,999 范围内，或者 rqtp 为 NULL。
 *         ENOTSUP: 指定的 clock_id 在此系统上不受支持。
 *
 * @note 忽略 clock_id，因为使用 RTOS 节拍计数。
 * @note 忽略 flags 参数。
 * @note 忽略 rmtp 参数，因为未实现信号。
 */
int clock_nanosleep( clockid_t clock_id,			 /* 时钟标识符，指定使用的时钟类型 */
                     int flags,						 /* 标志位，指定是否使用绝对时间 */
                     const struct timespec * rqtp,	 /* 请求的时间间隔，格式为timespec结构 */
                     struct timespec * rmtp )		 /* 返回剩余的时间间隔，如果非NULL */
{
	acoral_u32 sleep_duration_ticks = 0;   /* 存储转换后的 ticks */
	acoral_time sleep_duration_ms = 0;     /* 存储要传递给 acoral_delay_ms 的毫秒数 */

	/* 1.忽略 rmtp(不支持信号中断唤醒并返回剩余时间) */
    ( void ) rmtp;

    /* 2. 参数检查：clock_id */
    if (clock_id != CLOCK_MONOTONIC)
    {
    	errno = EINVAL;
    	return -1;
    }

    /* 3. 参数检查：rqtp */
    if ((NULL == rqtp) || (false == utils_validate_timespec(rqtp)))
    {
    	errno = EINVAL;
    	return -1;
    }

    /* 4. 参数检查：flags(仅支持相对时间) */
    if (0 != flags)
    {
    	errno = ENOTSUP;
    	return -1;
    }

    /* 5. timespec 到 ticks 的转换 */
    if (0 != utils_timespec_to_ticks(rqtp, &sleep_duration_ticks))
    {
    	errno = EINVAL;
    	return -1;
    }

    /* 6.将 ticks 转换为毫秒，以供 acoral_delay_ms 使用 */
    /* 使用64位整数进行中间计算，防止 (sleep_duration_ticks * 1000) 溢出 */
    sleep_duration_ms = (acoral_time)(((uint64_t)sleep_duration_ticks * 1000U) / CFG_TICKS_PER_SEC);

    /* 7.实际延时 */
    acoral_delay_ms(sleep_duration_ms);

    return 0;
}


/**
 * @brief 使调用线程暂停执行一段时间（简化版本）。
 *
 * 此版本的 nanosleep 函数：
 * - 忽略 rmtp 参数（不报告因信号中断而剩余的时间）。
 * - 不处理因信号引起的中断（EINTR）。延时将运行到完成，或直到任务被其他 acoral 机制唤醒，但不会返回 EINTR。
 * - 依赖 acoral_delay_ms 函数进行实际的线程挂起。
 *
 * @param rqtp 指向 timespec 结构的指针，指定请求的延时间隔。
 * @param rmtp 指向 timespec 结构的指针，用于在被中断时存储剩余时间。
 *             在此实现中，此参数被完全忽略。
 * @return 成功时返回 0。失败时返回 -1，并设置相应的 errno。
 */
int nanosleep(const struct timespec * rqtp,
               struct timespec * rmtp)
{
	acoral_u32 sleep_ticks = 0;   /* 需要休眠的 tick 数 */
	acoral_time sleep_ms = 0;     /* 需要传递给 acoral_delay_ms 的毫秒数 */

	/* 1. 忽略 rmtp 参数 */
	(void)rmtp; // 使用 (void) 显式忽略，避免编译器警告

	/* 2. 验证输入时间 rqtp 是否合法 */
	/* POSIX nanosleep 要求 rqtp->tv_sec >= 0 且 0 <= rqtp->tv_nsec < 1000000000 */
	if ((NULL == rqtp) || (rqtp->tv_sec < 0) || (false == utils_validate_timespec(rqtp)))
	{
		errno = EINVAL;
	    return -1;
	}

	/* 3. 将 rqtp 指定的时间转换为系统 tick 数 */
	if (0 != utils_timespec_to_ticks(rqtp, &sleep_ticks))
	{
		errno = EINVAL;
		return -1;
	}

	 /* 4. 如果计算出的 tick 数为 0，则立即返回（符合 POSIX 对 {0,0} 的行为） */
	if (0 == sleep_ticks)
	{
		return 0; // 成功“休眠”了零时间
	}

	/* 5. 将 tick 数转换为毫秒，以便调用 acoral_delay_ms */
	sleep_ms = (acoral_time)(((uint64_t)sleep_ticks * 1000ULL) / CFG_TICKS_PER_SEC);

	/* 6. 调用 acoral RTOS 延时函数进行实际的休眠 */
	acoral_delay_ms(sleep_ms);

	return 0;
}


