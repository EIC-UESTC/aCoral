/**
 * @file lwip_threads.c
 * @author 胡旋
 * @brief
 * @version 0.1
 * @date 2025-03-24
 *
 * Copyright (c) 2025
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>胡旋 <td>2025-05-14 <td>修改、增加测试内容
 * <tr><td>v1.0 <td>胡旋 <td>2025-05-14 <td>添加注释
 * <tr><td>v1.0 <td>胡旋 <td>2025-05-22 <td>规范代码风格
 * </table>
 */
#include <acoral.h>
#include "lfs.h"
#include "lfs_qspi.h"

#define MAXNUM 128

lfs_t lfs;
lfs_file_t file;
extern const struct lfs_config my_lfs_config;

int lfs_test(void)
{
    /*初始化QSPI*/
    acoral_32 res = init_qspi_flash();
    if(res)
    {
        acoral_print("FlashQuadEnable failed!\r\n");
        return res;    
    }
    acoral_print("FlashQuadEnable succeed\r\n");
    /*littlefs配置*/
    acoral_u32 err = lfs_mount(&lfs, &my_lfs_config);
    if (err) 
    {
        acoral_print("Mount failed, formatting...\r\n");
        lfs_format(&lfs, &my_lfs_config);   // 格式化文件系统
        err = lfs_mount(&lfs, &my_lfs_config);    // 重新挂载
        if(err)
        {
            acoral_print("lfs_mount error again!\r\n");
            return err;
        }
    }
    acoral_print("lfs_mount succed !\r\n");
    /* 测试文件读写 */
    lfs_file_t file;
    acoral_char *wbuf = "test the litle-fs file system ";
    acoral_char rbuf[MAXNUM] = "\0";
    acoral_32 count = 0;

    lfs_file_open(&lfs, &file, "test.txt", LFS_O_CREAT | LFS_O_RDWR);
    count += lfs_file_write(&lfs, &file, wbuf, acoral_str_len(wbuf));
    lfs_file_close(&lfs, &file);
    acoral_print("write: \"test the litle-fs file system \"\r\n");

    /*再次写入文件数据*/
    lfs_file_open(&lfs, &file, "test.txt", LFS_O_CREAT | LFS_O_RDWR | LFS_O_APPEND);
    // lfs_file_seek(&lfs, &file, 0, LFS_SEEK_CUR);
    acoral_print("write : \"add new string\"\r\n");
    wbuf = "add new string";
    count += lfs_file_write(&lfs, &file, wbuf, acoral_str_len(wbuf));
    // lfs_file_sync(&lfs, &file);
    lfs_file_close(&lfs, &file);
    
    /*读取文件数据*/
    lfs_file_open(&lfs, &file, "test.txt",LFS_O_RDONLY);
    acoral_u32 nr = lfs_file_read(&lfs, &file, rbuf, count);
    if(nr > 0)
    {
        rbuf[nr] = '\0';
        acoral_print("read : \"%s\"\r\n", rbuf);
    }
    else
    {
        acoral_print("[ERROR] fail to read file\r\n");
    }
    lfs_file_close(&lfs, &file);

    /*测试再次重新写入数据*/
    acoral_print("write : \"write new data\"\r\n");
    lfs_file_open(&lfs, &file, "test.txt",LFS_O_WRONLY | LFS_O_TRUNC);
    // lfs_file_rewind(&lfs,&file);
    wbuf = "write new data";
    count = lfs_file_write(&lfs, &file, wbuf, acoral_str_len(wbuf));
    lfs_file_close(&lfs, &file);

    /*读取文件数据*/
    lfs_file_open(&lfs, &file, "test.txt",LFS_O_RDONLY);
    nr = lfs_file_read(&lfs, &file, rbuf, count);
    if(nr > 0)
    {
        rbuf[nr] = '\0';
        acoral_print("read : \"%s\"\r\n", rbuf);
    }
    else
    {
        acoral_print("[ERROR] fail to read file\r\n");
    }
    lfs_file_close(&lfs, &file);
    
    lfs_unmount(&lfs);
    return KR_OK;
}
