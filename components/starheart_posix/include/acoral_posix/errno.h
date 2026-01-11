/**
 * @file errno.h
 * @author 李杰
 * @brief POSIX 兼容层的系统错误码定义头文件。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 此头文件定义了一组符合 POSIX 标准的系统错误码（errno）。
 * 这些宏定义了各种系统调用和库函数可能返回的错误情况，帮助应用程序判断和处理错误。
 * 本文件旨在为 acoral提供一个与 POSIX 兼容的错误报告机制。
 *
 * @par 注意
 * 本文件中定义的错误码数值可能与宿主系统（或工具链）自带的标准库 strerror函数不兼容。
 * 如果需要在应用程序中使用 strerror获取错误字符串，请确保其与此处定义的错误码值匹配。
 * 为了避免重定义冲突，本文件在定义宏之前会先 undef掉常见的错误码宏。
 *
 * @par 参考
 * 主要参考 POSIX.1-2008 标准中的 errno.h定义，在线文档：
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/errno.h.html
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建，包含 POSIX 错误码定义及 errno 映射机制。
 * </table>
 */

#ifndef _ACORAL_POSIX_ERRNO_H_
#define _ACORAL_POSIX_ERRNO_H_


#ifdef __cplusplus
extern "C" {
#endif

/*
 * @brief 取消已有的错误码宏定义。
 * @details 这些 #undef指令用于在定义新的错误码宏之前，
 * 清除可能由其他系统头文件引入的同名宏，以防止编译时的重定义错误。
 */
#undef EPERM
#undef ENOENT
#undef ESRCH
#undef EINTR
#undef EIO
#undef EBADF
#undef EAGAIN
#undef ENOMEM
#undef EFAULT
#undef EBUSY
#undef EEXIST
#undef EINVAL
#undef ENOSPC
#undef ERANGE
#undef ENAMETOOLONG
#undef EDEADLK
#undef EOVERFLOW
#undef ENOSYS
#undef EMSGSIZE
#undef ENOTSUP
#undef ETIMEDOUT

/**
 * @name POSIX 标准错误码定义
 * @brief 这些宏定义了符合 POSIX 标准的错误码及其对应的数值。
 */
#define EPERM           1   /**< 不允许的操作（无权限） */
#define ENOENT          2   /**< 文件或目录不存在  */
#define ESRCH			3	/**< 没有那个进程或线程  */
#define EINTR 			4 	/**< 系统调用被中断 */
#define EIO 			5   /**< I/O error */
#define EBADF           9   /**< 错误的文件描述符  */
#define EAGAIN          11  /**< 资源暂不可用，请重试 */
#define ENOMEM          12  /**< 内存不足  */
#define EFAULT 			14  /**< Bad address */
#define EBUSY           16  /**< 设备或资源忙  */
#define EEXIST          17  /**< 文件已存在  */
#define EINVAL          22  /**< 参数无效  */
#define ENOSPC          28  /**< 设备上无可用空间  */
#define ERANGE          34  /**< 结果数值超出范围 */
#define ENAMETOOLONG    36  /**< 文件名太长 */
#define EDEADLK         45  /**< 资源死锁 */
#define EOVERFLOW       75  /**< 值太大，无法存储 */
#define ENOSYS          88  /**< 不支持的函数 */
#define EMSGSIZE        90  /**< 消息太长 */
#define ENOTSUP         95  /**< 操作不被支持 */
#define ETIMEDOUT       116 /**< 连接超时 */

/**
 * @name 系统变量支持
 * @brief 配置 errno 线程局部变量的映射机制。
 *
 * @details 标准 errno宏被直接映射到内部的 acoral_errno变量。
 * (定义在posix_unisted.c)
 */
extern __thread int acoral_errno;   		/**< 声明一个线程局部错误码变量，用于存储当前线程的最新错误码。*/
#define errno acoral_errno  	/**< 将标准的 errno宏映射到 acoral_errno变量，使得应用程序可以直接使用 errno来获取或设置当前线程的错误码。*/



#ifdef __cplusplus
}
#endif

#endif /* ifndef _ACORAL_POSIX_ERRNO_H_ */
