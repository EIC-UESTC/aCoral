/**
 * @file lfs_qspi.c
 * @author 胡旋
 * @brief zynq7020-QSPI_FLASHD初始化以及配置接口代码（I/O模式单个从器件模式）
 * @version 0.1
 * @date 2025-03-24
 * 
 * Copyright (c) 2025
 * 
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>胡旋 <td>2025-03-24 <td>初步littlefs移植
 * <tr><td>v1.0 <td>胡旋 <td>2025-05-14 <td>添加注释
 * <tr><td>v1.0 <td>胡旋 <td>2025-05-22 <td>规范代码风格
 * </table>
 */
#include <acoral.h>	
#include "lfs_qspi.h"

/*平台相关*/
#include "xparameters.h"	/* SDK generated parameters */
#include "xqspips.h"		/* QSPI device driver */
#include "xil_printf.h"

XQspiPs qspi_instance;	/*QSPI设备实例*/

/**
 * @brief 初始化QSPI控制器
 * 
 * @return acoral_32 设备驱动程序的通用状态代码
 */
acoral_32 init_qspi_flash(void)
{
	acoral_32 status;
	XQspiPs_Config *qspi_config;
	/* Initialize the QSPI driver so that it's ready to use*/
	qspi_config = XQspiPs_LookupConfig(QSPI_DEVICE_ID);
	if (qspi_config == NULL) {
		return XST_FAILURE;
	}

	status = XQspiPs_CfgInitialize(&qspi_instance, qspi_config,qspi_config->BaseAddress);
	if (status != XST_SUCCESS) 
	{
		return XST_FAILURE;
	}

	/* Perform a self-test to check hardware build*/
	status = XQspiPs_SelfTest(&qspi_instance);
	if (status != XST_SUCCESS) 
	{
		return XST_FAILURE;
	}

	/*
	 * Initialize the write buffer for a pattern to write to the FLASH
	 * and the read buffer to zero so it can be verified after the read,
	 * the test value that is added to the unique value allows the value
	 * to be changed in a debug environment to guarantee
	 */
	acoral_memset(read_buffer, 0x00, sizeof(read_buffer));
	acoral_memset(write_buffer, 0x00, sizeof(write_buffer));
	/*
	 * Set Manual Start and Manual Chip select options and drive HOLD_B
	 * pin high.
	 */
	XQspiPs_SetOptions(&qspi_instance, 
			XQSPIPS_MANUAL_START_OPTION  |
			XQSPIPS_FORCE_SSELECT_OPTION |
			XQSPIPS_HOLD_B_DRIVE_OPTION);

	/* Set the prescaler for QSPI clock*/
	XQspiPs_SetClkPrescaler(&qspi_instance, XQSPIPS_CLK_PRESCALE_8);

	/* Assert the FLASH chip select.*/
	XQspiPs_SetSlaveSelect(&qspi_instance);

	flash_read_id();
	flash_quad_enable(&qspi_instance);
	/* Erase the flash.*/
	flash_erase(&qspi_instance, STARTE_ADDRESS, MAX_DATA);

	return XST_SUCCESS;
}


/* This function writes to the  serial FLASH connected to the QSPI interface.
* All the data put into the buffer must be in the same page of the device with
* page boundaries being on 256 byte boundaries.
*/
/**
* @brief 	写入连接到QSPI接口的串行FLASH
*
* @param	qspi_ptr QSPI驱动组件指针.
* @param	address 包含在FLASH中写入数据的地址
* @param	byte_count 包含需要写入的字节数
* @param	command 数据写入FLASH的命令 
*/
void flash_write(
	XQspiPs *qspi_ptr, 
	acoral_u32 address, 
	acoral_u32 byte_count, 
	acoral_u8 command
)
{
	acoral_u8 write_enable_cmd = { WRITE_ENABLE_CMD };
	acoral_u8 read_enable_cmd[] = { READ_STATUS_CMD, 0 };  /* must send 2 bytes */
	acoral_u8 flash_status[2];
	/*
	 * Send the write enable command to the FLASH so that it can be
	 * written to, this needs to be sent as a seperate transfer before
	 * the write
	 */
	/*使能发送写使能命令*/
	XQspiPs_PolledTransfer(qspi_ptr, &write_enable_cmd, NULL, sizeof(write_enable_cmd));
	/*构造写命令包*/
	write_buffer[COMMAND_OFFSET]   = command;
	write_buffer[ADDRESS_1_OFFSET] = (acoral_u8)((address & 0xFF0000) >> 16);
	write_buffer[ADDRESS_2_OFFSET] = (acoral_u8)((address & 0xFF00) >> 8);
	write_buffer[ADDRESS_3_OFFSET] = (acoral_u8)(address & 0xFF);

	/*
	 * Send the write command, address, and data to the FLASH to be
	 * written, no receive buffer is specified since there is nothing to
	 * receive
	 */
	XQspiPs_PolledTransfer(qspi_ptr, 
						   write_buffer, 
						   NULL,
						   byte_count + OVERHEAD_SIZE
						  );

	/*
	 * Wait for the write command to the FLASH to be completed, it takes
	 * some time for the data to be written
	 */
	while (1) 
	{
		/*
		 * Poll the status register of the FLASH to determine when it
		 * completes, by sending a read status command and receiving the
		 * status byte
		 */
		XQspiPs_PolledTransfer(qspi_ptr,
							   read_enable_cmd, 
							   flash_status,
							   sizeof(read_enable_cmd)
							  );

		/*
		 * If the status indicates the write is done, then stop waiting,
		 * if a value of 0xFF in the status byte is read from the
		 * device and this loop never exits, the device slave select is
		 * possibly incorrect such that the device status is not being
		 * read
		 */
		if ((flash_status[1] & 0x01) == 0)
		{
			break;
		}
	}
}

/**
 * @brief 	从连接到QSPI接口的串行FLASH读取数据
 * 
* @param	qspi_ptr QSPI驱动组件指针.
* @param	address 包含在FLASH中写入数据的地址
* @param	byte_count 包含需要写入的字节数
* @param	command 数据读取FLASH的命令 
 */
void flash_read(
	XQspiPs *qspi_ptr, 
	acoral_u32 address, 
	acoral_u32 byte_count, 
	acoral_u8 command
)
{
	/*
	 * Setup the write command with the specified address and data for the
	 * FLASH
	 */
	write_buffer[COMMAND_OFFSET]   = command;
	write_buffer[ADDRESS_1_OFFSET] = (acoral_u8)((address & 0xFF0000) >> 16);
	write_buffer[ADDRESS_2_OFFSET] = (acoral_u8)((address & 0xFF00) >> 8);
	write_buffer[ADDRESS_3_OFFSET] = (acoral_u8)(address & 0xFF);

	if ((command == FAST_READ_CMD) || 
		(command == DUAL_READ_CMD) ||
	    (command == QUAD_READ_CMD)
	   ) 
		{
			byte_count += DUMMY_SIZE;
		}
	/*
	 * Send the read command to the FLASH to read the specified number
	 * of bytes from the FLASH, send the read command and address and
	 * receive the specified number of bytes of data in the data buffer
	 */
	XQspiPs_PolledTransfer(qspi_ptr, 
						   write_buffer, 
						   read_buffer,
						   byte_count + OVERHEAD_SIZE
						  );
}

/**
 * @brief 擦除连接到QSPI接口的串行FLASH
 * 
 * @param qspi_ptr QSPI驱动组件指针
 * @param address 包含在FLASH中擦除数据的地址
 * @param byte_count 包含需要擦除的字节数
 * 
 */
void flash_erase(XQspiPs *qspi_ptr, acoral_u32 address, acoral_u32 byte_count)
{
	acoral_u8 write_enable_cmd = { WRITE_ENABLE_CMD };
	acoral_u8 read_enable_cmd[] = { READ_STATUS_CMD, 0 };  /* must send 2 bytes */
	acoral_u8 flash_status[2];
	acoral_32 sector;

	/*
	 * If erase size is same as the total size of the flash, use bulk erase
	 * command
	 */
	if (byte_count == (NUM_SECTORS * SECTOR_SIZE))
	{
		/*
		 * Send the write enable command to the FLASH so that it can be
		 * written to, this needs to be sent as a seperate transfer
		 * before the erase
		 */
		XQspiPs_PolledTransfer(qspi_ptr, &write_enable_cmd, NULL, sizeof(write_enable_cmd));

		/* Setup the bulk erase command*/
		write_buffer[COMMAND_OFFSET]   = BULK_ERASE_CMD;

		/*
		 * Send the bulk erase command; no receive buffer is specified
		 * since there is nothing to receive
		 */
		XQspiPs_PolledTransfer(qspi_ptr, write_buffer, NULL, BULK_ERASE_SIZE);

		/* Wait for the erase command to the FLASH to be completed*/
		while (1) 
		{
			/*
			 * Poll the status register of the device to determine
			 * when it completes, by sending a read status command
			 * and receiving the status byte
			 */
			XQspiPs_PolledTransfer(qspi_ptr, 
								   read_enable_cmd,
						           flash_status,
								   sizeof(read_enable_cmd)
								  );

			/*
			 * If the status indicates the write is done, then stop
			 * waiting; if a value of 0xFF in the status byte is
			 * read from the device and this loop never exits, the
			 * device slave select is possibly incorrect such that
			 * the device status is not being read
			 */
			if ((flash_status[1] & 0x01) == 0) 
			{
				break;
			}
		}

		return;
	}

	/*
	 * If the erase size is less than the total size of the flash, use
	 * sector erase command
	 */
	for (sector = 0; sector < ((byte_count / SECTOR_SIZE) + 1); sector++)
	 {
		/*
		 * Send the write enable command to the SEEPOM so that it can be
		 * written to, this needs to be sent as a seperate transfer
		 * before the write
		 */
		XQspiPs_PolledTransfer(qspi_ptr, 
			  				   &write_enable_cmd, 
							   NULL,
							   sizeof(write_enable_cmd)
							  );

		/*
		 * Setup the write command with the specified address and data
		 * for the FLASH
		 */
		write_buffer[COMMAND_OFFSET]   = SEC_ERASE_CMD;
		write_buffer[ADDRESS_1_OFFSET] = (acoral_u8)(address >> 16);
		write_buffer[ADDRESS_2_OFFSET] = (acoral_u8)(address >> 8);
		write_buffer[ADDRESS_3_OFFSET] = (acoral_u8)(address & 0xFF);

		/*
		 * Send the sector erase command and address; no receive buffer
		 * is specified since there is nothing to receive
		 */
		XQspiPs_PolledTransfer(qspi_ptr, write_buffer, NULL, SEC_ERASE_SIZE);

		/*
		 * Wait for the sector erse command to the
		 * FLASH to be completed
		 */
		while (1)
		{
			/*
			 * Poll the status register of the device to determine
			 * when it completes, by sending a read status command
			 * and receiving the status byte
			 */
			XQspiPs_PolledTransfer(qspi_ptr, 
								   read_enable_cmd,
								   flash_status,
								   sizeof(read_enable_cmd)
								  );

			/*
			 * If the status indicates the write is done, then stop
			 * waiting, if a value of 0xFF in the status byte is
			 * read from the device and this loop never exits, the
			 * device slave select is possibly incorrect such that
			 * the device status is not being read
			 */
			if ((flash_status[1] & 0x01) == 0) 
			{
				break;
			}
		}

		address += SECTOR_SIZE;
	}
}

/**
 * @brief 读取连接到QSPI接口的串行FLASH ID
 * 
 * @return acoral_32 读取到id返回 XST_SUCCESS ,否则返回 XST_FAILURE.
 */
acoral_32 flash_read_id(void)
{
	acoral_32 status;

	/* Read ID in Auto mode.*/
	write_buffer[COMMAND_OFFSET]   = READ_ID;
	write_buffer[ADDRESS_1_OFFSET] = 0x23;		/* 3 dummy bytes */
	write_buffer[ADDRESS_2_OFFSET] = 0x08;
	write_buffer[ADDRESS_3_OFFSET] = 0x09;

	status = XQspiPs_PolledTransfer(&qspi_instance, write_buffer, read_buffer, RD_ID_SIZE);
	if (status != XST_SUCCESS) 
	{
		return XST_FAILURE;
	}

	acoral_print("FlashID=0x%x 0x%x 0x%x\n\r", read_buffer[1], read_buffer[2],
		   read_buffer[3]);

	return XST_SUCCESS;
}

/**
 * @brief 使能QSPI Flash的四线模式
 * 
 * @param qspi_ptr QSPI驱动组件指针
 * 
 */
void flash_quad_enable(XQspiPs *qspi_ptr)
{
	acoral_u8 write_enable_cmd = {WRITE_ENABLE_CMD};
	acoral_u8 read_enable_cmd[] = {READ_STATUS_CMD, 0};
	acoral_u8 quad_enable_cmd[] = {WRITE_STATUS_CMD, 0};
	acoral_u8 flash_status[2];

	if (read_buffer[1] == 0x9D) 
	{
		XQspiPs_PolledTransfer(qspi_ptr,
							   read_enable_cmd,
							   flash_status,
						 	   sizeof(read_enable_cmd)
							  );

		quad_enable_cmd[1] = flash_status[1] | 1 << 6;

		XQspiPs_PolledTransfer(qspi_ptr, &write_enable_cmd, NULL, sizeof(write_enable_cmd));
		XQspiPs_PolledTransfer(qspi_ptr, quad_enable_cmd, NULL, sizeof(quad_enable_cmd));
	}
}
