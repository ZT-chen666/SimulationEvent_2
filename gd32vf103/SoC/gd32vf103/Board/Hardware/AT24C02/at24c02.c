/**
  ******************************************************************************
  * @file    at24c02.c
  * @brief   i2c EEPROM(AT24C02)应用函数库
  ******************************************************************************
  * @attention
  *
  *
  ******************************************************************************
  */ 
#include "at24c02.h"
#include "gd32vf103_i2c.h"
#include "systick.h"	
#include "usart.h"		
#include <stdio.h>

#define  EEP_Firstpage      0x00
uint8_t I2c_Buf_Write[256];
uint8_t I2c_Buf_Read[256];
uint8_t I2C_Test(void);


uint16_t EEPROM_ADDRESS;

static __IO uint32_t  I2CTimeout = I2CT_LONG_TIMEOUT;   


static uint32_t I2C_TIMEOUT_UserCallback(uint8_t errorCode);


/**
  * @brief  I2C I/O配置
  * @param  无
  * @retval 无
  */
void I2C_GPIO_Config(void)
{
    /* enable GPIOB clock */
    rcu_periph_clock_enable(RCU_GPIOB);
    /* enable I2C0 clock */
    rcu_periph_clock_enable(RCU_I2C0);
	
    /* I2C0 and I2C1 GPIO ports */
    /* connect PB6 to I2C0_SCL */
    /* connect PB7 to I2C0_SDA */
    gpio_init(GPIOB, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_6 | GPIO_PIN_7);
}


/**
  * @brief  I2C 工作模式配置
  * @param  无
  * @retval 无
  */
void I2C_Mode_Configu(void)
{
    /* I2C clock configure */
    i2c_clock_config(I2C0, 100000, I2C_DTCY_2);
    /* I2C address configure */
    i2c_mode_addr_config(I2C0, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, I2Cx_OWN_ADDRESS7);
    /* enable acknowledge */
    i2c_ack_config(I2C0, I2C_ACK_ENABLE);
    /* enable I2C0 */
    i2c_enable(I2C0);
}


/**
  * @brief  I2C 外设(EEPROM)初始化
  * @param  无
  * @retval 无
  */
void I2C_EE_Init(void)
{

/* 根据头文件i2c_ee.h中的定义来选择EEPROM的设备地址 */
#ifdef EEPROM_Block0_ADDRESS
  /* 选择 EEPROM Block0 来写入 */
  EEPROM_ADDRESS = EEPROM_Block0_ADDRESS;
#endif

#ifdef EEPROM_Block1_ADDRESS  
	/* 选择 EEPROM Block1 来写入 */
  EEPROM_ADDRESS = EEPROM_Block1_ADDRESS;
#endif

#ifdef EEPROM_Block2_ADDRESS  
	/* 选择 EEPROM Block2 来写入 */
  EEPROM_ADDRESS = EEPROM_Block2_ADDRESS;
#endif

#ifdef EEPROM_Block3_ADDRESS  
	/* 选择 EEPROM Block3 来写入 */
  EEPROM_ADDRESS = EEPROM_Block3_ADDRESS;
#endif
}


/**
  * @brief   将缓冲区中的数据写到I2C EEPROM中
  * @param   
  *		@arg pBuffer:缓冲区指针
  *		@arg WriteAddr:写地址
  *     @arg NumByteToWrite:写的字节数
  * @retval  无
  */
void I2C_EE_BufferWrite(u8* pBuffer, u8 WriteAddr, u16 NumByteToWrite)
{
  u8 NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0;

  Addr = WriteAddr % I2C_PageSize;
  count = I2C_PageSize - Addr;
  NumOfPage =  NumByteToWrite / I2C_PageSize;
  NumOfSingle = NumByteToWrite % I2C_PageSize;
 
  /* If WriteAddr is I2C_PageSize aligned  */
  if(Addr == 0) 
  {
    /* If NumByteToWrite < I2C_PageSize */
    if(NumOfPage == 0) 
    {
      I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
      I2C_EE_WaitEepromStandbyState();
    }
    /* If NumByteToWrite > I2C_PageSize */
    else  
    {
      while(NumOfPage--)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, I2C_PageSize); 
    	I2C_EE_WaitEepromStandbyState();
        WriteAddr +=  I2C_PageSize;
        pBuffer += I2C_PageSize;
      }

      if(NumOfSingle!=0)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
        I2C_EE_WaitEepromStandbyState();
      }
    }
  }
  /* If WriteAddr is not I2C_PageSize aligned  */
  else 
  {
    /* If NumByteToWrite < I2C_PageSize */
    if(NumOfPage== 0) 
    {
      I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
      I2C_EE_WaitEepromStandbyState();
    }
    /* If NumByteToWrite > I2C_PageSize */
    else
    {
      NumByteToWrite -= count;
      NumOfPage =  NumByteToWrite / I2C_PageSize;
      NumOfSingle = NumByteToWrite % I2C_PageSize;	
      
      if(count != 0)
      {  
        I2C_EE_PageWrite(pBuffer, WriteAddr, count);
        I2C_EE_WaitEepromStandbyState();
        WriteAddr += count;
        pBuffer += count;
      } 
      
      while(NumOfPage--)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, I2C_PageSize);
        I2C_EE_WaitEepromStandbyState();
        WriteAddr +=  I2C_PageSize;
        pBuffer += I2C_PageSize;  
      }
      if(NumOfSingle != 0)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle); 
        I2C_EE_WaitEepromStandbyState();
      }
    }
  }  
}


/**
  * @brief   写一个字节到I2C EEPROM中
  * @param   
  *		@arg pBuffer:缓冲区指针
  *		@arg WriteAddr:写地址 
  * @retval  无
  */
uint32_t I2C_EE_ByteWrite(u8* pBuffer, u8 WriteAddr) 
{
    /* wait until I2C bus is idle */
    while(i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_I2CBSY));

    /* Send STRAT condition */
    i2c_start_on_bus(EEPROM_I2Cx);

    I2CTimeout = I2CT_FLAG_TIMEOUT;  
    /* wait until SBSEND bit is set */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_SBSEND))
    {
        if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(0);
    } 
  
    I2CTimeout = I2CT_FLAG_TIMEOUT;
    /* send slave address to I2C bus */
    i2c_master_addressing(EEPROM_I2Cx, EEPROM_ADDRESS, I2C_TRANSMITTER);
    /* wait until ADDSEND bit is set */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_ADDSEND))
    {
        if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(1);
    }  
    /* clear ADDSEND bit */
    i2c_flag_clear(EEPROM_I2Cx, I2C_FLAG_ADDSEND);
    /* wait until the transmit data buffer is empty */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_TBE));

    /* Send the EEPROM's internal address to write to */
    i2c_data_transmit(EEPROM_I2Cx, WriteAddr);
    /* wait until the TBE bit is set */
    I2CTimeout = I2CT_FLAG_TIMEOUT;
    /* Test on EV8 and clear it */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_TBE))
    {
        if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(2);
    } 
    
    /* Send the byte to be written */
    i2c_data_transmit(EEPROM_I2Cx, *pBuffer);
    /* wait until the TBE bit is set */
    I2CTimeout = I2CT_FLAG_TIMEOUT;  
    /* Test on EV8 and clear it */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_TBE))
    {
        if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(3);
    } 
    
    /* Send STOP condition */
    /* send a stop condition to I2C bus */
    i2c_stop_on_bus(EEPROM_I2Cx);
    /* wait until stop condition generate */
    while(I2C_CTL0(EEPROM_I2Cx)&0x0200);
    
    return 1;
}


/**
  * @brief   在EEPROM的一个写循环中可以写多个字节，但一次写入的字节数
  *          不能超过EEPROM页的大小，AT24C02每页有8个字节
  * @param   
  *		@arg pBuffer:缓冲区指针
  *		@arg WriteAddr:写地址
  *     @arg NumByteToWrite:写的字节数
  * @retval  无
  */
uint32_t I2C_EE_PageWrite(u8* pBuffer, u8 WriteAddr, u8 NumByteToWrite)
{
    I2CTimeout = I2CT_LONG_TIMEOUT;

    /* wait until I2C bus is idle */
    while(i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_I2CBSY));
  
    /* Send STRAT condition */
    i2c_start_on_bus(EEPROM_I2Cx);

    I2CTimeout = I2CT_FLAG_TIMEOUT;  
    /* wait until SBSEND bit is set */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_SBSEND))
    {
        if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(0);
    } 
  
    I2CTimeout = I2CT_FLAG_TIMEOUT;
    /* send slave address to I2C bus */
    i2c_master_addressing(EEPROM_I2Cx, EEPROM_ADDRESS, I2C_TRANSMITTER);
    /* wait until ADDSEND bit is set */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_ADDSEND))
    {
        if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(1);
    }  
    /* clear ADDSEND bit */
    i2c_flag_clear(EEPROM_I2Cx, I2C_FLAG_ADDSEND);
    /* wait until the transmit data buffer is empty */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_TBE));

    /* Send the EEPROM's internal address to write to */
    i2c_data_transmit(EEPROM_I2Cx, WriteAddr);
    /* wait until the TBE bit is set */
    I2CTimeout = I2CT_FLAG_TIMEOUT;
    /* Test on EV8 and clear it */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_TBE))
    {
        if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(2);
    } 

    /* While there is data to be written */
    while(NumByteToWrite--)  
    {
        /* Send the byte to be written */
        i2c_data_transmit(EEPROM_I2Cx, *pBuffer);

        /* Point to the next byte to be written */
        pBuffer++; 
    
        I2CTimeout = I2CT_FLAG_TIMEOUT;  
        /* Test on EV8 and clear it */
        while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_TBE))
        {
            if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(3);
        } 
    }

    /* Send STOP condition */
    /* send a stop condition to I2C bus */
    i2c_stop_on_bus(EEPROM_I2Cx);
    /* wait until stop condition generate */
    while(I2C_CTL0(EEPROM_I2Cx)&0x0200);
    
    return 1;
}


/**
  * @brief   从EEPROM里面读取一块数据 
  * @param   
  *		@arg pBuffer:存放从EEPROM读取的数据的缓冲区指针
  *		@arg WriteAddr:接收数据的EEPROM的地址
  *     @arg NumByteToWrite:要从EEPROM读取的字节数
  * @retval  无
  */
uint32_t I2C_EE_BufferRead(u8* pBuffer, u8 ReadAddr, u16 NumByteToRead)
{  
  
    I2CTimeout = I2CT_LONG_TIMEOUT;
    i2c_ack_config(EEPROM_I2Cx, I2C_ACK_ENABLE);
    
    /* wait until I2C bus is idle */
    while(i2c_flag_get(I2C0, I2C_FLAG_I2CBSY));
    
    /* Send STRAT condition */
    i2c_start_on_bus(EEPROM_I2Cx);

    I2CTimeout = I2CT_FLAG_TIMEOUT;  
    /* wait until SBSEND bit is set */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_SBSEND));
  
    I2CTimeout = I2CT_FLAG_TIMEOUT;
    /* send slave address to I2C bus */
    i2c_master_addressing(EEPROM_I2Cx, EEPROM_ADDRESS, I2C_TRANSMITTER);
    /* wait until ADDSEND bit is set */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_ADDSEND));
    /* clear ADDSEND bit */
    i2c_flag_clear(EEPROM_I2Cx, I2C_FLAG_ADDSEND);
    /* wait until the transmit data buffer is empty */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_TBE));
        
    /* Clear EV6 by setting again the PE bit */
    i2c_enable(EEPROM_I2Cx);

    /* Send the EEPROM's internal address to write to */
    i2c_data_transmit(EEPROM_I2Cx, ReadAddr);
    /* wait until the TBE bit is set */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_TBE));
        
    /* Send STRAT condition a second time */  
    i2c_start_on_bus(EEPROM_I2Cx);
    /* wait until SBSEND bit is set */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_SBSEND));
        
    /* Send EEPROM address for read */
    i2c_master_addressing(I2C0, EEPROM_ADDRESS , I2C_RECEIVER);
    /* wait until ADDSEND bit is set */
    while(!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND));
    /* clear ADDSEND bit */
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);
    
    /* While there is data to be read */
    while(NumByteToRead)  
    {
        if(NumByteToRead == 1)
        {
            delay_us(10);
            /* Disable Acknowledgement */
            i2c_ack_config(EEPROM_I2Cx, I2C_ACK_DISABLE);
            
            /* Send STOP condition */
            /* send a stop condition to I2C bus */
            i2c_stop_on_bus(EEPROM_I2Cx);
            /* wait until stop condition generate */
            while(I2C_CTL0(EEPROM_I2Cx)&0x0200);
        }

        /* wait until the RBNE bit is set */
        while(!i2c_flag_get(I2C0, I2C_FLAG_RBNE));    
            
        /* Read a byte from the EEPROM */
        *pBuffer = i2c_data_receive(EEPROM_I2Cx);

        /* Point to the next location where the byte read will be saved */
        pBuffer++; 
        
        /* Decrement the read bytes counter */
        NumByteToRead--;        
    }

    /* Enable Acknowledgement to be ready for another reception */
    //I2C_AcknowledgeConfig(EEPROM_I2Cx, ENABLE);
    
    return 1;
}


/**
  * @brief  Wait for EEPROM Standby state 
  * @param  无
  * @retval 无
  */
void I2C_EE_WaitEepromStandbyState(void)      
{
    do {
      /* Send STRAT condition */
      i2c_start_on_bus(EEPROM_I2Cx);
      /* wait until SBSEND bit is set */
      while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_SBSEND));
      /* send slave address to I2C bus */
      i2c_master_addressing(EEPROM_I2Cx, EEPROM_ADDRESS, I2C_TRANSMITTER);
      /* wait until ADDSEND bit is set */
      while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_ADDSEND));
      // /* clear ADDSEND bit */
      // i2c_flag_clear(EEPROM_I2Cx, I2C_FLAG_ADDSEND);
    }while (!(I2C_STAT0(EEPROM_I2Cx) & 0x0002));
    
    /* clear ADDSEND bit */
    i2c_flag_clear(EEPROM_I2Cx, I2C_FLAG_ADDSEND);
    /* wait until the transmit data buffer is empty */
    while(!i2c_flag_get(EEPROM_I2Cx, I2C_FLAG_TBE));
  
    /* Clear AF flag */
    i2c_flag_clear(EEPROM_I2Cx, I2C_STAT0_AERR);
    /* STOP condition */    
    /* send a stop condition to I2C bus */
    i2c_stop_on_bus(I2C0);
    /* wait until stop condition generate */
    while(I2C_CTL0(EEPROM_I2Cx) & 0x0200);
}

/**
  * @brief  I2C(AT24C02)读写测试
  * @param  无
  * @retval 正常返回1，异常返回0
  */
uint8_t I2C_Test(void)
{
	uint16_t i;
    
    for ( i=0; i<3; i++ ) //填充缓冲
    {   
        I2c_Buf_Write[i] = i;
    }

  //将I2c_Buf_Write中顺序递增的数据写入EERPOM中 
	I2C_EE_BufferWrite( I2c_Buf_Write, EEP_Firstpage, 3);

  //将EEPROM读出数据顺序保持到I2c_Buf_Read中
	I2C_EE_BufferRead(I2c_Buf_Read, EEP_Firstpage, 3); 
   
  //将I2c_Buf_Read中的数据通过串口打印
	for (i=0; i<3; i++)
	{	
		if(I2c_Buf_Read[i] != I2c_Buf_Write[i])
		{
            printf("0x%02X ", I2c_Buf_Read[i]);
            printf("ERROR:read != write data.\n\r");
			return 0;
		}
        printf("read I2c_Buf_Read[%d] = %d\r\n ", i, I2c_Buf_Read[i]); 
	} 
  return 1;
}

//EEPROM 读写测试
void I2C_test_process(void)
{
	if(I2C_Test() ==1)
	{
       printf("I2C(AT24C02)write and read test ok.\n\r");
	}
	else
	{
       printf("I2C(AT24C02)write and read test failed.\n\r");
	}
}

/**
  * @brief  Basic management of the timeout situation.
  * @param  errorCode：错误代码，可以用来定位是哪个环节出错.
  * @retval 返回0，表示IIC读取失败.
  */
static  uint32_t I2C_TIMEOUT_UserCallback(uint8_t errorCode)
{
  /* Block communication and all processes */
  EEPROM_ERROR("I2C 等待超时!errorCode = %d",errorCode);
  
  return 0;
}
/*********************************************END OF FILE**********************/

