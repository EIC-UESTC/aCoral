/**
 * @file ringbuffer.h
 * @author 文佳源 (648137125@qq.com)
 * @brief 环形缓冲区头文件
 * @version 0.1
 * @date 2023-06-17
 * 
 * Copyright (c) 2023
 * 
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>文佳源 <td>2023-06-17 <td>新建文件
 * </table>
 */
#ifndef CAT_RINGBUFFER_H
#define CAT_RINGBUFFER_H

#include "acoral.h"

#define MIN_RINGBUFFER_SIZE     4

typedef struct _cat_ringbuffer_t cat_ringbuffer_t;
struct _cat_ringbuffer_t
{
    acoral_u8      *p_buffer;     /* 放置数据的缓冲区地址 */
    acoral_u32      ring_mask;    /* 用于循环头尾索引的掩码，用于代替取余操作 */
    acoral_u32      tail_index;   /* 尾部索引 */
    acoral_u32      head_index;   /* 头部索引 */
};

/**
 * @brief 缓冲区初始化
 * 
 * @param  p_ringbuffer     环形缓冲区结构体指针
 * @param  p_buffer_space   缓冲区内存空间首地址
 * @param  buffer_size      缓冲区大小(字节)
 */
void cat_ringbuffer_init(cat_ringbuffer_t *p_ringbuffer, acoral_u8 *p_buffer_space, acoral_u32 buffer_size);

/**
 * @brief 清空缓冲区里所有数据
 * 
 * @param  p_ringbuffer     环形缓冲区结构体指针
 */
void cat_ringbuffer_clear(cat_ringbuffer_t *p_ringbuffer);

/**
 * @brief 从缓冲区取元素
 * 
 * @param  p_ringbuffer     环形缓冲区结构体指针
 * @param  data             要存入的数据值
 * @return acoral_32 
 */
acoral_32 cat_ringbuffer_put(cat_ringbuffer_t *p_ringbuffer, acoral_u8 data);

/**
 * @brief 
 * 
 * @param  p_ringbuffer     环形缓冲区结构体指针
 * @param  p_data           取出的数据存放的数据地址
 * @return acoral_32      成功失败
 */
acoral_32 cat_ringbuffer_get(cat_ringbuffer_t *p_ringbuffer, acoral_u8 *p_data);

/**
 * @brief 
 * 
 * @param  p_ringbuffer     环形缓冲区结构体指针
 * @param  p_data           要存入的数据的起始地址
 * @param  size             要存入的数据的长度
 * @return acoral_32      成功失败
 */
acoral_32 cat_ringbuffer_put_more(cat_ringbuffer_t *p_ringbuffer, const acoral_u8 *p_data, acoral_u32 size);

/**
 * @brief 
 * 
 * @param  p_ringbuffer     环形缓冲区结构体指针
 * @param  p_data           取出数据要存放的起始地址
 * @param  size             要取出数据的长度
 * @return acoral_u32     从环形缓冲区实际取出的数据个数(目前每个数据单位为字节)
 */
acoral_u32 cat_ringbuffer_get_more(cat_ringbuffer_t *p_ringbuffer, acoral_u8 *p_data, acoral_u32 size);

/**
 * @brief 检查环形缓冲区是否已满
 * 
 * @param  p_ringbuffer     环形缓冲区结构体指针
 * @return acoral_u8      1：已满
 *                          0：未满
 */
static inline acoral_u8 cat_ringbuffer_is_full(cat_ringbuffer_t *p_ringbuffer)
{
    acoral_u8 ret;

    /**
     * 这里
     * ((p_ringbuffer->head_index - p_ringbuffer->tail_index) & (p_ringbuffer->ring_mask))
     * 这个最外层括号必须加，否则就会把(p_ringbuffer->ring_mask)) == 
        (p_ringbuffer->ring_mask)的结果(一定为真)和前面相减的结果按位与，就错辣！
     */
    ret = (
        ((p_ringbuffer->head_index - p_ringbuffer->tail_index) & (p_ringbuffer->ring_mask)) == 
        (p_ringbuffer->ring_mask)
        );

    return ret;
}

/**
 * @brief 检查环形缓冲区是否为空
 * 
 * @param  p_ringbuffer     环形缓冲区结构体指针
 * @return acoral_u8      1：非空
 *                          0；为空
 */
static inline acoral_u8 cat_ringbuffer_is_empty(cat_ringbuffer_t *p_ringbuffer)
{
    acoral_u8 ret;

    ret = ((p_ringbuffer->head_index) == (p_ringbuffer->tail_index));

    return ret;
}

#endif /* #ifndef RINGBUFFER_H */