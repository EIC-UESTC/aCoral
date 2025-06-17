/**
 * @file timer.c
 * @author 胡博文 (@921576434@qq.com)
 * @brief kernel层时钟相关源文件
 * @version 2.0
 * @date 2025-04-14
 *
 * @copyright Copyright (c) 2022 EIC-UESTC
 *
 * @par 修订历史
 *     <table>
 *         <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 *         <tr><td>v1.0 <td>胡博文 <td>2022-07-12 <td>增加注释
 *         <tr><td>v2.0 <td>饶洪江 <td>2025-04-14 <td>规范代码风格
 *
 */
#include "hal.h"
#include "queue.h"
#include "cpu.h"
#include "int.h"
#include "policy.h"
#include "lsched.h"
#include "timer.h"
#include "print.h"
#include "xscutimer.h"

/* 时钟控制结构体 */
static XScuTimer      acoral_timer;
/* 延时队列 */
static acoral_queue_t time_delay_queue;
/* 基石时刻 */
static acoral_u32 ticks;


/**
 * @brief 时钟管理系统初始化
 *
 */
void acoral_time_sys_init(void)
{
    /* 延时队列初始化 */
    acoral_tick_queue_init(&time_delay_queue);
}

/**
 * @brief 获得当前时刻
 *
 * @return acoral_time 时刻
 */
acoral_time acoral_get_ticks()
{
    return ticks;
}


/**
 * @brief 设置时刻
 *
 * @param time 要设置的时刻
 */
void acoral_set_ticks(acoral_time time)
{
    ticks = time;
}



/**
 * @brief 基石时钟中断函数
 *
 * @param CallBackRef 回调参数
 */
void acoral_ticks_handler(void *CallBackRef)
{
#ifdef CFG_OS_TICK_PRINT_ENABLE
    static acoral_time time_tmp = 0;
    static acoral_u32 seconds   = 0;
#endif
    XScuTimer *TimerInstancePtr = (XScuTimer *)CallBackRef;
    if (XScuTimer_IsExpired(TimerInstancePtr))
    {
#ifdef CFG_HOOK_TICKS
        acoral_ticks_hook();
#endif

        ticks++;

#ifdef CFG_OS_TICK_PRINT_ENABLE
        if ((ticks - time_tmp) >= 1000)
        {
            seconds++;
            time_tmp = ticks;
            acoral_print("\r\n----------OS Time: %ds----------\r\n", seconds);
        }
#endif
        if (acoral_sched_is_start() == true)
        {
            time_delay_deal();         /* 延时链表处理 */
            acoral_policy_time_deal(); /* 调度处理 */
        }
        /* 清楚定时器中断标志位 */
        XScuTimer_ClearInterruptStatus(TimerInstancePtr);
    }
}
/**
 * @brief 基石时钟初始化
 *
 */
void acoral_ticks_init()
{
    ticks = 0; /* 时刻初始化 */
    XScuTimer_Config *timerConfig;
    timerConfig = XScuTimer_LookupConfig(XPAR_SCUTIMER_DEVICE_ID);              /* 获取时钟配置 */
    XScuTimer_CfgInitialize(&acoral_timer, timerConfig, timerConfig->BaseAddr); /* 配置时钟 */

    /* 绑定中断服务函数 */
    XScuGic_Connect(&int_ctrl[0],
                    XPAR_SCUTIMER_INTR,
                    (Xil_ExceptionHandler)acoral_ticks_handler,
                    (void *)&acoral_timer
                   );
    XScuGic_Enable(&int_ctrl[0], XPAR_SCUTIMER_INTR); /* GIC使能时钟中断 */

    XScuTimer_EnableAutoReload(&acoral_timer);                                      /* 使能自动重装载 */
    XScuTimer_SetPrescaler(&acoral_timer, 0);                                       /* 设置时钟分频 */
    XScuTimer_LoadTimer(&acoral_timer, XPAR_CPU_CORTEXA9_0_CPU_CLK_FREQ_HZ / 2000); /* 设置自动装载值 */

    XScuTimer_EnableInterrupt(&acoral_timer); /* 时钟使能中断 */
    XScuTimer_Start(&acoral_timer);           /* 启动时钟 */
    acoral_intr_enable();                     /* 开中断 */
}

/**
 * @brief 将线程挂到延时队列上
 *
 * @param new 要添加的线程
 */
void acoral_time_delay_queue_add(void *new)
{
    acoral_thread_t *thread = (acoral_thread_t *)new;
    acoral_enter_critical();
    acoral_tick_queue_add(&time_delay_queue, &thread->delaying);
    acoral_suspend_thread(thread);
    acoral_exit_critical();
    return;
}

/**
 * @brief 将线程移出延时队列
 *
 * @param old 要移出的线程
 */

void acoral_time_delay_queue_del(void *old)
{
    acoral_thread_t *thread = (acoral_thread_t *)old;
    acoral_enter_critical();
    acoral_tick_queue_del(&time_delay_queue, &thread->delaying);
    acoral_rdy_thread(thread);
    acoral_exit_critical();
    return;
}

/**
 * @brief 延时处理函数
 *
 */
void time_delay_deal()
{
    acoral_list_t *tmp, *tmp1, *head;
    acoral_thread_t *thread;

    head = &time_delay_queue.head;
    if (acoral_list_empty(head))
    {
        return;
    }
    if (head->next->value > 0)
    {
        head->next->value--;
    }

    for (tmp = head->next; tmp != head;)
    {
        thread = list_entry(tmp, acoral_thread_t, delaying);
        if (tmp->value > 0)
        {
            break;
        }
        tmp1 = tmp->next;
        acoral_tick_queue_del(&time_delay_queue, &thread->delaying);
        tmp  = tmp1;
        acoral_rdy_thread(thread);
    }
}
