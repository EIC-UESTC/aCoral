/**
 * @file fcntl.h
 * @author 李杰
 * @brief 文件控制选项和标志的定义头文件。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 此头文件定义了与文件控制相关的宏和标志，这些标志主要用于 POSIX 文件操作函数，
 * 如 open(), openat(), 和 fcntl()。
 *
 * 当前版本仅实现宏定义，尚无对应的函数实现。其主要目的是为了保证 POSIX 兼容层的接口定义完整性，从而支持未来逐步接入的文件系统实现。
 * (POSIX 接口函数（如 open, openat, fcntl）都依赖 fcntl.h中定义的宏值作为其 oflag参数或函数返回值的一部分。)
 *
 * @par 参考
 * 主要参考 POSIX.1-2008 标准中的 fcntl.h定义，在线文档：
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/fcntl.h.html
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建，包含 POSIX 文件创建、状态和访问模式标志。
 * </table>
 */

#ifndef _ACORAL_POSIX_FCNTL_H_
#define _ACORAL_POSIX_FCNTL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name 用于 open() 和 openat() 中 oflag 参数的文件创建标志
 * @brief 这些标志控制文件在被打开或创建时的行为。它们可以按位或 (|) 操作组合使用。
 */
#define O_CLOEXEC      0x0001 /**< 执行 exec() 时关闭该文件描述符  */
#define O_CREAT        0x0002 /**< 如果文件不存在，则创建文件  */
#define O_DIRECTORY    0x0004 /**< 如果文件不是目录，则打开失败  */
#define O_EXCL         0x0008 /**< 独占使用标志，与 O_CREAT 配合使用时，如果文件已存在则失败  */
#define O_NOCTTY       0x0010 /**< 不将打开的文件分配为控制终端  */
#define O_NOFOLLOW     0x0020 /**< 不跟随符号链接 */
#define O_TRUNC        0x0040 /**< 如果文件已存在，则截断为长度为 0 */
#define O_TTY_INIT     0x0080 /**< termios 结构提供符合标准的终端行为初始化 */


/**
 * @name open()、openat() 和 fcntl() 的文件状态标志
 * @brief 这些标志控制文件被打开后的行为或特性。
 */
#define O_APPEND      0x0100 /**< 设置为追加模式 */
#define O_DSYNC       0x0200 /**< 写入时执行数据完整性的同步 I/O */
#define O_NONBLOCK    0x0400 /**< 设置为非阻塞模式 */
#define O_RSYNC       0x0800 /**< 读取操作按照同步 I/O 完成  */
#define O_SYNC        0x0200 /**< 写入时执行文件完整性的同步 I/O（注意和 O_DSYNC 共享值） */


/**
 * @name 文件访问模式的掩码
 * @brief 用于从 oflag参数中提取文件访问权限部分的掩码。
 * @details 将 oflag与此掩码进行位与 (&) 操作，可以得到文件的访问模式（如只读、只写、读写）。
 */
#define O_ACCMODE    0xF000 /**< 用于提取访问模式（只读、只写、读写等）的掩码。 */


/**
 * @name open()、openat() 和 fcntl() 的文件访问模式
 * @brief 定义文件打开时的读/写权限。这些标志是互斥的，通常只能选择其中一个。
 */
#define O_EXEC      0x1000  /**< 仅用于执行（非目录文件） */
#define O_RDONLY    0x2000  /**< 只读打开 */
#define O_RDWR      0xA000  /**< 读写打开 */
#define O_SEARCH    0x4000  /**< 仅用于搜索目录  */
#define O_WRONLY    0x8000  /**< 只写打开 */


#ifdef __cplusplus
}
#endif

#endif /* ifndef _ACORAL_POSIX_FCNTL_H_ */
