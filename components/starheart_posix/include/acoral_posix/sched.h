/**
 * @file sched.h
 * @brief 执行调度。
 *
 * @par 描述
 * 此头文件定义了与进程或线程调度相关的常量、数据结构和函数原型，旨在提供符合 POSIX 标准的调度管理接口。
 *
 * @par 参考
 * 主要参考 POSIX.1-2017 标准中的 sched.h定义
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sched.h.html
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>1.0 <td>李杰 <td>2025-06-16 <td>首次创建。
 * </table>
 */

#ifndef _ACORAL_POSIX_SCHED_H_
#define _ACORAL_POSIX_SCHED_H_

#include "acoral_posix/acoral_sys/types.h"
#include "acoral_posix/time.h"

#include "kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief aCoral 用户线程可用优先级的数量。
 */
#define ACORAL_NUM_USER_PRIOS (ACORAL_MAX_PRIO_NUM - 1)		//61-1=60

/**
 * @brief POSIX SCHED_OTHER调度策略的固定优先级。
 * 这是系统默认的非实时调度策略优先级。
 */
#define POSIX_SCHED_OTHER_PRIORITY   0

/**
 * @brief POSIX 实时调度策略（SCHED_FIFO和 SCHED_RR）的最小优先级。
 */
#define POSIX_REALTIME_PRIORITY_MIN 1

/**
 * @brief POSIX 实时调度策略（SCHED_FIFO 和 SCHED_RR）的最大优先级。
 */
#define POSIX_REALTIME_PRIORITY_MAX  ACORAL_NUM_USER_PRIOS	//60


/**
 * @brief 系统默认的非实时调度策略(通常是分时调度)
 */
#define SCHED_OTHER 	0

/**
 * @brief 实时调度策略：先进先出 (FIFO)
 * 基于优先级、抢占式、同优先级无时间片轮转的调度
 */
#define SCHED_FIFO		1

/**
 * @brief 实时调度策略：时间片轮转 (Round Robin)
 * 与 SCHED_FIFO 类似，但增加了时间片轮转
 */
#define SCHED_RR 		2


/**
 * @brief 实现每种支持的调度策略所需的调度参数，用于在应用程序和操作系统之间传递调度优先级信息。
 */
struct sched_param
{
    int sched_priority; /**< 进程或线程执行的调度优先级  */
};


int sched_yield(void);

int sched_get_priority_min(int policy);
int sched_get_priority_max(int policy);

int sched_getparam(pid_t pid, struct sched_param *param);
int sched_getscheduler(pid_t pid);

int sched_setparam(pid_t pid, const struct sched_param *param);
int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
int sched_rr_get_interval(pid_t pid, struct timespec *interval);



#ifdef __cplusplus
}
#endif


#endif /* ifndef _ACORAL_POSIX_SCHED_H_ */
