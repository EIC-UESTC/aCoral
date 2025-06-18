/**
 * @file thread.c
 * @author 胡博文 (@921576434@qq.com)
 * @brief kernel层线程相关源文件
 * @version 2.0
 * @date 2025-04-17
 *
 * @copyright Copyright (c) 2022 EIC-UESTC
 *
 * @par 修订历史
 *     <table>
 *         <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 *         <tr><td>v1.0 <td>胡博文 <td>2022-07-14 <td>增加注释
 *         <tr><td>v1.1 <td>胡博文 <td>2022-09-26 <td>错误头文件相关改动
 *         <tr><td>v1.2 <td>胡博文 <td>2023-09-09 <td>去除no_sched
 *         <tr><td>v1.3 <td>饶洪江 <td>2025-03-14 <td>在线程控制块中增加线程错误检测字段
 *         <tr><td>v2.0 <td>文佳源 <td>2025-04-17 <td>规范代码
 */
#include "type.h"
#include "hal.h"
#include "queue.h"
#include "lsched.h"
#include "cpu.h"
#include "error.h"
#include "timer.h"
#include "mem.h"
#include "res_pool.h"
#ifdef CFG_SMP
#include "ipi.h"
#endif
#include "thread.h"
#include "policy.h"
#include "list.h"
#include "bitops.h"

#include "print.h"

acoral_queue_t     acoral_threads_queue;    /* 全部线程队列 */

static acoral_res_api_t   thread_api;       /* 线程内存池api结构体实例 */
static acoral_pool_ctrl_t thread_pool_ctrl; /* 线程内存池控制结构体实例 */

/**********************************kill******************************************/

/**
 * @brief 杀死线程（调度）
 *
 * @param thread 线程tcb指针
 */
void acoral_kill_thread(acoral_thread_t *thread)
{
#ifdef CFG_SMP
    acoral_u32 cpu;
#endif
    acoral_ipc_t *ipc;
#ifdef CFG_SMP
    cpu = thread->cpu;
    /* kill */
    if (cpu != acoral_current_cpu) /* 不是当前核的线程 */
    {
        /* 使用核间中断命令 */
        acoral_ipi_cmd_send(cpu, ACORAL_IPI_THREAD_KILL, thread->res.id, NULL);

        return;
    }
#endif

    acoral_enter_critical();

    /* 根据线程状态来进行一些删除操作 */
    if (thread->state & ACORAL_THREAD_STATE_SUSPEND)
    {
        ipc = thread->ipc;
        if (!acoral_list_empty(&thread->timing))
        {
            acoral_list_del(&thread->timing);
        }
        if (!acoral_list_empty(&thread->delaying))
        {
            acoral_list_del(&thread->delaying);
        }
        if (ipc != NULL)
        {
#ifdef CFG_SMP
            acoral_spin_lock(&ipc->lock);
#endif
            acoral_ipc_wait_queue_del(ipc, thread);
#ifdef CFG_SMP
            acoral_spin_unlock(&ipc->lock);
#endif
        }
    }
    acoral_suspend_thread(thread);    /* 以不调度方式挂起线程 */
    acoral_prerelease_thread(thread); /* 预回收线程 */

    acoral_exit_critical();
}

/**
 * @brief 使用id来杀死线程（调度）
 *
 * @param id 线程id
 */
void acoral_kill_thread_by_id(acoral_id id)
{
    acoral_thread_t *thread = (acoral_thread_t *)acoral_get_res_by_id(id);
    acoral_kill_thread(thread);
}
/**********************************kill******************************************/

/**********************************ready******************************************/

/**
 * @brief 就绪线程（调度）
 *
 * @param thread 线程tcb指针
 * @return acoral_err 错误检测
 */
acoral_err acoral_rdy_thread(acoral_thread_t *thread)
{
#ifdef CFG_SMP
    acoral_u32 cpu;
#endif

    if (!(thread->state & ACORAL_THREAD_STATE_SUSPEND))
    {
        return KR_THREAD_ERR_STATE;
    }

#ifdef CFG_SMP
    cpu = thread->cpu;
    /* ready */
    if (cpu != acoral_current_cpu) /* 不是当前核的线程 */
    {
        /* 使用核间中断命令 */
        acoral_ipi_cmd_send(cpu, ACORAL_IPI_THREAD_READY, thread->res.id, NULL);

        return KR_OK;
    }
#endif

    acoral_enter_critical();
    acoral_sched_rdyqueue_add((void *)thread); /* 添加线程到就绪队列 */
    acoral_exit_critical();

    return KR_OK;
}

/**
 * @brief 使用id来就绪线程（调度）
 *
 * @param thread_id 线程id
 * @return acoral_err 错误检测
 */
acoral_err acoral_rdy_thread_by_id(acoral_u32 thread_id)
{
    acoral_thread_t *thread = (acoral_thread_t *)acoral_get_res_by_id(thread_id);
    return acoral_rdy_thread(thread);
}
/**********************************ready******************************************/

/**********************************suspend******************************************/
/**
 * @brief 挂起线程（调度）
 *
 * @param thread 线程tcb指针
 * @return acoral_err 错误检测
 */
acoral_err acoral_suspend_thread(acoral_thread_t *thread)
{
#ifdef CFG_SMP
    acoral_u32 cpu;
#endif
    if (!(thread->state & ACORAL_THREAD_STATE_READY))
    {
        return KR_THREAD_ERR_STATE;
    }
#ifdef CFG_SMP
    cpu = thread->cpu;
    /* suspend */
    if (cpu != acoral_current_cpu) /* 不是当前核的线程 */
    {
        /* 使用核间中断命令 */
        acoral_ipi_cmd_send(cpu, ACORAL_IPI_THREAD_SUSPEND, thread->res.id, NULL);

        return KR_OK;
    }
#endif
    acoral_enter_critical();
    acoral_sched_rdyqueue_del((void *)thread); /* 从就绪队列删除线程 */
    acoral_exit_critical();

    return KR_OK;
}

/**
 * @brief 挂起自己
 *
 * @return acoral_err 错误检测
 */
acoral_err acoral_suspend_self(void)
{
    return acoral_suspend_thread(acoral_cur_thread);
}

/**
 * @brief 使用id来挂起线程（调度）
 *
 * @param thread_id 线程id
 * @return acoral_err 错误检测
 */
acoral_err acoral_suspend_thread_by_id(acoral_u32 thread_id)
{
    acoral_thread_t *thread = (acoral_thread_t *)acoral_get_res_by_id(thread_id);
    return acoral_suspend_thread(thread);
}
/**********************************suspend******************************************/

/**********************************change prio******************************************/
/**
 * @brief 更改线程优先级
 *
 * @param thread 线程tcb指针
 * @param prio 要改成的优先级
 */
void acoral_thread_change_prio(acoral_thread_t *thread, acoral_u32 prio)
{
#ifdef CFG_SMP
    acoral_u32 cpu;
#endif

#ifdef CFG_SMP
    cpu = thread->cpu;
    /* change prio */
    if (cpu != acoral_current_cpu) /* 不是当前核的线程 */
    {
        /* 使用核间中断命令 */
        acoral_ipi_cmd_send(cpu, ACORAL_IPI_THREAD_CHG_PRIO, thread->res.id, (void *)prio);

        return;
    }
#endif

    acoral_enter_critical();
    if (thread->state & ACORAL_THREAD_STATE_READY) /* 如果线程是就绪状态 */
    {
        acoral_sched_rdyqueue_del((void *)thread); /* 从就绪队列删除线程 */
        thread->prio = prio;                       /* 改动优先级 */
        acoral_sched_rdyqueue_add((void *)thread); /* 重新添加线程到就绪队列 */
    }
    else
    {
        thread->prio = prio; /* 改动优先级 */
    }
    acoral_exit_critical();
}

/**
 * @brief 使用id来改变线程优先级
 *
 * @param thread_id 线程tcb指针
 * @param prio 要改成的优先级
 */
void acoral_thread_change_prio_by_id(acoral_u32 thread_id, acoral_u32 prio)
{
    acoral_thread_t *thread = (acoral_thread_t *)acoral_get_res_by_id(thread_id);
    acoral_thread_change_prio(thread, prio);
}

/**
 * @brief 改变自己的优先级
 *
 * @param prio 要改成的优先级
 */
void acoral_thread_change_prio_self(acoral_u32 prio)
{
    acoral_thread_change_prio(acoral_cur_thread, prio);
}
/**********************************change prio******************************************/

#ifdef CFG_SMP
/**********************************move to******************************************/

/**
 * @brief 迁移线程到目标cpu
 *
 * @param thread 线程tcb指针
 * @param cpu    目标cpu
 * @return acoral_err 错误检测
 */
acoral_err acoral_moveto_thread(acoral_thread_t *thread, acoral_u32 cpu)
{
    if (cpu >= CFG_MAX_CPU)
    {
        return KR_THREAD_ERR_CPU;
    }

    if (thread->cpu == cpu) /* 不需要迁移 */
    {
        return KR_OK;
    }

    acoral_thread_t *cur = acoral_cur_thread;

    acoral_enter_critical();
    acoral_spin_lock(&thread->move_lock);
    /* move to */
    if (thread->cpu != acoral_current_cpu) /* 不是当前核的线程 */
    {
        /* 使用核间中断命令 */
        acoral_ipi_cmd_send(thread->cpu, ACORAL_IPI_THREAD_MOVETO, thread->res.id, (void *)cpu);
        acoral_spin_unlock(&thread->move_lock);
        acoral_exit_critical();

        return KR_THREAD_ERR_CPU;
    }
    
    acoral_suspend_thread(thread);
    thread->state |= ACORAL_THREAD_STATE_MOVE;
    acoral_ipi_cmd_send(cpu, ACORAL_IPI_THREAD_MOVEIN, thread->res.id, NULL); /* 使用核间中断命令，告诉目标cpu有线程迁移进来 */

    if ((acoral_32)thread != (acoral_32)cur)
    {
        acoral_spin_unlock(&thread->move_lock);
        acoral_exit_critical();

        return KR_OK;
    }
    else
    {
        acoral_exit_critical();
    }

    return KR_OK;
}

/**
 * @brief 使用id来迁移线程到目标cpu
 *
 * @param thread_id 线程id
 * @param cpu       目标cpu
 * @return acoral_err 错误检测
 */
acoral_err acoral_moveto_thread_by_id(acoral_id thread_id, acoral_u32 cpu)
{
    acoral_thread_t *thread = (acoral_thread_t *)acoral_get_res_by_id(thread_id);
    return acoral_moveto_thread(thread, cpu);
}

/**
 * @brief 把当前线程迁移到目标cpu
 *
 * @param cpu 目标cpu
 * @return acoral_err 错误检测
 */
acoral_err acoral_jump_cpu(acoral_u32 cpu)
{
    return acoral_moveto_thread(acoral_cur_thread, cpu);
}
/**********************************move to******************************************/

/**********************************move in******************************************/

/**
 * @brief 迁移线程进当前cpu
 *
 * @param thread 线程tcb指针
 * @return acoral_err 错误检测
 */
acoral_err acoral_movein_thread(acoral_thread_t *thread)
{
    acoral_err err;

    acoral_spin_lock(&thread->move_lock);
    thread->cpu = acoral_current_cpu; /* 更改线程所在cpu */
    err = acoral_rdy_thread(thread);  /* 就绪线程 */
    acoral_spin_unlock(&thread->move_lock);

    return err;
}

/**
 * @brief 使用id来迁移线程进当前cpu
 *
 * @param thread_id 线程id
 * @return acoral_err 错误检测
 */
acoral_err acoral_movein_thread_by_id(acoral_id thread_id)
{
    acoral_thread_t *thread = (acoral_thread_t *)acoral_get_res_by_id(thread_id);
    return acoral_movein_thread(thread);
}
/**********************************move in******************************************/
#endif

/**********************************release******************************************/

/**
 * @brief 预回收线程
 *
 * @param thread 线程tcb指针
 * @return acoral_err 错误检测
 */
acoral_err acoral_prerelease_thread(acoral_thread_t *thread)
{
    acoral_thread_t *daem;

    thread->state = ACORAL_THREAD_STATE_EXIT;
    acoral_release_queue_add(&thread->pending); /* 添加线程到释放队列 */

    acoral_id daemon_id = get_daemon_id();
    daem = (acoral_thread_t *)acoral_get_res_by_id(daemon_id);

    return acoral_rdy_thread(daem); /* 唤醒daemon线程来回收 */
}

/**
 * @brief 实际回收线程，在daemon线程中使用
 *
 * @param res 线程tcb指针
 */
void acoral_release_thread(acoral_res_t *res)
{
    acoral_thread_t *thread = (acoral_thread_t *)res;

    acoral_lifo_queue_del(&acoral_threads_queue, &thread->global_list);
    acoral_policy_thread_release(thread);      /* 基于策略回收线程 */
    acoral_free((void *)thread->stack_buttom); /* 回收内存 */
    thread->stack_buttom = NULL;

    acoral_release_res((acoral_res_t *)thread); /* 回收res */
}
/**********************************release******************************************/

/**********************************delay******************************************/

/**
 * @brief 延时线程
 *
 * @param thread 线程tcb指针
 * @param time   延时时间
 * @return acoral_err 错误检测
 */
acoral_err acoral_delay_thread(acoral_thread_t *thread, acoral_time time)
{
    acoral_u32 real_ticks;

    if (!acoral_list_empty(&thread->delaying))
    {
        return KR_THREAD_ERR_UNDEF;
    }
#ifdef CFG_SMP
    if (thread->cpu != acoral_current_cpu)
    {
        return KR_THREAD_ERR_CPU;
    }
#endif
    /*timeticks*/
    /*real_ticks=time*ACORAL_TICKS_PER_SEC/1000;*/
    real_ticks = TIME_TO_TICKS(time); /* 换算ticks */
    thread->delay = real_ticks;
    acoral_vlist_init(&thread->delaying, real_ticks); /* 初始化链表节点 */
    acoral_time_delay_queue_add((void *)thread);      /* 添加到延时队列 */

    return KR_OK;
}

/**
 * @brief 延时自己
 *        注意：该函数会调用acoral_suspend_thread
 * @param time
 * @return acoral_err 错误检测
 */
acoral_err acoral_delay_ms(acoral_time time)
{
    return acoral_delay_thread(acoral_cur_thread, time);
}
/**********************************delay******************************************/

/**
 * @brief 线程优先级队列添加节点
 *
 * @param array 线程优先级array
 * @param prio  优先级
 * @param list  节点
 */
void acoral_thread_prio_queue_add(
    acoral_thread_prio_array_t *array,
    acoral_u8                   prio,
    acoral_list_t              *list
)
{
    acoral_queue_t *queue;

    array->num++;                        /* 线程数++ */
    queue = array->queue + prio;         /* 找到该优先级的线程队列 */
    acoral_fifo_queue_add(queue, list);  /* 以fifo方式添加线程到队列 */
    acoral_set_bit(prio, array->bitmap); /* 设置位图 */
}

/**
 * @brief 线程优先级队列删除节点
 *
 * @param array 线程优先级array
 * @param prio  优先级
 * @param list  节点
 */
void acoral_thread_prio_queue_del(
    acoral_thread_prio_array_t *array,
    acoral_u8                   prio,
    acoral_list_t              *list
)
{
    acoral_queue_t *queue;

    array->num--;                              /* 线程数-- */
    queue = array->queue + prio;               /* 找到该优先级的线程队列 */
    acoral_fifo_queue_del(queue, list);        /* 以fifo方式从队列删除线程 */
    if (acoral_list_empty(&queue->head))       /* 该优先级的线程队列为空 */
    {
        acoral_clear_bit(prio, array->bitmap); /* 设置位图 */
    }
}

/**
 * @brief 获取最高优先级线程的index
 *
 * @param array 线程优先级array
 * @return acoral_u32 最高优先级线程的index
 */
acoral_u32 acoral_thread_get_high_prio(acoral_thread_prio_array_t *array)
{
    return acoral_find_first_bit(array->bitmap, ACORAL_THREAD_PRIO_BITMAP_SIZE);
}

/**
 * @brief 线程优先级队列初始化
 *
 * @param array 线程优先级array
 */
void acoral_thread_prio_queue_init(acoral_thread_prio_array_t *array)
{
    acoral_u8 i;
    acoral_queue_t *queue;
    array->num = 0; /* 初始线程数为0 */

    for (i = 0; i < ACORAL_THREAD_PRIO_BITMAP_SIZE; i++)
    {
        array->bitmap[i] = 0; /* 初始化位图 */
    }

    for (i = 0; i < ACORAL_MAX_PRIO_NUM; i++)
    {
        queue = array->queue + i;
        acoral_fifo_queue_init(queue); /* 初始化各个优先级的线程队列 */
    }
}

/**
 * @brief 设置线程的控制台id
 *
 * @param id 控制台id
 */
void acoral_set_thread_console(acoral_id id)
{
    acoral_cur_thread->console_id = id;
}

/**
 * @brief 分配线程tcb
 *
 * @return acoral_thread_t* 分配的线程tcb指针
 */
acoral_thread_t *acoral_alloc_thread(void)
{
    return (acoral_thread_t *)acoral_get_res(&thread_pool_ctrl);
}

/**
 * @brief 通用线程初始化
 *
 * @param thread 线程tcb指针
 * @param route  线程运行函数
 * @param exit   线程退出函数
 * @param args   传递参数
 * @return acoral_err 错误检测
 */
acoral_err acoral_thread_init(
    acoral_thread_t *thread,
    void           (*route)(void *args),
    void           (*exit)(void),
    void            *args
)
{

    acoral_u32 stack_size = thread->stack_size;
    /* 线程栈计算 */
    if (thread->stack_buttom == NULL)
    {
        if (stack_size < ACORAL_MIN_STACK_SIZE)
        {
            stack_size = ACORAL_MIN_STACK_SIZE;
        }

        thread->stack_buttom = (acoral_u32 *)acoral_malloc(stack_size);

        if (thread->stack_buttom == NULL)
        {
            return KR_THREAD_ERR_NO_STACK;
        }

        thread->stack_size = stack_size;
    }

    thread->stack = (acoral_u32 *)((acoral_8 *)thread->stack_buttom + stack_size - 4);
    HAL_STACK_INIT(&thread->stack, route, exit, args); /* 线程栈初始化 */

    thread->delay        = 0;
    thread->time         = 0;
    thread->ipc          = NULL;
    thread->preempt_type = ACORAL_PREEMPT_LOCAL; /* 设置线程作用范围 */
    /* cpu_mask */
    if (thread->cpu_mask == -1)
    {
        thread->cpu_mask = 0xefffffff;
    }
    if (thread->cpu < 0)
    {
        thread->cpu = acoral_get_idle_maskcpu(thread->cpu_mask);
    }
    if (thread->cpu >= CFG_MAX_CPU)
    {
        thread->cpu = CFG_MAX_CPU - 1;
    }
    thread->data       = NULL;
    thread->state      = ACORAL_THREAD_STATE_SUSPEND;   /* 线程初始为挂起状态 */
    thread->console_id = acoral_cur_thread->console_id; /* 继承父线程的console_id */
    thread->err        = KR_OK;                         /* 初始化无错误 */

    /* 初始化各个链表节点 */
    acoral_vlist_init(&thread->timing, 0);
    acoral_vlist_init(&thread->delaying, 0);
#if CFG_IPC_QUEUE_MODE == CFG_FIFO_QUEUE
    acoral_list_init(&thread->pending);
#else
    acoral_vlist_init(&thread->pending, thread->prio);
#endif
    acoral_list_init(&thread->ready);
    acoral_list_init(&thread->global_list);

    acoral_spin_init(&thread->move_lock);
    acoral_enter_critical();
    /* 以fifo方式添加线程到全部线程队列 */
    acoral_fifo_queue_add(&acoral_threads_queue, &thread->global_list);
    acoral_exit_critical();

    return KR_OK;
}

/**
 * @brief 通用线程退出函数
 *
 */
void acoral_thread_exit(void)
{
    acoral_kill_thread(acoral_cur_thread);
}

/**
 * @brief 线程内存池初始化
 *
 */
static void acoral_thread_pool_init(void)
{
    thread_api.release_res = acoral_release_thread;

    thread_pool_ctrl.type = ACORAL_RES_THREAD;
    thread_pool_ctrl.size = sizeof(acoral_thread_t);
    if (CFG_MAX_THREAD > 20)
    {
        thread_pool_ctrl.num_per_pool = 20;
    }
    else
    {
        thread_pool_ctrl.num_per_pool = CFG_MAX_THREAD;
    }
    thread_pool_ctrl.max_pools = ACORAL_MAX_THREAD / thread_pool_ctrl.num_per_pool;
    thread_pool_ctrl.api       = &thread_api;

    acoral_pool_ctrl_init(&thread_pool_ctrl);
}

/**
 * @brief 线程调度机制初始化
 *
 */
static void acoral_sched_mechanism_init(void)
{
    acoral_thread_pool_init();
    acoral_sched_rdyqueue_init();
    acoral_fifo_queue_init(&acoral_threads_queue);
}

/**
 * @brief 线程管理系统初始化
 *
 */
void acoral_thread_sys_init(void)
{
    acoral_sched_mechanism_init();
    acoral_sched_policy_init();
}
