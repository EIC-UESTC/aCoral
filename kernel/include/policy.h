/**
 * @file policy.h
 * @author 胡博文 (@921576434@qq.com)
 * @brief kernel层线程策略相关头文件
 * @version 2.0
 * @date 2025-04-03
 * 
 * @copyright Copyright (c) 2022 EIC-UESTC
 * 
 * @par 修订历史
 *     <table>
 *         <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 *         <tr><td>v1.0 <td>胡博文 <td>2022-07-14 <td>增加注释
 *         <tr><td>v2.0 <td>饶洪江 <td>2025-04-03 <td>规范代码风格
 */
#ifndef KERNEL_POLICY_H
#define KERNEL_POLICY_H
#include "thread.h"

/* policy类型：基底 */
#define ACORAL_SCHED_POLICY_BASE 20
/* policy类型：普通线程策略 */
#define ACORAL_SCHED_POLICY_COMM   ACORAL_SCHED_POLICY_BASE + 1
/* policy类型：周期线程策略 */
#define ACORAL_SCHED_POLICY_PERIOD ACORAL_SCHED_POLICY_BASE + 2
/* policy类型：时间确定性线程策略 */
#define ACORAL_SCHED_POLICY_TIMED  ACORAL_SCHED_POLICY_BASE + 3

/**
 * @brief 调度策略结构体
 * policy_thread_init 函数指针的作用是指向特定调度策略的线程初始化例程。它本身不包含代码，而是存储了一个函数的地址。
 * 这个地址是在系统初始化或策略注册时，根据具体的调度策略（如COMM、FIFO、PERIOD等）被设置的。
 * 
 * 理解：
 * 1.acoral_sched_policy_t 是一个“遥控器”的蓝图。
 * 2.acoral_comm_policy 是一个具体的“COMM策略遥控器”实例。
 * 3.comm_policy_init() 是“组装这个COMM策略遥控器”的工厂函数。
 * 4.policy_thread_init 是遥控器上的一个按钮（函数指针）。
 * 5.comm_policy_thread_init 是按下这个按钮后实际执行的具体动作（一个具体的函数）。
 * 6.comm_policy_init() 的任务就是把 comm_policy_thread_init 这个“具体动作”连接到遥控器的 policy_thread_init “按钮”上。
 */
typedef struct
{
    acoral_list_t list; /* 链表节点 */
    acoral_u8     type; /* 策略类型 */
    acoral_id   (*policy_thread_init)(acoral_thread_t *, 
                                      void (*route)(void *args),
                                      void *,
                                      void *,
                                      void *
                                     ); /* 策略线程初始化函数 */
    void        (*policy_thread_release)(acoral_thread_t *); /* 策略回收释放函数 */
    void        (*time_deal)(void); /* 策略时间处理函数 */
    acoral_char  *name; /* 策略名称 */
} acoral_sched_policy_t;

acoral_id create_thread_by_policy(void                (*route)(void *args),
                                  acoral_u32            stack_size,
                                  void                 *args,
                                  acoral_char          *name,
                                  void                 *stack,
                                  acoral_u32            policy_type,
                                  void                 *p_data, 
                                  void                 *data,
                                  acoral_thread_hook_t *hook
);
void acoral_register_sched_policy(acoral_sched_policy_t *policy);
void acoral_policy_time_deal(void);
void acoral_policy_thread_release(acoral_thread_t *thread);
void acoral_sched_policy_init(void);
#endif
