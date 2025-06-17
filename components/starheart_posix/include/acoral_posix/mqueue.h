/**
 * @file mqueue.h
 * @author 李杰
 * @brief POSIX 消息队列（Message Queues）接口定义。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 此头文件定义了 POSIX 标准中消息队列相关的类型、结构体和函数原型。
 * 消息队列是一种用于进程间/线程间通信（IPC）的机制，允许不同执行实体安全地发送和接收数据消息。
 *
 * 它提供了与 mq_open(), mq_send(), mq_receive()等 POSIX 标准函数兼容的接口，旨在为应用程序提供标准的实时消息队列功能。
 *
 * @par 参考
 * 主要参考 POSIX.1-2008 标准中的 mqueue.h定义：
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/mqueue.h.html
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建，定义 POSIX 消息队列接口。
 * </table>
 */

#ifndef _ACORAL_POSIX_MQUEUE_H_
#define _ACORAL_POSIX_MQUEUE_H_

// 内核层头文件
#include "kernel.h"

// posix兼容层头文件
#include "acoral_posix/time.h"
#include "acoral_posix/fcntl.h"
#include "acoral_posix/signal.h"
#include "acoral_posix/acoral_sys/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 消息队列描述符类型。
 * @details mqd_t被定义为一个 void *(空类型指针)。
 * 它用作一个不透明的句柄或标识符，代表一个已打开的消息队列实例。
 * 应用程序通过这个描述符来引用特定的消息队列进行操作（如发送、接收消息）。
 */
typedef void * mqd_t;

/**
 * @brief 用于表示阻塞操作中“永久等待”的宏。
 * @details 在消息队列发送或接收操作中，如果指定此超时，表示任务将无限期阻塞，直到操作完成或被外部事件（如信号）中断。
 */
#define ACORAL_WAIT_FOREVER ((acoral_32)-1);

/**
 * @brief 单元测试辅助宏。
 * @details 仅当此宏被定义时，相关的测试辅助函数（如 get_destroy_call_count()）才会被声明和编译，有助于将测试专用代码与生产代码分离。
 */
#define UNIT_TESTING


/**
 * @brief 消息队列属性结构体。
 */
struct mq_attr
{
    long mq_flags;   /**< 消息队列文件标志 */
    long mq_maxmsg;  /**< 消息队列可容纳的最大消息个数 */
    long mq_msgsize; /**< 消息队列的单则消息的最大长度（字节为单位） */
    long mq_curmsgs; /**< 当前消息队列中的消息数量(由 mq_getattr 填充) */
};

// 仅在 UNIT_TESTING 被定义时声明这些测试辅助函数
#ifdef UNIT_TESTING

// 获取 posix_mq_object_destroy 的调用次数
extern int get_destroy_call_count();
// 重置 posix_mq_object_destroy 的调用计数
extern void reset_destroy_call_count();

#endif


mqd_t mq_open(const char *name, int oflags, mode_t mode, struct mq_attr *attr);
int mq_close(mqd_t mqdes);
int mq_unlink(const char *name);
int mq_getattr(mqd_t mqdes, struct mq_attr *mqstat);
int mq_setattr(mqd_t mqdes, const struct mq_attr *mqstat,
	       struct mq_attr *omqstat);

int mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
		   unsigned int *msg_prio);
int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
	    unsigned int msg_prio);
int mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len,
			unsigned int *msg_prio, const struct timespec *abstime);
int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
		 unsigned int msg_prio, const struct timespec *abstime);
int mq_notify(mqd_t mqdes, const struct sigevent *notification);



#ifdef __cplusplus
}
#endif


#endif /* ifndef _ACORAL_POSIX_MQUEUE_H_ */
