/**
 * @file signal.h
 * @author 李杰
 * @brief POSIX 兼容层的信号定义头文件。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 此头文件定义了与 POSIX 信号机制相关的必要数据结构，例如 union sigval和 struct sigevent。
 * 当前尚未完全实现完整的 POSIX 信号机制，这些结构体的定义为未来扩展和兼容性打下了基础，并确保其他模块在使用信号相关类型时能保持接口一致性。
 * 本文件旨在提供信号数据结构的骨架，而非实现信号处理逻辑。
 *
 * @par 参考
 * 主要参考 POSIX.1-2008 标准中的 signal.h定义，在线文档：
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/signal.h.html
 *
 * @par 注意
 * 当前实现中，SIGEV_SIGNAL通知类型尚不支持。
 * 在 struct sigevent中，sigev_signo成员会被忽略，且 sigev_value联合体仅使用 sival_ptr成员。
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建，定义 union sigval 和 struct sigevent等信号相关数据结构。
 * </table>
 */


#ifndef _ACORAL_POSIX_SIGNAL_H_
#define _ACORAL_POSIX_SIGNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name sigev_notify 的取值
 * @brief 定义 sigevent 结构体中 sigev_notify成员的可能值，用于指定异步通知的类型。
 */
#define SIGEV_NONE      0 /**< 当通知/触发等事件发生时，不发送任何异步通知。 */
#define SIGEV_SIGNAL    1 /**< 当通知/触发等事件发生时，产生一个带有应用定义值的排队信号。不支持该方式。 */
#define SIGEV_THREAD    2 /**< 当通知/触发等事件发生时，调用一个通知函数进行通知。 */


/**
 * @brief 信号值联合体。
 * union sigval用于在信号通知中传递一个值，可以是整数类型或指针类型。
 */
union sigval
{
    int sival_int;    /**< 整型信号值。 */
    void * sival_ptr; /**< 指针类型信号值。 */
};

/**
 * @brief 信号事件结构体。
 * struct sigevent定义了如何异步通知进程或线程一个事件的发生。
 * 它包含了通知类型、信号编号（在当前实现中被忽略）、信号值以及通知函数的指针。
 */
struct sigevent
{
    int sigev_notify;                                 /**< 通知类型。SIGEV_SIGNAL 值当前不支持 */
    int sigev_signo;                                  /**< 信号编号。该成员被忽略 */
    union sigval sigev_value;                         /**< 信号值。仅使用 sival_ptr 成员 */
    void ( * sigev_notify_function )( union sigval ); /**< 通知函数指针 */
//    pthread_attr_t * sigev_notify_attributes;         /**< 通知属性 */
};

#ifdef __cplusplus
}
#endif

#endif /* ifndef _ACORAL_POSIX_SIGNAL_H_ */
