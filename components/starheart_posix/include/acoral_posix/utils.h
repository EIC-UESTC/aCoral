/**
 * @file utils.h
 * @author 李杰
 * @brief POSIX 兼容层使用的通用工具函数。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 此头文件包含了一系列在  POSIX 兼容层中使用的辅助工具函数。
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建，定义了时间处理和字符串长度计算等工具函数。
 * </table>
 */

#ifndef _ACORAL_POSIX_UTILS_
#define _ACORAL_POSIX_UTILS_

#ifdef __cplusplus
extern "C" {
#endif

/* C standard library includes. */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

/* posix includes. */
#include "time.h"

/* kernel includes */
#include "type.h"


size_t utils_strnlen( const char * const pc_string,
                      size_t x_max_length);

int utils_absolute_timespec_to_delta_ticks( const struct timespec * const px_absolute_time,
                                        const struct timespec * const px_current_time,
										acoral_u32 * const px_result);

int utils_timespec_to_ticks( const struct timespec * const px_timespec,
						   acoral_u32 * const px_result);

void utils_nanoseconds_to_timespec( int64_t ll_source,
                                  struct timespec * const px_destination);

int utils_timespec_add( const struct timespec * const px_time_x,
                       const struct timespec * const px_time_y,
                       struct timespec * const px_result);

int utils_timespec_add_nanoseconds( const struct timespec * const px_time_x,
                                  int64_t ll_nanoseconds,
                                  struct timespec * const px_result);

int utils_timespec_subtract( const struct timespec * const px_time_x,
                            const struct timespec * const px_time_y,
                            struct timespec * const px_result  );

int utils_timespec_compare( const struct timespec * const px_time_x,
                           const struct timespec * const px_time_y);

bool utils_validate_timespec( const struct timespec * const px_timespec);

#ifdef __cplusplus
}
#endif

#endif /* ifndef _ACORAL_POSIX_UTILS_ */
