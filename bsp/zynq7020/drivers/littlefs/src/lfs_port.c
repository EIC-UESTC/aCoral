/**
 * @file lfs_test.c
 * @author 胡旋
 * @brief 文件系统Little-fs配置结构体及flash操作接口函数
 * @version 0.1
 * @date 2024-03-24
 * 
 * Copyright (c) 2025
 * 
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>胡旋 <td>2025-03-24 <td>初步littlefs移植
 * <tr><td>v1.0 <td>胡旋 <td>2025-05-09 <td>添加注释
 * <tr><td>v1.0 <td>胡旋 <td>2025-05-14 <td>规范代码风格
 * </table>
 */
#include "lfs.h"
#include "lfs_qspi.h"

/*QSPI设备实例*/
extern XQspiPs qspi_instance;   

static int qspi_flash_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
static int qspi_flash_prog(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
static int qspi_flash_erase(const struct lfs_config *cfg, lfs_block_t block);
static int qspi_flash_sync(const struct lfs_config *cfg);

/*文件系统配置*/
const struct lfs_config my_lfs_config = {
    /*硬件操作函数 */
    .read  = qspi_flash_read,           /*从 Flash 读取数据的函数*/
    .prog  = qspi_flash_prog,           /*写入数据到 Flash 的函数*/
    .erase = qspi_flash_erase,          /*擦除 Flash 块的函数*/    
    .sync  = qspi_flash_sync,           /*同步操作*/
    /*Flash 物理特性配置*/ 
    .read_size = 16,                    /*最小读取单位*/
    .prog_size = 16,                    /*最小编程单位*/
    .block_size = 0x10000,              /*Flash 物理擦除块大小*/  
    .block_count = 0x100,               /*Flash 总块数*/ 
    .cache_size = 16,                   /*读写缓存大小*/
    .lookahead_size = 32,               /*空闲块搜索位图大小*/
    .block_cycles = 500                 /*每个块的最大擦写周期*/
};
/**
 * @brief 从QSPI Flash读取数据
 * 
 * @param cfg     LittleFS 配置指针
 * @param block   逻辑块号
 * @param off     块内偏移地址
 * @param buffer  目标缓冲区指针，用于存储读取的数据
 * @param size    需要读取的数据长度
 * @return acoral_32 错误码：LFS_ERR_OK：成功，LFS_ERR_IO 硬件错误
 */
static acoral_32 qspi_flash_read(
    const struct lfs_config *cfg, 
    lfs_block_t block, 
    lfs_off_t off, 
    void *buffer, 
    lfs_size_t size
) 
{ 
    // 检查参数合法性
    if (block >= cfg->block_count || off + size > cfg->block_size)
    {
        return LFS_ERR_IO;
    }
    // 计算物理地址
    acoral_u32 address = block * cfg->block_size + off;
    // 调用 QSPI Flash 读取函数
    flash_read(&qspi_instance, address,size, READ_CMD);
    // 剥离协议头，拷贝有效数据
    acoral_memcpy(buffer, read_buffer + OVERHEAD_SIZE, size);
    return LFS_ERR_OK;
}
/**
 * @brief 向 QSPI Flash 写入数据
 * 
 * @param cfg      LittleFS 配置指针
 * @param block    逻辑块号
 * @param off      块内偏移地址
 * @param buffer   源缓冲区指针
 * @param size     写入数据长度
 * @return acoral_32 LFS_ERR_OK：成功，LFS_ERR_IO 硬件错误
 */
static acoral_32 qspi_flash_prog(
    const struct lfs_config *cfg, 
    lfs_block_t block, 
    lfs_off_t off, 
    const void *buffer, 
    lfs_size_t size
) 
{
    // 计算物理地址
    acoral_u32 address = block * cfg->block_size + off;
    // 调用 QSPI Flash 写入函数
    acoral_memcpy( write_buffer + OVERHEAD_SIZE, buffer, size);
    flash_write(&qspi_instance, address,size, WRITE_CMD);
    return LFS_ERR_OK;
}
/**
 * @brief 擦除 QSPI Flash 的指定块
 * 
 * @param cfg    LittleFS 配置指针
 * @param block  逻辑块号
 * @return acoral_32 LFS_ERR_OK：成功，LFS_ERR_IO 硬件错误
 * 
 * @note 擦除操作粒度必须与 block_size 严格匹配
 */
static acoral_32 qspi_flash_erase(const struct lfs_config *cfg, lfs_block_t block) 
{
    // 计算物理地址
    acoral_u32 address = block * cfg->block_size;
    acoral_u32 bytes = cfg->block_size;

    // 调用 QSPI Flash 擦除函数
    //  flash_erase(&qspi_instance, address, MAX_DATA);
    flash_erase(&qspi_instance, address, bytes);
    return LFS_ERR_OK;
}
/**
 * @brief 同步 Flash 操作
 * 
 * @param cfg  LittleFS 配置指针
 * @return acoral_32 LFS_ERR_OK：成功，LFS_ERR_IO 硬件错误 
 * 
 * @note 对于 QSPI Flash 通常无需额外操作
 */
static acoral_32 qspi_flash_sync(const struct lfs_config *cfg) 
{
    (void)cfg;   
    return LFS_ERR_OK;
}


