/**
 * @file lsched.h
 * @author 胡博文 (@921576434@qq.com)
 * @brief kernel层调度相关头文件
 * @version 2.0
 * @date 2025-04-10
 *
 * @copyright Copyright (c) 2022 EIC-UESTC
 *
 * @par 修订历史
 *     <table>
 *         <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 *         <tr><td>v1.0 <td>胡博文 <td>2022-07-13 <td>增加注释
 *         <tr><td>v2.0 <td>文佳源 <td>2025-04-10 <td>规范代码风格

 */
#ifndef KERNEL_LSCHED_H
#define KERNEL_LSCHED_H
#include "cpu.h"
#include "thread.h"

/**
 * @brief 线程就绪队列组结构体
 *
 */
typedef struct
{
    acoral_thread_prio_array_t array; /* 线程优先级array */
} acoral_rdy_queue_t;

// 为每一个 CPU 核心都分配了一个独立的、完整的就绪队列系统((Per-CPU Ready Queues))
extern acoral_rdy_queue_t acoral_ready_queues[CFG_MAX_CPU];

void acoral_sched_lock();
void acoral_sched_unlock();
acoral_u8 acoral_sched_is_lock(void);

void acoral_enter_critical(void);
void acoral_exit_critical(void);

acoral_u8 acoral_need_sched(void);
void acoral_set_need_sched(acoral_u8 val);

acoral_thread_t *acoral_get_running_thread(acoral_u32 cpu);
void acoral_set_running_thread(acoral_thread_t *thread);
acoral_thread_t *acoral_get_ready_thread(void);
void acoral_set_ready_thread(acoral_thread_t *thread);

/* 重定义获取最优先就绪线程函数为变量 */
#define acoral_ready_thread acoral_get_ready_thread()
/* 重定义获取当前核正在运行函数为当前线程变量 */
#define acoral_cur_thread acoral_get_running_thread(acoral_current_cpu)


void acoral_sched_init();

void acoral_sched_rdyqueue_init(void);
void acoral_sched_rdyqueue_add(acoral_thread_t *new);
void acoral_sched_rdyqueue_del(acoral_thread_t *old);

void acoral_sched(void);
void acoral_select_thread();
void acoral_run_orig_thread(acoral_thread_t *orig);
#endif
