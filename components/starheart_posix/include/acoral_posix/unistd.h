/**
 * @file unistd.h
 * @author 李杰
 * @brief POSIX 兼容层的标准符号常量和类型定义头文件。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 此头文件包含了 POSIX 标准中定义的一些基本常量、类型和函数声明，这些是实现操作系统抽象和提供系统调用接口的基础。
 *
 * @par 参考
 * 主要参考 POSIX.1-2008 标准中的 unistd.h 定义，在线文档：
 * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建。
 * </table>
 */

#ifndef _ACORAL_POSIX_UNISTD_H_
#define _ACORAL_POSIX_UNISTD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "acoral_posix/acoral_sys/types.h"


unsigned int sleep( unsigned seconds );
int usleep( useconds_t usec );


#ifdef __cplusplus
}
#endif

#endif /* ifndef _ACORAL_POSIX_UNISTD_H_ */
