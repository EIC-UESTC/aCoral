/**
 * @file posix_unistd.c
 * @author 李杰
 * @brief unistd.h 中函数的实现。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 本文件提供了 POSIX unistd.h中定义的标准函数实现，主要包括时间延迟功能。
 * 此外，它还定义了线程局部存储的 acoral_errno变量，以支持 POSIX 风格的错误报告机制。
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建。
 * </table>
 */

/* posix兼容层头文件  */
#include "acoral_posix.h"

/**
 * @brief 线程局部错误码变量。
 * 使用 __thread 关键字声明 acoral_errno 为线程局部存储。
 */
__thread int acoral_errno;


/**
 * @brief 暂停(Suspend)程序执行一段时间（单位：秒）
 *
 * http://pubs.opengroup.org/onlinepubs/9699919799/functions/sleep.html
 *
 * @param[in] seconds 要暂停执行的秒数。
 *
 * @return 0 - 表示成功完成，即睡眠时间全部耗尽。
 *
 * @note 当前尚未支持返回值为正数（即被中断后返回剩余秒数）的情况。
 */
unsigned int sleep(unsigned seconds)
{
	/* 延时当前线程 (ms)*/
	acoral_delay_ms((acoral_time)seconds * 1000U);

    return 0;
}


/**
 * @brief 暂停(Suspend)程序执行一段微秒时间
 *
 * 这是一个实用但非 POSIX 标准的函数。
 * @param[in] usec 要暂停执行的微秒数。
 *
 * @retval 0 - 表示成功完成。
 */
int usleep( useconds_t usec )
{
	/* 微秒转毫秒（向上取整）：(usec_val + 999) / 1000 */
    acoral_time delay_ms = ((acoral_time)usec + 999U) / 1000U;

    /* 调用 acoral 的延时函数（单位：毫秒） */
    return acoral_delay_ms(delay_ms);
}


