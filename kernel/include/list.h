/**
 * @file list.h
 * @author 胡博文 (@921576434@qq.com)
 * @brief kernel层链表相关头文件
 * @version 2.0
 * @date 2025-04-01
 * 
 * @copyright Copyright (c) 2022 EIC-UESTC
 * 
 * @par 修订历史
 *     <table>
 *         <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 *         <tr><td>v1.0 <td>胡博文 <td>2022-07-02 <td>增加注释
 *         <tr><td>v2.0 <td>饶洪江 <td>2025-04-01 <td>规范代码风格
 */

#ifndef KERNEL_LIST_H
#define KERNEL_LIST_H
#include "spinlock.h"

/* 寻找包含某链表节点的结构体首地址 */
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/**
 * @brief 链表节点结构体
 * 
 */
typedef struct acoral_list
{
    struct acoral_list *next; /* 下一个链表节点指针 */
    struct acoral_list *prev; /* 上一个链表节点指针 */
    acoral_32 value;          /* 值 */
#ifdef CFG_SMP
    acoral_spinlock_t lock;   /* 自旋锁 */
#endif
} acoral_list_t;

/* 检测链表是否为空 */
#define acoral_list_empty(head) ((head)->next==(head))

/**
 * @brief 遍历双向循环链表 (不包括头节点本身)。
 * @param pos 用作循环迭代器的 acoral_list_t 指针。
 * @param head 指向链表头节点的 acoral_list_t 指针。
 */
#define acoral_list_for_each(pos, head)	\
	for(pos = (head)->next;				\
	pos != (head);						\
	pos = pos->next )

/**
 * @brief 遍历包含特定 acoral_list_t 成员的结构体链表 (删除安全)。
 * @param pos 循环变量，指向当前父结构体的指针。
 * @param n 临时变量，指向下一个父结构体的指针 (用于安全删除)。
 * @param type 父结构体的类型。
 * @param head 链表头指针 (acoral_list_t *)。
 * @param member 父结构体中 acoral_list_t 成员的名称。
 */
#define acoral_list_for_each_entry_safe(pos, n, type, head, member)    	\
    for (pos = list_entry((head)->next, type, member),         			\
         n = list_entry(pos->member.next, type, member);       			\
         &pos->member != (head);                                      	\
         pos = n, n = list_entry(n->member.next, type, member))

/**
 * @brief 遍历包含特定 acoral_list_t 成员的结构体链表。
 * 这个版本是非删除安全的，即在循环体内删除 'pos' 指向的节点是不安全的。
 * @param pos 循环变量，指向父结构体的指针。
 * 例如：posix_mq_desc_t *entry;
 * @param type 父结构体的类型。
 * 例如：posix_mq_desc_t
 * @param head 链表头指针 (acoral_list_t *)。
 * 例如：&g_posix_mq_active_desc_list_head
 * @param member 父结构体中 acoral_list_t 成员的名称。
 * 例如：desc_list_node
 */
#define acoral_list_for_each_entry(pos, type, head, member)            	\
    for (pos = list_entry((head)->next, type, member);          		\
         &pos->member != (head);                                       	\
         pos = list_entry(pos->member.next, type, member))



void acoral_list_add(acoral_list_t *new, acoral_list_t *head);
void acoral_list_add_tail(acoral_list_t *new, acoral_list_t *head);
void acoral_list_del(acoral_list_t *entry);
void acoral_list_init(acoral_list_t *ptr);
void acoral_vlist_init(acoral_list_t *ptr, acoral_32 value);
#endif
