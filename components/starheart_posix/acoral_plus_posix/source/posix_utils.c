/**
 * @file posix_utils.c
 * @author 李杰
 * @brief utils.h中定义的通用工具函数的实现。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 本文件包含了在  POSIX 兼容层中使用的辅助工具函数的具体实现。
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建。
 * </table>
 */

/* posix兼容层头文件 */
#include "acoral_posix.h"



/**
 * @brief 安全地计算字符串长度，最多检查 x_max_length  个字符。
 *
 * @param pc_string 要计算长度的字符串指针。
 * @param x_max_length 要检查的最大字符数。
 *
 * @return size_t 字符串长度（不包含 '\0'），最大不超过 x_max_length。
 *
 * @note 该函数适用于不确定字符串是否以 '\0' 结尾的场景，可避免越界访问。
 */
size_t utils_strnlen(
    const char * const pc_string,
	size_t x_max_length
)
{
	/* 指向当前检查的字符 */
    const char * pc_char_pointer  = pc_string;
    /* 当前已计数的字符长度 */
    size_t x_length  = 0;

    /* 空指针保护 */
    if (NULL != pc_string)
    {
    	/* 遍历字符串，直到遇到 '\0' 或达到最大长度限制 */
        while ((*pc_char_pointer != '\0') && (x_length < x_max_length))
        {
        	x_length++;			/* 递增长度计数 */
        	pc_char_pointer++;	/* 移动到下一个字符 */
        }
    }

    return x_length;			/* 返回计算得到的长度 */
}


/**
 * @brief 将绝对时间与当前时间的差值转换为 RTOS tick 数。
 *
 * 该函数常用于实现定时等待，根据目标绝对时间和当前时间，计算还需等待多少 RTOS tick。
 *
 * @param px_absolute_time 绝对时间（目标时间点）
 * @param px_current_time 当前时间。
 * @param px_result 输出：从当前时间到目标时间所需的 tick 数。
 *
 * @return  0          成功，pxResult 中写入 tick 数。
 *      	EINVAL     输入参数非法或计算失败。
 *   		ETIMEDOUT  目标时间早于当前时间，表示已经超时。
 */
int utils_absolute_timespec_to_delta_ticks(
    const struct timespec * const px_absolute_time,
    const struct timespec * const px_current_time,
	acoral_u32 * const px_result
)
{
    int i_status = 0;
    /* 用于保存 px_absolute_time - px_current_time 的差值 */
    struct timespec x_difference  = { 0 };

    /* 参数有效性检查：任意一个为 NULL 则返回参数错误  */
    if ((NULL == px_absolute_time) || (NULL == px_current_time) || (NULL == px_result))
    {
    	i_status = EINVAL;
    }

    /* 计算当前时间与目标绝对时间的时间差 */
    if (0 == i_status)
    {
    	i_status = utils_timespec_subtract(px_absolute_time, px_current_time, &x_difference);

        if (1 == i_status)
        {
        	/* 目标时间已过（px_absolute_time < px_current_time） */
        	i_status = ETIMEDOUT;
        }
        else if (-1 == i_status)
        {
            /* 时间差计算出错，例如溢出或负数非法等 */
        	i_status = EINVAL;
        }
    }

    /* 将时间差转换为 RTOS tick 数（输出至 px_result） */
    if (0 == i_status)
    {
    	i_status = utils_timespec_to_ticks( &x_difference, px_result );
    }

    return i_status;
}


/**
 * @brief 将 timespec 结构表示的时间转换为 RTOS 系统 tick 数。
 *
 * 支持将纳秒级时间转换为精确的 RTOS tick 单位，常用于定时器、超时处理等。
 *
 * @param px_timespec 输入时间（秒 + 纳秒）。
 * @param px_result 输出转换后的 tick 数。
 *
 * @return 0        转换成功。
 * 		   EINVAL   参数非法或转换结果超出 acoral_u32 表示范围。
 */
int utils_timespec_to_ticks(
    const struct timespec * const px_timespec,
	acoral_u32 * const px_result
)
{
    int i_status = 0;
    /* 最终的 tick 总数（64 位中间变量，防止溢出） */
    int64_t ll_total_ticks  = 0;
    long l_nanoseconds  = 0;

    /* 1. 参数合法性检查 */
    if ((NULL == px_timespec) || (NULL == px_result))
    {
    	i_status = EINVAL;
    }
    else if ((0 == i_status) && (false == utils_validate_timespec(px_timespec)))
    {
    	/* 检查 timespec 是否是合法时间（例如：0 <= tv_nsec < 1e9） */
    	i_status = EINVAL;
    }

    if (0 == i_status)
    {
        /* 2. 将秒部分转换为 tick：秒 × CFG_TICKS_PER_SEC */
    	ll_total_ticks = (int64_t)CFG_TICKS_PER_SEC * (px_timespec->tv_sec);

        /* 3. 将纳秒部分转换为 tick，并四舍五入向上（防止 tick 过少） */
    	l_nanoseconds = (long)((long)px_timespec->tv_nsec / (long)NANOSECONDS_PER_TICK) +
                       (long)(((long)px_timespec->tv_nsec % (long)NANOSECONDS_PER_TICK) != 0);

        /* 4. 累加纳秒部分对应的 tick */
    	ll_total_ticks += (int64_t)l_nanoseconds;

        /* 5. 溢出检查（int64_t 溢出） */
        if (0 > ll_total_ticks)
        {
        	i_status = EINVAL;
        }
        else
        {
            /* 6. 检查最终结果是否能存入 acoral_u32 */
            uint32_t ul_tick_type_size  = (uint32_t)sizeof(acoral_u32);

            /* 如果 acoral_u32 是 32 位，需要做上溢检查 */
            if ((uint32_t)sizeof(uint32_t) == ul_tick_type_size)
            {
                if (ll_total_ticks  > (int64_t)UINT_MAX) /* 显式转换为 int64_t 进行比较 */
                {
                	i_status = EINVAL;
                }
            }
        }

        /* 7. 写入转换结果 */
        *px_result = (acoral_u32)ll_total_ticks;
    }

    return i_status;
}


/**
 * @brief 将 64 位整数表示的纳秒时间转换为 timespec 结构体。
 *
 * 支持负时间处理，确保转换后 timespec 满足合法性约束：
 *     - tv_nsec >= 0
 *     - 0 <= tv_nsec < 1_000_000_000
 *
 * @param ll_source 输入的时间（单位：纳秒，可为负数）
 * @param px_destination 输出 timespec 结构体（秒 + 纳秒）。
 */
void utils_nanoseconds_to_timespec(
    int64_t ll_source,
	struct timespec * const px_destination
)
{
    if (NULL == px_destination)
    {
    	errno = EINVAL;
    	return;
    }

    /* 1. 计算秒部分 */
    int64_t seconds = ll_source / (int64_t)NANOSECONDS_PER_SECOND;

    /* 2. 计算初始纳秒部分 */
    int64_t nanoseconds = ll_source % (int64_t)NANOSECONDS_PER_SECOND;

    /*
     * 3.调整纳秒部分和秒部分以满足 timespec 约束:
     * 	   如果 ll_source 是负数，nanoseconds 可能是负数,
     * 	    需要确保 0 <= tv_nsec < NANOSECONDS_PER_SECOND。
     */
    if (0 > nanoseconds)
    {
    	nanoseconds += (int64_t)NANOSECONDS_PER_SECOND; 	/* 将纳秒部分调整为正数 */
    	seconds--;                   						/* 将纳秒部分调整为正数 */
    }

    // 4. 赋值给目标 timespec 结构体，并进行类型转换
    px_destination->tv_sec = (time_t)seconds;
    px_destination->tv_nsec = (long)nanoseconds;
}


/**
 * @brief 计算两个 timespec 时间结构体之和。
 *
 * 输出结果满足 POSIX timespec 的约定：
 *     - tv_nsec 在 [0, 1000000000) 范围内。
 *     - 若发生秒或纳秒加法溢出，返回 1。
 *     - 若参数为 NULL，返回 -1。
 *
 * @param px_time_x 被加数 timespec。
 * @param px_time_y 加数 timespec。
 * @param px_result 结果 timespec。
 *
 * @return 0 表示成功；
 *         1 表示加法过程中的时间值溢出；
 *        -1 表示参数无效。
 */
int utils_timespec_add(
    const struct timespec * const px_time_x,
    const struct timespec * const px_time_y,
    struct timespec * const px_result
)
{
	/* 存放纳秒加法产生的进位秒 */
    int64_t ll_partial_sec  = 0;
    int i_status = 0;

    /* 1. 参数合法性检查 */
    if ((NULL == px_result) || (NULL == px_time_x) || (NULL == px_time_y))
    {
    	i_status = -1;
    }

    if (0 == i_status)
    {
    	/* 2. 纳秒部分相加 */
    	px_result->tv_nsec = px_time_x->tv_nsec + px_time_y->tv_nsec;

        /* 3. 检查纳秒加法是否导致溢出（即结果为负，不合法） */
        if (0 > px_result->tv_nsec)
        {
        	i_status = 1;
        }
        else
        {
        	/* 4. 纳秒转换为进位秒，加到秒部分 */
        	ll_partial_sec = (int64_t)(px_result->tv_nsec) / (int64_t)NANOSECONDS_PER_SECOND;

            /* 保留纳秒部分在合法范围 */
            px_result->tv_nsec = (long)((int64_t)px_result->tv_nsec % (int64_t)NANOSECONDS_PER_SECOND);

            /* 5. 秒部分相加，加上进位秒 */
            px_result->tv_sec = px_time_x->tv_sec + px_time_y->tv_sec + (time_t)ll_partial_sec;

            /* 6. 最终秒值为负，表示溢出 */
            if (0 > px_result->tv_sec)
            {
            	i_status = 1;
            }
        }
    }

    return i_status;
}


/**
 * @brief 将指定的纳秒数加到给定的 timespec 结构体中。
 *
 * 该函数会将指定的纳秒数加到 px_time_x 所指向的 timespec结构体中，
 * 并将结果存储到 px_result 中。若发生溢出（纳秒或秒数溢出），函数将返回错误状态。
 *
 * @param px_time_x 指向要加上纳秒的原始 timespec结构体指针。
 * @param ll_nanoseconds 要加上的纳秒数（可以为负数）。
 * @param px_result 存储结果的 timespec结构体指针。
 *
 * @return 如果成功，则返回 0；若参数无效，则返回 -1；若发生溢出，则返回 1。
 */
int utils_timespec_add_nanoseconds(
    const struct timespec * const px_time_x,
	int64_t ll_nanoseconds,
    struct timespec * const px_result
)
{
    int64_t ll_total_nsec = 0;
    int i_status = 0;

    /* 检查参数有效性 */
    if ((NULL == px_result) || (NULL == px_time_x))
    {
    	i_status = -1;		// 如果输入参数为空，返回错误代码 -1
    }

    if (0 == i_status)
    {
    	/* 将纳秒值加到原来的时间戳 */
    	ll_total_nsec = (int64_t)px_time_x->tv_nsec + ll_nanoseconds;

        /* 检查纳秒溢出 */
        if (0 > ll_total_nsec)
        {
        	i_status = 1;	/* 如果总纳秒数为负数，则溢出 */
        }
        else
        {
        	 /* 计算新的纳秒部分，确保不超过一秒 */
        	px_result->tv_nsec = (long)(ll_total_nsec % (int64_t)NANOSECONDS_PER_SECOND);
            /* 计算增加后的秒部分 */
        	px_result->tv_sec = px_time_x->tv_sec + (time_t)(ll_total_nsec / (int64_t)NANOSECONDS_PER_SECOND);

            /* 检查秒数溢出 */
            if (0 > px_result->tv_sec)
            {
            	i_status = 1; /* 如果秒数为负，则溢出 */
            }
        }
    }

    return i_status;
}


/**
 * @brief 从一个 timespec 结构体中减去另一个 timespec 结构体。
 *
 * 该函数计算两个 timespec结构体 px_time_x 和 px_time_y 之间的差值，并将结果存储到 px_result 中。
 * 如果 px_time_x 小于 px_time_y，则返回错误状态 1；如果px_time_x 和 px_time_y 相等，则结果为零。
 * 如果计算过程中发生溢出，函数将返回错误状态 -1。
 *
 * @param px_time_x 被减去的时间。
 * @param px_time_y 减去的时间。
 * @param px_result 存储结果的 timespec结构体指针。
 *
 * @return 如果成功，则返回 0；如果 px_time_x < px_time_y，则返回 1；如果发生溢出，则返回 -1。
 */
int utils_timespec_subtract(
    const struct timespec * const px_time_x,
    const struct timespec * const px_time_y,
    struct timespec * const px_result
)
{
    int i_compare_result = 0;
    int i_status = 0;

    /* 检查参数有效性 */
    if ((NULL == px_result) || (NULL == px_time_x) || (NULL == px_time_y))
    {
    	i_status = -1;	/* 如果输入参数为空，返回错误代码 -1 */
    }

    if (0 == i_status)
    {
    	/* 比较 x 和 y 的时间 */
    	i_compare_result = utils_timespec_compare( px_time_x, px_time_y );

        /* 如果 x < y，结果为负，返回错误 1 */
        if (-1 == i_compare_result)
        {
        	i_status = 1;	/* x < y，返回错误 */
        }
        else if (0 == i_compare_result)
        {
        	/* 如果两个时间相同，返回零 */
        	px_result->tv_sec = 0;
        	px_result->tv_nsec = 0;
        }
        else
        {
        	/* 如果 x > y，执行减法 */
        	px_result->tv_sec = px_time_x->tv_sec - px_time_y->tv_sec;
        	px_result->tv_nsec = px_time_x->tv_nsec - px_time_y->tv_nsec;

            /* 检查是否需要借位处理纳秒 */
            if (0 > px_result->tv_nsec)
            {
            	/* 根据比较结果，tv_sec 应大于 0 */
            	px_result->tv_sec--;
            	px_result->tv_nsec += (long)NANOSECONDS_PER_SECOND;
            }

            /* 如果借位后纳秒依然为负，表示溢出错误 */
            if (px_result->tv_nsec < 0)
            {
            	i_status = -1;	/* 溢出错误 */
            }
        }
    }

    return i_status;
}


/**
 * @brief 比较两个 timespec 结构体的时间。
 *
 * @param px_time_x 第一个 timespec 结构体。
 * @param px_time_y 第二个 timespec 结构体。
 *
 * @return 比较结果：
 *         - 1：如果 x > y
 *         - -1：如果 x < y
 *         - 0：如果 x == y
 */
int utils_timespec_compare(
    const struct timespec * const px_time_x,
    const struct timespec * const px_time_y
)
{
    int i_status = 0;

    /* 检查参数有效性  */
    if ((NULL == px_time_x) && (NULL == px_time_y))
    {
    	i_status = 0;	  /* 如果两个都为空，认为它们相等 */
    }
    else if (NULL == px_time_y)
    {
    	i_status = 1;	 /* 如果 y 为空，则 x 大于 y */
    }
    else if (NULL == px_time_x)
    {
    	i_status = -1;	/* 如果 x 为空，则 x 小于 y */
    }
    else if (px_time_x->tv_sec > px_time_y->tv_sec)
    {
    	i_status = 1;	/* 如果秒数比较，x > y */
    }
    else if (px_time_x->tv_sec < px_time_y->tv_sec)
    {
    	i_status = -1;	/* 如果秒数比较，x < y */
    }
    else
    {
    	 /* 如果秒数相等，比较纳秒 */
        if (px_time_x->tv_nsec > px_time_y->tv_nsec)
        {
        	i_status = 1;	/* 如果纳秒数比较，x > y */
        }
        else if (px_time_x->tv_nsec < px_time_y->tv_nsec)
        {
        	i_status = -1;	/* 如果纳秒数比较，x < y */
        }
        else
        {
        	i_status = 0;	/* 如果秒数和纳秒数都相等，返回 0 */
        }
    }

    return i_status;
}


/**
 * @brief 验证 timespec 结构体的有效性。
 *
 * 该函数用于验证 timespec结构体中的纳秒部分 (tv_nsec) 是否在有效范围内，
 * 即检查 tv_nsec是否满足 0 <= tv_nsec < NANOSECONDS_PER_SECOND 的条件。
 *
 * @param px_timespec 待验证的timespec结构体。
 *
 * @return bool 返回 truetimespec结构体有效，返回 false无效。
 */
bool utils_validate_timespec(const struct timespec * const px_timespec)
{
    bool x_return = false;

    /* 检查 timespec 是否为空 */
    if (NULL != px_timespec)
    {
    	/* 验证 tv_nsec 是否在有效范围内 (0 <= tv_nsec < NANOSECONDS_PER_SECOND) */
        if ((px_timespec->tv_nsec >= 0) &&
            (px_timespec->tv_nsec < (long)NANOSECONDS_PER_SECOND))
        {
        	x_return = true;	/* 如果有效，返回 true */
        }
    }

    return x_return;
}


