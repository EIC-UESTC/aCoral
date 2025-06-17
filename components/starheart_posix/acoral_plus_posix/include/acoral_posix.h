/**
 * @file acoral_posix.h
 * @author 李杰
 * @brief acoral的 POSIX 兼容层主头文件。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 确保在所有其他 acoral+posix 相关头文件之前包含此文件，以便正确配置和初始化所有必要的数据类型和宏。
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建，集成acoral和posix相关头文件。
 * </table>
 */


#ifndef _ACORAL_POSIX_H_
#define _ACORAL_POSIX_H_

/*
 * @brief POSIX 平台特定配置头文件
 */
#include "portable/acoral_posix_portable_default.h"



/*
 * @brief POSIX 基础类型和内部结构体配置头文件
 */
#include "acoral_posix/acoral_sys/types.h"

/*
 * @brief POSIX 兼容层的核心头文件
 * 这些头文件实现了 POSIX 标准中的各种 API 函数和相关宏定义。
 */
#include "acoral_posix/utils.h"
#include "acoral_posix/time.h"
#include "config.h"
#include "acoral_posix/unistd.h"
#include "acoral_posix/errno.h"
#include "acoral_posix/sched.h"
#include "acoral_posix/mqueue.h"


/*
 * @brief kernel层核心头文件 (以便 POSIX 层能够调用 kernel提供的底层服务)
 */
#include "thread.h"
#include "timer.h"
#include "lsched.h"




#endif /* _ACORAL_POSIX_H_ */
