
/******************************************************************************
//              GND   电源地
//              VCC   接5V或3.3v电源
//              SCL   接PA5（SCL）
//              SDA   接PA7（SDA）
//              RES   接PB0
//              DC    接PB1
//              CS    接PA4 
//				BL	  背光控制，如无需控制可接3.3V常亮
//            	取模参数：
//            	    字体大小：16×16
//            	    点阵格式：阴码
//            	    取模方式：逐行式
//            	    取模走向：顺向（高位在前）
*******************************************************************************/
#include "lcd_driver.h"
#include "systick.h"
#include "stdlib.h"  //函数调用头文件
#include "stdio.h"   //为了解除sprintf警告
#include "lcd_calculate.h"    

//液晶IO初始化配置
void LCD_GPIO_Init(void)
{
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_15);
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);

	gpio_bit_set(GPIOB,GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
	gpio_bit_set(GPIOA,GPIO_PIN_15);
}

//向SPI总线传输一个8位数据
void  SPI_WriteData(u8 Data)
{
	unsigned char i=0;
	for(i=8;i>0;i--)
	{
		if(Data&0x80)	
			LCD_SDA_SET; //输出数据
		else 
			LCD_SDA_CLR;
	   
		LCD_SCL_CLR;       
		LCD_SCL_SET;
		Data<<=1; 
	}
}

//向液晶屏写一个8位指令
void Lcd_WriteIndex(u8 Index)
{
	//SPI 写命令时序开始
	LCD_CS_CLR;
	LCD_RS_CLR;
	SPI_WriteData(Index);
	LCD_CS_SET;
}

//向液晶屏写一个8位数据
void Lcd_WriteData(u8 Data)
{
   LCD_CS_CLR;
   LCD_RS_SET;
   SPI_WriteData(Data);
   LCD_CS_SET; 
}
//向液晶屏写一个16位数据
void LCD_WriteData_16Bit(u16 Data)
{
	LCD_CS_CLR;
	LCD_RS_SET;
	SPI_WriteData(Data>>8); 	//写入高8位数据
	SPI_WriteData(Data); 			//写入低8位数据
	LCD_CS_SET; 
}

void Lcd_WriteReg(u8 Index,u8 Data)
{
	Lcd_WriteIndex(Index);
	Lcd_WriteData(Data);
}

void Lcd_Reset(void)
{
	LCD_RST_CLR;
	delay_ms(100);
	LCD_RST_SET;
	delay_ms(50);
}

//LCD Init For 1.44Inch LCD Panel with ST7735R.
void Lcd_Init(void)
{	
	LCD_GPIO_Init();
	Lcd_Reset(); //Reset before LCD Init.

	//LCD Init For 1.44Inch LCD Panel with ST7735R.
	Lcd_WriteIndex(0x11);//Sleep exit 
	delay_ms (120);
		
	//ST7735R Frame Rate
	Lcd_WriteIndex(0xB1); 
	Lcd_WriteData(0x01); 
	Lcd_WriteData(0x2C); 
	Lcd_WriteData(0x2D); 

	Lcd_WriteIndex(0xB2); 
	Lcd_WriteData(0x01); 
	Lcd_WriteData(0x2C); 
	Lcd_WriteData(0x2D); 

	Lcd_WriteIndex(0xB3); 
	Lcd_WriteData(0x01); 
	Lcd_WriteData(0x2C); 
	Lcd_WriteData(0x2D); 
	Lcd_WriteData(0x01); 
	Lcd_WriteData(0x2C); 
	Lcd_WriteData(0x2D); 
	
	Lcd_WriteIndex(0xB4); //Column inversion 
	Lcd_WriteData(0x07); 
	
	//ST7735R Power Sequence
	Lcd_WriteIndex(0xC0); 
	Lcd_WriteData(0xA2); 
	Lcd_WriteData(0x02); 
	Lcd_WriteData(0x84); 
	Lcd_WriteIndex(0xC1); 
	Lcd_WriteData(0xC5); 

	Lcd_WriteIndex(0xC2); 
	Lcd_WriteData(0x0A); 
	Lcd_WriteData(0x00); 

	Lcd_WriteIndex(0xC3); 
	Lcd_WriteData(0x8A); 
	Lcd_WriteData(0x2A); 
	Lcd_WriteIndex(0xC4); 
	Lcd_WriteData(0x8A); 
	Lcd_WriteData(0xEE); 
	
	Lcd_WriteIndex(0xC5); //VCOM 
	Lcd_WriteData(0x0E); 
	
	Lcd_WriteIndex(0x36); //MX, MY, RGB mode 
	Lcd_WriteData(0xC0); 
	
	//ST7735R Gamma Sequence
	Lcd_WriteIndex(0xe0); 
	Lcd_WriteData(0x0f); 
	Lcd_WriteData(0x1a); 
	Lcd_WriteData(0x0f); 
	Lcd_WriteData(0x18); 
	Lcd_WriteData(0x2f); 
	Lcd_WriteData(0x28); 
	Lcd_WriteData(0x20); 
	Lcd_WriteData(0x22); 
	Lcd_WriteData(0x1f); 
	Lcd_WriteData(0x1b); 
	Lcd_WriteData(0x23); 
	Lcd_WriteData(0x37); 
	Lcd_WriteData(0x00); 	
	Lcd_WriteData(0x07); 
	Lcd_WriteData(0x02); 
	Lcd_WriteData(0x10); 

	Lcd_WriteIndex(0xe1); 
	Lcd_WriteData(0x0f); 
	Lcd_WriteData(0x1b); 
	Lcd_WriteData(0x0f); 
	Lcd_WriteData(0x17); 
	Lcd_WriteData(0x33); 
	Lcd_WriteData(0x2c); 
	Lcd_WriteData(0x29); 
	Lcd_WriteData(0x2e); 
	Lcd_WriteData(0x30); 
	Lcd_WriteData(0x30); 
	Lcd_WriteData(0x39); 
	Lcd_WriteData(0x3f); 
	Lcd_WriteData(0x00); 
	Lcd_WriteData(0x07); 
	Lcd_WriteData(0x03); 
	Lcd_WriteData(0x10);  
	
	Lcd_WriteIndex(0x2a);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x7f);

	Lcd_WriteIndex(0x2b);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x9f);
	
	Lcd_WriteIndex(0xF0); //Enable test command  
	Lcd_WriteData(0x01); 
	Lcd_WriteIndex(0xF6); //Disable ram power save mode 
	Lcd_WriteData(0x00); 
	
	Lcd_WriteIndex(0x3A); //65k mode 
	Lcd_WriteData(0x05); 
	
	
	Lcd_WriteIndex(0x29);//Display on	 
}


/*************************************************
函数名：LCD_Set_Region
功能：设置lcd显示区域，在此区域写点数据自动换行
入口参数：xy起点和终点
返回值：无
*************************************************/
void Lcd_SetRegion(u16 x_start,u16 y_start,u16 x_end,u16 y_end)
{		
	Lcd_WriteIndex(0x2a);
	Lcd_WriteData(0x00);
	Lcd_WriteData(x_start);//Lcd_WriteData(x_start+2);
	Lcd_WriteData(0x00);
	Lcd_WriteData(x_end+2);

	Lcd_WriteIndex(0x2b);
	Lcd_WriteData(0x00);
	Lcd_WriteData(y_start+0);
	Lcd_WriteData(0x00);
	Lcd_WriteData(y_end+1);
	
	Lcd_WriteIndex(0x2c);

}

/*************************************************
函数名：LCD_Set_XY
功能：设置lcd显示起始点
入口参数：xy坐标
返回值：无
*************************************************/
void Lcd_SetXY(u16 x,u16 y)
{
  	Lcd_SetRegion(x,y,x,y);
}

	
/*************************************************
函数名：LCD_DrawPoint
功能：画一个点
入口参数：无
返回值：无
*************************************************/
void Gui_DrawPoint(u16 x,u16 y,u16 Data)
{
	Lcd_SetRegion(x,y,x+1,y+1);
	LCD_WriteData_16Bit(Data);

}    

/*****************************************
 函数功能：读TFT某一点的颜色                          
 出口参数：color  点颜色值                                 
******************************************/
// unsigned int Lcd_ReadPoint(u16 x,u16 y)
// {
//   unsigned int Data;
//   Lcd_SetXY(x,y);

//   //Lcd_ReadData();//丢掉无用字节
//   //Data=Lcd_ReadData();
//   Lcd_WriteData(Data);
//   return Data;
// }
/*************************************************
函数名：Lcd_Clear
功能：全屏清屏函数
入口参数：填充颜色COLOR
返回值：无
*************************************************/
void Lcd_Clear(u16 Color)     //刷新全屏           
{	
   unsigned int i,m;
   Lcd_SetRegion(0,0,X_MAX_PIXEL-1,Y_MAX_PIXEL-1);
   Lcd_WriteIndex(0x2C);
   for(i=0;i<X_MAX_PIXEL;i++)
    for(m=0;m<Y_MAX_PIXEL;m++)
    {	
	  	LCD_WriteData_16Bit(Color);
    }   
}

void Lcd_Clear_1(u16 Color)      //刷新局部           
{	
	unsigned int i,m;
	Lcd_SetRegion(105,137,120,152-1);
	Lcd_WriteIndex(0x2C);
	for(i=0;i<120;i++)
	for(m=0;m<152;m++)
    {	
	  	LCD_WriteData_16Bit(Color);
    }  
}

/*
屏幕显示测试
*/

void Redraw_Mainmenu(void)
{
	Lcd_Clear(GRAY0);
	Gui_DrawFont_GBK16(5,5,BLUE,GRAY0,"智能环境监测实验");
  	DisplayButtonUp(5,25,45,45); //x1,y1,x2,y2
	Gui_DrawFont_GBK16(10,25,BLACK,GRAY0,"温度");
	DisplayButtonUp(5,47,45,67); //x1,y1,x2,y2
	Gui_DrawFont_GBK16(10,47,RED,GRAY0,"湿度");
	DisplayButtonUp(5,69,45,89); //x1,y1,x2,y2
	Gui_DrawFont_GBK16(10,69,GREEN,GRAY0,"API");
	DisplayButtonUp(5,91,45,111); //x1,y1,x2,y2
	Gui_DrawFont_GBK16(10,91,YELLOW,GRAY0,"H2S");
	DisplayButtonUp(5,113,45,133); //x1,y1,x2,y2
	Gui_DrawFont_GBK16(10,113,GRAY1,GRAY0,"NH3");
	DisplayButtonUp(50,25,125,45); //x1,y1,x2,y2  //TO
	DisplayButtonUp(50,47,125,67); //x1,y1,x2,y2  //ONE
	DisplayButtonUp(50,69,125,89); //x1,y1,x2,y2  
	DisplayButtonUp(50,91,125,111); //x1,y1,x2,y2  
	DisplayButtonUp(50,113,125,133); //x1,y1,x2,y2 
}

void ASCLL_Test(void)
{	
	Gui_DrawFont_GBK16(90,25, RED,   GRAY0, "txt1");	
	Gui_DrawFont_GBK16(90,47, GREEN, GRAY0, "txt2");
	Gui_DrawFont_GBK16(90,69, BLUE,	 GRAY0, "txt3");
	Gui_DrawFont_GBK16(90,91, RED,	 GRAY0, "txt4");
	Gui_DrawFont_GBK16(90,113,GREEN, GRAY0, "txt5");	
	Gui_DrawFont_GBK16(90,135,BLUE,	 GRAY0, "txt6");	
}



void Font_Test(void)
{
//	Lcd_Clear(GRAY0);
  Gui_DrawFont_GBK16(20,10,BLUE,GRAY0,"网络连接中...");
}

void Color_Test(void)
{
	u8 i=1;
	Lcd_Clear(GRAY0);
	
	Gui_DrawFont_GBK16(20,10,BLUE,GRAY0,"Color Test");
	delay_ms(200);

	while(i--)
	{
		Lcd_Clear(WHITE);
		Lcd_Clear(BLACK);
		Lcd_Clear(RED);
	  	Lcd_Clear(GREEN);
	  	Lcd_Clear(BLUE);
	}		
}

//取模方式 水平扫描 从左到右 低位在前
void showimage(const unsigned char *p) //显示40*40 QQ图片
{
  	int i,j,k; 
	unsigned char picH,picL;
	Lcd_Clear(WHITE); //清屏  
	
	for(k=0;k<4;k++)
	{
	   	for(j=0;j<3;j++)
		{	
			Lcd_SetRegion(40*j+2,40*k,40*j+39,40*k+39);		//坐标设置
		    for(i=0;i<40*40;i++)
			 {	
			 	picL=*(p+i*2);	//数据低位在前
				picH=*(p+i*2+1);				
				LCD_WriteData_16Bit(picH<<8|picL);  						
			 }	
		 }
	}		
}
