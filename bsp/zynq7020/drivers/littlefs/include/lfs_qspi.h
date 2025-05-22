/**
 * @file lfs_qspi.h
 * @author 胡旋
 * @brief 
 * @version 0.1
 * @date 2025-03-24
 * 
 * 
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>胡旋 <td>2025-03-24 <td>初步littlefs移植
 * <tr><td>v1.0 <td>胡旋 <td>2025-05-14 <td>添加注释
 * <tr><td>v1.0 <td>胡旋 <td>2025-05-22 <td>规范代码风格
 * </table>
 */
#ifndef LFS_QSPI_H
#define LFS_QSPI_H

#include "lfs.h"
#include "xqspips.h"

/* QSPI设备ID */
#define QSPI_DEVICE_ID XPAR_XQSPIPS_0_DEVICE_ID     
 
/*发送到 Flash 器件的指令*/
#define WRITE_STATUS_CMD  0x01   /*写入状态寄存器*/  
#define WRITE_CMD         0x02   /*写入数据*/    
#define READ_CMD          0x03   /*读取数据*/
#define WRITE_DISABLE_CMD 0x04   /*禁用写操作*/
#define READ_STATUS_CMD   0x05   /*读取状态寄存器*/
#define WRITE_ENABLE_CMD  0x06   /*使能写操作*/
#define FAST_READ_CMD     0x0B   /*快速读取*/
#define DUAL_READ_CMD     0x3B   /*双线读取*/
#define QUAD_READ_CMD     0x6B   /*四线读取*/
#define BULK_ERASE_CMD    0xC7   /*全片擦除*/
#define SEC_ERASE_CMD     0xD8   /*扇区擦除*/
#define READ_ID           0x9F   /*读取厂商和设备ID*/

/*Flash BUFFER 中各数据的偏移量*/
#define COMMAND_OFFSET     0     /*操作指令偏移位置*/
#define ADDRESS_1_OFFSET   1     /*地址最高字节*/
#define ADDRESS_2_OFFSET   2     /*地址中间字节*/
#define ADDRESS_3_OFFSET   3     /*地址最低字节*/
#define DATA_OFFSET        4     /*数据起始偏移*/
#define DUMMY_OFFSET       4     /*Dummy字节偏移位置*/
#define DUMMY_SIZE         1     /*Dummy字节数量*/ 
#define RD_ID_SIZE         4     /*读取ID：命令字节+3字节ID响应*/ 
#define BULK_ERASE_SIZE    1     /*全片擦除，仅包含命令字节*/ 
#define SEC_ERASE_SIZE     4     /*扇区擦除：命令字节 + 3 字节扇区地址*/ 
#define OVERHEAD_SIZE      4     /*控制开销：命令字节 + 三字节地址*/ 
#define SECTOR_SIZE      0x10000 /*每个扇区的大小（64KB）*/
#define NUM_SECTORS       256    /*总扇区数量（256 个扇区）*/
#define NUM_PAGES        0x10000 /*总页面数量（65536 页）*/
#define PAGE_SIZE          256   /*每页大小（256 字节）*/
#define PAGE_COUNT         16    /*写入的Flash页面数*/

/*数据要写入Flash起始地址*/
#define STARTE_ADDRESS 0x00055000  
/*最大数据字节*/
#define MAX_DATA (PAGE_COUNT * PAGE_SIZE)   

/*全局读写缓冲区*/
acoral_u8 read_buffer[MAX_DATA + DATA_OFFSET + DUMMY_SIZE];
acoral_u8 write_buffer[PAGE_SIZE + DATA_OFFSET];

/*zynq7020qspi-flash操作函数*/
void flash_erase(XQspiPs *qspi_ptr, acoral_u32 address, acoral_u32 byte_count);
void flash_write(XQspiPs *qspi_ptr, acoral_u32 address, acoral_u32 byte_count, acoral_u8 command);
void flash_read(XQspiPs *qspi_ptr, acoral_u32 address, acoral_u32 byte_count, acoral_u8 command);
acoral_32 flash_read_id(void);
void flash_quad_enable(XQspiPs *qspi_ptr);
acoral_32 init_qspi_flash();

/*little-fs测试函数*/
int lfs_test(void);

#endif
