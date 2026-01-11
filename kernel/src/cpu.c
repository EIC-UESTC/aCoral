/**
 * @file cpu.c
 * @author 胡博文 (@921576434@qq.com)
 * @brief kernel层cpu源文件
 * @version 1.0
 * @date 2025-03-28
 * 
 * @copyright Copyright (c) 2022 EIC-UESTC
 * 
 * @par 修订历史
 *     <table>
 *         <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 *         <tr><td>v1.0 <td>胡博文 <td>2022-07-11 <td>增加注释
 *         <tr><td>v2.0 <td>饶洪江 <td>2025-03-28 <td>规范代码风格
 */
#include "cpu.h"
#include "bitops.h"


/* 活动cpu位图 */
acoral_u32 acoral_active_map[1] = {0};

//为每一个 CPU 核心都分配了一个独立的、完整的就绪队列系统((Per-CPU Ready Queues))
//extern acoral_rdy_queue_t acoral_ready_queues[CFG_MAX_CPU];

/**
 * @brief 查看某cpu是否在活动状态
 * 
 * @param cpu 要查看的cpu
 * @return acoral_u32 返回状态，1 活动，0 不活动
 */
acoral_u32 acoral_cpu_is_active(acoral_u32 cpu)
{
    return acoral_get_bit(cpu, acoral_active_map);
}

/**
 * @brief 设置cpu为活动状态
 * 
 * @param cpu 要设置的cpu
 */
void acoral_cpu_set_active(acoral_u32 cpu)
{
	acoral_set_bit(cpu, acoral_active_map);
}

/**
 * @brief 获取最空闲的cpu
 * 
 * @return acoral_u32 最空闲的cpu
 */
acoral_u32 acoral_get_idlest_cpu(void)
{
    acoral_u32 cpu = 0;
#if 0
    acoral_u32 i, count = 0xffffffff;
    acoral_rdy_queue_t *rdy_queue;

    for (i = 0; i < CFG_MAX_CPU; i++)
    {
        rdy_queue = acoral_ready_queues + i;
        if (count > rdy_queue->array.num)
        {
            count = rdy_queue->array.num;
            cpu = i;
        }
    }
#endif
    return cpu;
}

/**
 * @brief 获取最空闲的cpu（屏蔽）
 * 
 * @param cpu_mask 屏蔽cpu的掩码
 * @return acoral_u32 最空闲的cpu（屏蔽）
 */
acoral_u32 acoral_get_idle_maskcpu(acoral_u32 cpu_mask)
{
    acoral_u32 cpu = 0;
#if 0
    acoral_u32 i, count = 0xffffffff;
    acoral_rdy_queue_t *rdy_queue;

    for(i = 0, cpu = 0;i < CFG_MAX_CPU; i++)
    {
        rdy_queue = acoral_ready_queues + i;
        if ((count > rdy_queue->array.num) && ((1 << i) & cpu_mask))
        {
            count = rdy_queue->array.num;
            cpu = i;
        }
    }
#endif
    return cpu;
}
