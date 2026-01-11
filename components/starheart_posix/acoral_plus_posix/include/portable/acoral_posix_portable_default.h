/**
 * @file acoral_posix_portable_default.h
 * @author 李杰
 * @brief POSIX兼容层的平台特定默认配置
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 此头文件定义了POSIX兼容层在不同平台上的默认配置宏。
 *
 * @par 注意
 * 所有配置宏都使用 #ifndef ... #define ... #endif结构，
 * 这意味着如果这些宏已经在其他地方被定义，则此文件中的定义将不会生效，从而允许在更高层级的配置文件中进行覆盖。
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建，包含各类 POSIX 默认配置。
 * </table>
 */

#ifndef _ACORAL_POSIX_PORTABLE_DEFAULT_H_
#define _ACORAL_POSIX_PORTABLE_DEFAULT_H_

/**
 * @name 任务命名配置
 * @brief 定义 POSIX 线程（pthread）相关任务的默认名称。
 */
#ifndef posixconfigPTHREAD_TASK_NAME
    #define posixconfigPTHREAD_TASK_NAME    "pthread"
#endif


/**
 * @name 定时器名称配置
 * @brief 定义 POSIX 定时器（timer_t）相关任务的默认名称。
 */
#ifndef posixconfigTIMER_NAME
    #define posixconfigTIMER_NAME    "timer"
#endif


/**
 * @name 消息队列默认配置
 * @brief 定义 POSIX 消息队列（mqueue）的默认行为参数。
 */
#ifndef posixconfigMQ_MAX_MESSAGES
    #define posixconfigMQ_MAX_MESSAGES    10 /**< 消息队列中最大消息数量 */
#endif

#ifndef posixconfigMQ_MAX_SIZE
    #define posixconfigMQ_MAX_SIZE    128 /**< 每条消息的最大大小(字节) */
#endif


/**
 * @name 条件变量默认配置
 * @brief 定义 POSIX 条件变量（pthread_cond_t）的默认行为参数。
 */
#ifndef posixconfigPTHREAD_COND_MAX_WAITERS
    #define posixconfigPTHREAD_COND_MAX_WAITERS 4 /**< 每个条件变量上最多等待的任务数 */
#endif


/**
 * @name POSIX 实现相关常量
 * @brief 定义了用于 POSIX 兼容性的一些重要常量。
 *
 * 这些定义通常在标准 C 库的 <limits.h>或相关头文件中找到。
 * 在嵌入式环境中，在此处手动定义以确保可移植性。
 */
#ifndef NAME_MAX
	/**
     * @brief 文件名或目录名的最大字节数（不包含终止空字符）。
     * @details 根据 POSIX 标准定义，用于确保文件或目录名称长度的兼容性。
     */
    #define NAME_MAX             64                                               /**< 文件名的最大字节数(不包含终止空字符) */
#endif

#ifndef SEM_VALUE_MAX
	/**
     * @brief 信号量 sem_t的最大值。
     * @details 定义了 POSIX 命名信号量或无名信号量可以达到的最大值。
     * 通常设置为 0x7FFFU，即 32767。
     */
    #define SEM_VALUE_MAX        0x7FFFU                                          /**< 信号量sem_t 的最大值 */
#endif


/**
 * @name 启用 POSIX 类型的 typedef 定义
 * @brief 控制是否在此头文件中定义特定的 POSIX 数据类型。
 *
 * 将相应的宏设为 1 启用 typedef，设为 0 则禁用。
 * 建议仅在与系统（例如，Toolchain 的标准库）已有的类型定义冲突时才考虑禁用。
 * 禁用意味着这些类型将依赖于其他头文件或编译环境的定义。
 */
#ifndef posixconfigENABLE_CLOCK_T
    #define posixconfigENABLE_CLOCK_T                1 /**< clock_t 类型（定义于 sys/types.h） */
#endif
#ifndef posixconfigENABLE_CLOCKID_T
    #define posixconfigENABLE_CLOCKID_T              0 /**< clockid_t 类型 */
#endif
#ifndef posixconfigENABLE_MODE_T
    #define posixconfigENABLE_MODE_T                 0 /**< mode_t 类型 */
#endif
#ifndef posixconfigENABLE_PID_T
    #define posixconfigENABLE_PID_T                  1 /**< pid_t 类型 */
#endif
#ifndef posixconfigENABLE_PTHREAD_ATTR_T
    #define posixconfigENABLE_PTHREAD_ATTR_T         1 /**< pthread_attr_t 类型 */
#endif
#ifndef posixconfigENABLE_PTHREAD_COND_T
    #define posixconfigENABLE_PTHREAD_COND_T         1 /**< pthread_cond_t 类型 */
#endif
#ifndef posixconfigENABLE_PTHREAD_CONDATTR_T
    #define posixconfigENABLE_PTHREAD_CONDATTR_T     1 /**< pthread_condattr_t 类型 */
#endif
#ifndef posixconfigENABLE_PTHREAD_MUTEX_T
    #define posixconfigENABLE_PTHREAD_MUTEX_T        1 /**< pthread_mutex_t 类型 */
#endif
#ifndef posixconfigENABLE_PTHREAD_MUTEXATTR_T
    #define posixconfigENABLE_PTHREAD_MUTEXATTR_T    1 /**< pthread_mutexattr_t 类型 */
#endif
#ifndef posixconfigENABLE_PTHREAD_T
    #define posixconfigENABLE_PTHREAD_T              1 /**< pthread_t 类型 */
#endif
#ifndef posixconfigENABLE_SSIZE_T
    #define posixconfigENABLE_SSIZE_T                1 /**< ssize_t 类型 */
#endif
#ifndef posixconfigENABLE_TIME_T
    #define posixconfigENABLE_TIME_T                 0 /**< time_t 类型 */
#endif
#ifndef posixconfigENABLE_TIMER_T
    #define posixconfigENABLE_TIMER_T                0 /**< timer_t 类型 */
#endif
#ifndef posixconfigENABLE_USECONDS_T
    #define posixconfigENABLE_USECONDS_T             1 /**< useconds_t 类型 */
#endif
#ifndef posixconfigENABLE_PTHREAD_BARRIER_T
    #define posixconfigENABLE_PTHREAD_BARRIER_T      1 /**< pthread_barrier_t 类型 */
#endif
#ifndef posixconfigENABLE_TIMESPEC
    #define posixconfigENABLE_TIMESPEC               0 /**< struct timespec 结构体(定义于time.h) */
#endif
#ifndef posixconfigENABLE_ITIMERSPEC
    #define posixconfigENABLE_ITIMERSPEC             0 /**< struct itimerspec 结构体(定义于time.h) */
#endif
#ifndef posixconfigENABLE_SEM_T
    #define posixconfigENABLE_SEM_T                  1 /**< struct sem_t 结构体(定义于semaphore.h) */
#endif
#ifndef posixconfigENABLE_SCHED_PARAM
    #define posixconfigENABLE_SCHED_PARAM            1 /**< sched_param 类型（定义于 sched.h） */
#endif




#endif /* ifndef _ACORAL_POSIX_PORTABLE_DEFAULT_H_ */
