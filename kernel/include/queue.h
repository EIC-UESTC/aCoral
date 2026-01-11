/**
 * @file queue.h
 * @author 胡博文 (@921576434@qq.com)
 * @brief kernel层队列相关头文件
 * @version 2.0
 * @date 2025-04-10
 * 
 * @copyright Copyright (c) 2022 EIC-UESTC
 * 
 * @par 修订历史
 *     <table>
 *         <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 *         <tr><td>v1.0 <td>胡博文 <td>2022-07-02 <td>增加注释
 *         <tr><td>v2.0 <td>饶洪江 <td>2025-04-10 <td>规范代码风格
 */
#ifndef KERNEL_QUEUE_H
#define KERNEL_QUEUE_H
#include "list.h"

/**
 * @brief 队列结构体
 * 1.acoral_list_t head(链表头节点)：
 * 使用了一种侵入式链表（intrusive linked list）来实现队列。
 * 侵入式链表意味着线程控制块 (TCB) 自身内部会包含一个 acoral_list_t 类型的成员（在 sched_yield 函数中我们看到了 current_thread->ready，这个 ready 就是 TCB 内的链表节点）。
 * 当一个线程变为就绪状态时，它 TCB 内的那个 acoral_list_t 节点就会被链接到这个 head 所代表的链表中。
 * 这个 head 充当了该优先级就绪线程队列的入口。它是一个双向循环链表头，head.next 指向队列中的第一个线程，head.prev 指向队列中的最后一个线程。如果队列为空，head.next 和 head.prev 都指向 head 自身。
 * 2.acoral_spinlock_t lock(自旋锁):
 * 确保在任何时刻只有一个 CPU 核心可以修改同一个优先级的就绪队列，防止因并发访问导致的数据竞争和队列损坏。
 */
typedef struct{
    acoral_list_t     head; /* 队列头 */
    acoral_spinlock_t lock; /* 自旋锁 */
    void             *data; /* 数据 */
} acoral_queue_t;

void acoral_tick_queue_init(acoral_queue_t *queue);
void acoral_tick_queue_add(acoral_queue_t *queue, acoral_list_t *tnode);
void acoral_tick_queue_del(acoral_queue_t *queue, acoral_list_t *tnode);

void acoral_prio_queue_init(acoral_queue_t *queue);
void acoral_prio_queue_add(acoral_queue_t *queue, acoral_list_t *pnode);
void acoral_prio_queue_del(acoral_queue_t *queue, acoral_list_t *pnode);

void acoral_fifo_queue_init(acoral_queue_t *queue);
void acoral_fifo_queue_v_init(acoral_queue_t *queue, acoral_32 value);
void acoral_fifo_queue_add(acoral_queue_t *queue, acoral_list_t *node);
void acoral_fifo_queue_del(acoral_queue_t *queue, acoral_list_t *node);

void acoral_lifo_queue_init(acoral_queue_t *queue);
void acoral_lifo_queue_v_init(acoral_queue_t *queue, acoral_32 value);
void acoral_lifo_queue_add(acoral_queue_t *queue, acoral_list_t *node);
void acoral_lifo_queue_del(acoral_queue_t *queue, acoral_list_t *node);
#endif /* QUEUE_H_ */
