/**
 * @file    w25qxx.c
 * @brief   W25Qxx 系列 SPI Flash 驱动（W25Q16/32/64/128）
 * @note    - 使用 SPI1（PB13-SCK, PB14-MISO, PB15-MOSI, PB12-CS）
 *          - 支持页写（256B）、扇区擦除（4KB）、整片擦除
 *          - 写操作前自动判断是否需要擦除
 */

#include "w25qxx.h" 
#include "systick.h"

/** @brief W25Qxx 芯片型号（默认 W25Q128） */
uint16_t W25QXX_TYPE=W25Q128;	

/**
 * @brief 通过 SPI1 发送一个字节并接收一个字节的数据
 * @param TxData 要发送的数据字节
 * @return 从 SPI 设备接收到的数据字节
 */
uint8_t SPI1_ReadWriteByte(uint8_t TxData)
{		
	uint8_t retry = 0;		
    
    while(RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_TBE));
        spi_i2s_data_transmit(SPI1, TxData);

    while(RESET == spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE));

	return spi_i2s_data_receive(SPI1);			
}

/**
 * @brief 初始化 W25Qxx Flash
 * @details 初始化 GPIO 和 SPI2，并配置 CS 引脚为高电平。
 * @retval None
 */
void W25QXX_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_SPI1);

	gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_15 | GPIO_PIN_13);
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_14);

    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12);

    gpio_bit_set(GPIOB, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    spi_parameter_struct spi_init_struct;
    /* deinitilize SPI and the parameters */
    spi_i2s_deinit(SPI1);
    spi_struct_para_init(&spi_init_struct);

    /* SPI1 parameter config */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_2;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(SPI1, &spi_init_struct);
}

/**
 * @brief 读取 W25Qxx Flash 的状态寄存器
 * @retval 当前的状态寄存器值
 */
uint8_t W25QXX_ReadSR(void)   
{
	uint8_t byte = 0;   
	gpio_bit_reset(GPIOB, GPIO_PIN_12);												 
	SPI1_ReadWriteByte(W25X_ReadStatusReg);
	byte = SPI1_ReadWriteByte(0Xff);							
	gpio_bit_set(GPIOB, GPIO_PIN_12);												 
	return byte;   
}

/**
 * @brief 写入 W25Qxx Flash 的状态寄存器
 * @param sr 新的状态寄存器值
 * @retval None
 */
void W25QXX_Write_SR(uint8_t sr)   
{
	gpio_bit_reset(GPIOB, GPIO_PIN_12);												 
	SPI1_ReadWriteByte(W25X_WriteStatusReg);
	SPI1_ReadWriteByte(sr);               	
	gpio_bit_set(GPIOB, GPIO_PIN_12);												 
}
	
/**
 * @brief 启用 W25Qxx Flash 的写操作
 * @details 发送写使能命令以允许对 Flash 进行写操作。
 * @retval None
 */
void W25QXX_Write_Enable(void)   
{
	gpio_bit_reset(GPIOB, GPIO_PIN_12);												 
    SPI1_ReadWriteByte(W25X_WriteEnable);	
	gpio_bit_set(GPIOB, GPIO_PIN_12);												 
}
 
/**
 * @brief 禁用 W25Qxx Flash 的写操作
 * @details 发送写禁用命令以禁止对 Flash 进行写操作。
 * @retval None
 */
void W25QXX_Write_Disable(void)   
{
	gpio_bit_reset(GPIOB, GPIO_PIN_12);												 
    SPI1_ReadWriteByte(W25X_WriteDisable);
	gpio_bit_set(GPIOB, GPIO_PIN_12);												 
}		
/**
 * @brief 读取 W25Qxx Flash 的 ID
 * @details 通过发送特定命令读取 Flash 的制造商ID和设备ID。
 * @retval 返回读取到的 ID 值
 */
uint16_t W25QXX_ReadID(void)
{
	uint16_t Temp = 0;	  
	gpio_bit_reset(GPIOB, GPIO_PIN_12);
	//delay(1);					    
	SPI1_ReadWriteByte(0x90);
	//delay(1);
	SPI1_ReadWriteByte(0x00);		    
	//delay(1);
	SPI1_ReadWriteByte(0x00);		    
	//delay(1);
	SPI1_ReadWriteByte(0x00);		 			   
	//delay(1);
	Temp |= SPI1_ReadWriteByte(0xFF) << 8;
	//delay(1);
	Temp |= SPI1_ReadWriteByte(0xFF);	 
	//delay(1);
	gpio_bit_set(GPIOB, GPIO_PIN_12);						    
	return Temp;
}	

/**
 * @brief 从 W25Qxx Flash 中读取数据
 * @param pBuffer 存储读取数据的缓冲区指针
 * @param ReadAddr 在 Flash 中的起始地址
 * @param NumByteToRead 要读取的字节数
 * @retval None
 */
void W25QXX_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)   
{
	uint16_t i;   													
	gpio_bit_reset(GPIOB, GPIO_PIN_12);												 
    SPI1_ReadWriteByte(W25X_ReadData);			 
    SPI1_ReadWriteByte((uint8_t)((ReadAddr) >> 16));	
    SPI1_ReadWriteByte((uint8_t)((ReadAddr) >> 8));
    SPI1_ReadWriteByte((uint8_t)ReadAddr);
    for (i = 0; i < NumByteToRead; i++)
	{
        pBuffer[i] = SPI1_ReadWriteByte(0XFF);
    }
	gpio_bit_set(GPIOB, GPIO_PIN_12);														     
}
 
/**
 * @brief 将一页数据写入 W25Qxx Flash
 * @param pBuffer 包含要写入数据的缓冲区指针
 * @param WriteAddr 在 Flash 中的目标地址
 * @param NumByteToWrite 要写入的字节数
 * @retval None
 */
void W25QXX_Write_Page(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
	uint16_t i;
    W25QXX_Write_Enable();										
	gpio_bit_reset(GPIOB, GPIO_PIN_12);												 
    SPI1_ReadWriteByte(W25X_PageProgram);			  
    SPI1_ReadWriteByte((uint8_t)((WriteAddr) >> 16));	
    SPI1_ReadWriteByte((uint8_t)((WriteAddr) >> 8));
    SPI1_ReadWriteByte((uint8_t)WriteAddr);
    for (i = 0; i < NumByteToWrite; i++) SPI1_ReadWriteByte(pBuffer[i]);
	gpio_bit_set(GPIOB, GPIO_PIN_12);												 
	W25QXX_Wait_Busy();							   		
}

/**
 * @brief 直接写入数据到 W25Qxx Flash 不进行检查
 * @param pBuffer 包含要写入数据的缓冲区指针
 * @param WriteAddr 在 Flash 中的目标地址
 * @param NumByteToWrite 要写入的字节数
 * @retval None
 */
void W25QXX_Write_NoCheck(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)   
{							 
	uint16_t pageremain;
	pageremain = 256 - WriteAddr % 256;		 
	if (NumByteToWrite <= pageremain) pageremain = NumByteToWrite;
	while (1)
	{
		W25QXX_Write_Page(pBuffer, WriteAddr, pageremain);
		if (NumByteToWrite == pageremain) break;
		else //NumByteToWrite>pageremain
		{
			pBuffer += pageremain;
			WriteAddr += pageremain;

			NumByteToWrite -= pageremain;
			if (NumByteToWrite > 256) pageremain = 256;
			else pageremain = NumByteToWrite;
		}
	};
}

uint8_t W25QXX_BUFFER[4096];

/**
 * @brief 写入数据到 W25Qxx Flash
 * @param pBuffer 包含要写入数据的缓冲区指针
 * @param WriteAddr 在 Flash 中的目标地址
 * @param NumByteToWrite 要写入的字节数
 * @retval None
 */
void W25QXX_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)   
{
	uint32_t secpos;
	uint16_t secoff;
	uint16_t secremain;
	uint16_t i;
	uint8_t * W25QXX_BUF;
   	W25QXX_BUF = W25QXX_BUFFER;
 	secpos = WriteAddr / 4096;
	secoff = WriteAddr % 4096;
	secremain = 4096 - secoff;
	//printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);
	if (NumByteToWrite <= secremain) secremain = NumByteToWrite;
	while (1)
	{
		W25QXX_Read(W25QXX_BUF, secpos * 4096, 4096);
		for (i = 0; i < secremain; i++)
		{
			if (W25QXX_BUF[secoff + i] != 0XFF) break;	  
		}
		if (i < secremain)
		{
			W25QXX_Erase_Sector(secpos);		
			for (i = 0; i < secremain; i++)		   		
			{
				W25QXX_BUF[i + secoff] = pBuffer[i];	  
			}
			W25QXX_Write_NoCheck(W25QXX_BUF, secpos * 4096, 4096);
		}
		else W25QXX_Write_NoCheck(pBuffer, WriteAddr, secremain);
		if (NumByteToWrite == secremain) break;
		else
		{
			secpos++;
			secoff = 0;	 

		   	pBuffer += secremain;
			WriteAddr += secremain;
			NumByteToWrite -= secremain;
			if (NumByteToWrite > 4096) secremain = 4096;
			else secremain = NumByteToWrite;
		}	
	};
}

/**
 * @brief 擦除整个 W25Qxx Flash 芯片
 * @retval None
 */
void W25QXX_Erase_Chip(void)   
{
    W25QXX_Write_Enable();							 				//SET WEL 
    W25QXX_Wait_Busy();
  	gpio_bit_reset(GPIOB, GPIO_PIN_12);												 
    SPI1_ReadWriteByte(W25X_ChipErase);
	gpio_bit_set(GPIOB, GPIO_PIN_12);														     
	W25QXX_Wait_Busy();
}

/**
 * @brief 擦除 W25Qxx Flash 的一个扇区
 * @param Dst_Addr 要擦除的扇区起始地址
 * @retval None
 */
void W25QXX_Erase_Sector(uint32_t Dst_Addr)   
{
	//printf("fe:%x\r\n",Dst_Addr);	  
	Dst_Addr *= 4096;
    W25QXX_Write_Enable();										//SET WEL 	 
    W25QXX_Wait_Busy();
  	gpio_bit_reset(GPIOB, GPIO_PIN_12);												 
    SPI1_ReadWriteByte(W25X_SectorErase);
    SPI1_ReadWriteByte((uint8_t)((Dst_Addr) >> 16));	
    SPI1_ReadWriteByte((uint8_t)((Dst_Addr) >> 8));
    SPI1_ReadWriteByte((uint8_t)Dst_Addr);
	gpio_bit_set(GPIOB, GPIO_PIN_12);														     
    W25QXX_Wait_Busy();
}

/**
 * @brief 等待 W25Qxx Flash 完成当前操作
 * @retval None
 */
void W25QXX_Wait_Busy(void)   
{
	while ((W25QXX_ReadSR() & 0x01) == 0x01);
}

/**
 * @brief 让 W25Qxx Flash 进入低功耗模式
 * @retval None
 */
void W25QXX_PowerDown(void)   
{
  	gpio_bit_reset(GPIOB, GPIO_PIN_12);												 
    SPI1_ReadWriteByte(W25X_PowerDown);
	gpio_bit_set(GPIOB, GPIO_PIN_12);
    delay_us(3);
}

/**
 * @brief 从低功耗模式唤醒 W25Qxx Flash
 * @retval None
 */
void W25QXX_WAKEUP(void)   
{
  	gpio_bit_reset(GPIOB, GPIO_PIN_12);												 
    SPI1_ReadWriteByte(W25X_ReleasePowerDown);	//  send W25X_PowerDown command 0xAB
	gpio_bit_set(GPIOB, GPIO_PIN_12);
    delay_us(3);
}
