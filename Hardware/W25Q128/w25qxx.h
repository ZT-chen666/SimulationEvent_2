/**
 ******************************************************************************
 * @file    w25qxx.h
 * @brief   W25Qxx 系列 SPI Flash 驱动头文件（支持 Winbond & 国产兼容型号）
 * @details
 *          - 支持 W25Q80/16/32/64/128/256 等型号
 *          - 支持国产 NM25Qxx 兼容系列（如 Puya、XMC 等）
 *          - 使用硬件 SPI1（默认 PB12-CS, PB13-SCK, PB14-MISO, PB15-MOSI）
 *          - 适配 GD32VF1xx（使用 GPIO 复用功能配置）
 *
 * @note
 *          - 所有地址均为 24-bit（最大 16MB）
 *          - 写操作前自动判断是否需要擦除（基于 0xFF 检测）
 *          - 扇区大小 = 4KB，页大小 = 256B
 ******************************************************************************
 */
#ifndef __FLASH_H
#define __FLASH_H			    
#include "gd32vf103.h"
 
/* ================================================================= */
/** @defgroup W25QXX_Chip_IDs W25Qxx 芯片 ID 定义
 * @{
 */
 
//W25Q80  ID  0XEF13
//W25Q16  ID  0XEF14
//W25Q32  ID  0XEF15
//W25Q64  ID  0XEF16	
//W25Q128 ID  0XEF17	
#define W25Q80      0xEF13  /*!< Winbond W25Q80  (1MB)  */
#define W25Q16      0xEF14  /*!< Winbond W25Q16  (2MB)  */
#define W25Q32      0xEF15  /*!< Winbond W25Q32  (4MB)  */
#define W25Q64      0xEF16  /*!< Winbond W25Q64  (8MB)  */
#define W25Q128     0xEF17  /*!< Winbond W25Q128 (16MB) */
#define W25Q256     0xEF19  /*!< Winbond W25Q256 (32MB) */

/* 国产兼容型号（如 Puya, XMC, Zetta 等）*/
#define NM25Q80     0x5213
#define NM25Q16     0x5214
#define NM25Q32     0x5215
#define NM25Q64     0x5216
#define NM25Q128    0x5217
#define NM25Q256    0x5218

/* ================================================================= */
/** @defgroup W25QXX_Exported_Variables 全局变量
 * @{
 */
extern uint16_t W25QXX_TYPE;		
/**
 * @}
 */

/* ================================================================= */
/** @defgroup W25QXX_Pin_Config 引脚配置
 * @{
 */
#define	W25QXX_CS 		PBout(12)  		
/**
 * @}
 */

/* ================================================================= */
/** @defgroup W25QXX_Commands W25Qxx 命令码定义
 * @{
 */
#define W25X_WriteEnable		0x06   
#define W25X_WriteDisable		0x04   
#define W25X_ReadStatusReg		0x05   
#define W25X_WriteStatusReg		0x01   
#define W25X_ReadData			0x03   
#define W25X_FastReadData		0x0B   
#define W25X_FastReadDual		0x3B   
#define W25X_PageProgram		0x02   
#define W25X_BlockErase			0xD8   
#define W25X_SectorErase		0x20   
#define W25X_ChipErase			0xC7 
#define W25X_PowerDown			0xB9 
#define W25X_ReleasePowerDown	0xAB   
#define W25X_DeviceID			0xAB 
#define W25X_ManufactDeviceID	0x90   
#define W25X_JedecDeviceID		0x9F 

/**
 * @brief 通过 SPI1 发送并接收一个字节
 * @param TxData 要发送的字节
 * @return 接收到的字节
 */
uint8_t SPI1_ReadWriteByte(uint8_t TxData);

/**
 * @brief 初始化 W25Qxx Flash（GPIO + SPI + 自动识别型号）
 * @retval None
 */
void W25QXX_Init(void);

/**
 * @brief 读取 W25Qxx 的 JEDEC ID（推荐方式）
 * @return 16-bit 设备 ID（如 0xEF17 = W25Q128）
 * @note 比 0x90 命令更可靠，兼容性更好
 */
uint16_t W25QXX_ReadID(void);

/**
 * @brief 读取状态寄存器（SR1）
 * @return 状态寄存器值
 * @note bit0 = BUSY, bit1 = WEL
 */
uint8_t W25QXX_ReadSR(void);

/**
 * @brief 写入状态寄存器
 * @param sr 要写入的值
 * @retval None
 */
void W25QXX_Write_SR(uint8_t sr);

/**
 * @brief 发送写使能命令
 * @retval None
 */
void W25QXX_Write_Enable(void);

/**
 * @brief 发送写禁止命令
 * @retval None
 */
void W25QXX_Write_Disable(void);

/**
 * @brief 从 Flash 读取数据
 * @param pBuffer 接收缓冲区指针（非空）
 * @param ReadAddr 读取起始地址（0 ~ 最大容量-1）
 * @param NumByteToRead 读取字节数（>0）
 * @retval None
 */
void W25QXX_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);

/**
 * @brief 向 Flash 写入数据（自动处理跨页、擦除）
 * @param pBuffer 源数据缓冲区（非空）
 * @param WriteAddr 写入起始地址
 * @param NumByteToWrite 写入字节数（建议 ≤4096）
 * @retval None
 * @warning 内部使用 4KB 全局缓冲区，确保 RAM 足够
 */
void W25QXX_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

/**
 * @brief 直接写入数据（不检查是否需擦除，仅用于已擦除区域）
 * @param pBuffer 源数据
 * @param WriteAddr 地址
 * @param NumByteToWrite 字节数（≤256 且不跨页）
 * @retval None
 */
void W25QXX_Write_NoCheck(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

/**
 * @brief 擦除整个芯片
 * @retval None
 * @warning 耗时较长（W25Q128 约 20~40 秒）
 */
void W25QXX_Erase_Chip(void);

/**
 * @brief 擦除一个 4KB 扇区
 * @param Dst_Addr 扇区号（0, 1, 2, ...），实际地址 = Dst_Addr * 4096
 * @retval None
 */
void W25QXX_Erase_Sector(uint32_t Dst_Addr);

/**
 * @brief 等待 Flash 空闲（轮询 BUSY 位）
 * @retval None
 * @note 建议在关键操作后调用（如写、擦除后）
 */
void W25QXX_Wait_Busy(void);

/**
 * @brief 进入低功耗掉电模式
 * @retval None
 */
void W25QXX_PowerDown(void);

/**
 * @brief 从掉电模式唤醒
 * @retval None
 */
void W25QXX_WAKEUP(void);

#endif /* __W25QXX_H */