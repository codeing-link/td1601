//////////////////////////////////////////////////////////////////////////////////	 
//±¾³ÌÐòÖ»¹©Ñ§Ï°Ê¹ÓÃ£¬Î´¾­×÷ÕßÐí¿É£¬²»µÃÓÃÓÚÆäËüÈÎºÎÓÃÍ¾
//²âÊÔÓ²¼þ£ºµ¥Æ¬»úSTM32F407ZGT6,ÕýµãÔ­×ÓExplorer STM32F4¿ª·¢°å,Ö÷Æµ168MHZ£¬¾§Õñ12MHZ
//QDtech-TFTÒº¾§Çý¶¯ for STM32 IOÄ£Äâ
//xiao·ë@ShenZhen QDtech co.,LTD
//¹«Ë¾ÍøÕ¾:www.qdtft.com
//ÌÔ±¦ÍøÕ¾£ºhttp://qdtech.taobao.com
//wiki¼¼ÊõÍøÕ¾£ºhttp://www.lcdwiki.com
//ÎÒË¾Ìá¹©¼¼ÊõÖ§³Ö£¬ÈÎºÎ¼¼ÊõÎÊÌâ»¶Ó­ËæÊ±½»Á÷Ñ§Ï°
//¹Ì»°(´«Õæ) :+86 0755-23594567 
//ÊÖ»ú:15989313508£¨·ë¹¤£© 
//ÓÊÏä:lcdwiki01@gmail.com    support@lcdwiki.com    goodtft@163.com 
//¼¼ÊõÖ§³ÖQQ:3002773612  3002778157
//¼¼Êõ½»Á÷QQÈº:324828016
//´´½¨ÈÕÆÚ:2018/08/09
//°æ±¾£ºV1.0
//°æÈ¨ËùÓÐ£¬µÁ°æ±Ø¾¿¡£
//Copyright(C) ÉîÛÚÊÐÈ«¶¯µç×Ó¼¼ÊõÓÐÏÞ¹«Ë¾ 2018-2028
//All rights reserved
/****************************************************************************************************
//=========================================µçÔ´½ÓÏß================================================//
//     LCDÄ£¿é                STM32µ¥Æ¬»ú
//      VCC          ½Ó        DC5V/3.3V      //µçÔ´
//      GND          ½Ó          GND          //µçÔ´µØ
//=======================================Òº¾§ÆÁÊý¾ÝÏß½ÓÏß==========================================//
//±¾Ä£¿éÄ¬ÈÏÊý¾Ý×ÜÏßÀàÐÍÎªSPI×ÜÏß
//     LCDÄ£¿é                STM32µ¥Æ¬»ú    
//    SDI(MOSI)      ½Ó          PB5          //Òº¾§ÆÁSPI×ÜÏßÊý¾ÝÐ´ÐÅºÅ
//    SDO(MISO)      ½Ó          PB4          //Òº¾§ÆÁSPI×ÜÏßÊý¾Ý¶ÁÐÅºÅ£¬Èç¹û²»ÐèÒª¶Á£¬¿ÉÒÔ²»½ÓÏß
//=======================================Òº¾§ÆÁ¿ØÖÆÏß½ÓÏß==========================================//
//     LCDÄ£¿é 					      STM32µ¥Æ¬»ú 
//       LED         ½Ó          PB13         //Òº¾§ÆÁ±³¹â¿ØÖÆÐÅºÅ£¬Èç¹û²»ÐèÒª¿ØÖÆ£¬½Ó5V»ò3.3V
//       SCK         ½Ó          PB3          //Òº¾§ÆÁSPI×ÜÏßÊ±ÖÓÐÅºÅ
//      DC/RS        ½Ó          PB14         //Òº¾§ÆÁÊý¾Ý/ÃüÁî¿ØÖÆÐÅºÅ
//       RST         ½Ó          PB12         //Òº¾§ÆÁ¸´Î»¿ØÖÆÐÅºÅ
//       CS          ½Ó          PB15         //Òº¾§ÆÁÆ¬Ñ¡¿ØÖÆÐÅºÅ
//=========================================´¥ÃþÆÁ´¥½ÓÏß=========================================//
//Èç¹ûÄ£¿é²»´ø´¥Ãþ¹¦ÄÜ»òÕß´øÓÐ´¥Ãþ¹¦ÄÜ£¬µ«ÊÇ²»ÐèÒª´¥Ãþ¹¦ÄÜ£¬Ôò²»ÐèÒª½øÐÐ´¥ÃþÆÁ½ÓÏß
//	   LCDÄ£¿é                STM32µ¥Æ¬»ú 
//      T_IRQ        ½Ó          PB1          //´¥ÃþÆÁ´¥ÃþÖÐ¶ÏÐÅºÅ
//      T_DO         ½Ó          PB2          //´¥ÃþÆÁSPI×ÜÏß¶ÁÐÅºÅ
//      T_DIN        ½Ó          PF11         //´¥ÃþÆÁSPI×ÜÏßÐ´ÐÅºÅ
//      T_CS         ½Ó          PC5          //´¥ÃþÆÁÆ¬Ñ¡¿ØÖÆÐÅºÅ
//      T_CLK        ½Ó          PB0          //´¥ÃþÆÁSPI×ÜÏßÊ±ÖÓÐÅºÅ
**************************************************************************************************/		
 /* @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, QD electronic SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
**************************************************************************************************/	
#include "lcd.h"
#include "stdlib.h"
// #include "delay.h"	 
//#include "spi.h"

#include <drv/spi.h>
#include <soc.h>
#include <dw_spi_ll.h>
//¹ÜÀíLCDÖØÒª²ÎÊý
//Ä¬ÈÏÎªÊúÆÁ
_lcd_dev lcddev;

//»­±ÊÑÕÉ«,±³¾°ÑÕÉ«
uint16_t POINT_COLOR = 0x0000,BACK_COLOR = 0xFFFF;  
uint16_t DeviceCode;	 
extern csi_spi_t spi;

/*****************************************************************************
 * @name       :void LCD_WR_REG(uint8_t data)
 * @date       :2018-08-09 
 * @function   :Write an 8-bit command to the LCD screen
 * @parameters :data:Command value to be written
 * @retvalue   :None
******************************************************************************/
void LCD_WR_REG(uint8_t data)
{ 
   LCD_CS_CLR;     
	 LCD_RS_CLR;	  
//	 LCD_RS_SET;
//	 csi_spi_send(&spi, &data, 1, 1000);
	dw_spi_transmit_data(DW_SPI0_BASE, data);
	while((dw_spi_get_status(DW_SPI0_BASE) & (DW_SPI_SR_TFE | DW_SPI_SR_BUSY)) != DW_SPI_SR_TFE);

   LCD_CS_SET;	
}

/*****************************************************************************
 * @name       :void LCD_WR_DATA(uint8_t data)
 * @date       :2018-08-09 
 * @function   :Write an 8-bit data to the LCD screen
 * @parameters :data:data value to be written
 * @retvalue   :None
******************************************************************************/
void LCD_WR_DATA(uint8_t data)
{
   	LCD_CS_CLR;
	LCD_RS_SET;
	dw_spi_transmit_data(DW_SPI0_BASE, data);
	while((dw_spi_get_status(DW_SPI0_BASE) & (DW_SPI_SR_TFE | DW_SPI_SR_BUSY)) != DW_SPI_SR_TFE);
   	LCD_CS_SET;
}

void LCD_WR_DATA2(uint8_t data)
{
	dw_spi_transmit_data(DW_SPI0_BASE, data);
	while((dw_spi_get_status(DW_SPI0_BASE) & (DW_SPI_SR_TFE | DW_SPI_SR_BUSY)) != DW_SPI_SR_TFE);
}

/*****************************************************************************
 * @name       :void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue)
 * @date       :2018-08-09 
 * @function   :Write data into registers
 * @parameters :LCD_Reg:Register address
                LCD_RegValue:Data to be written
 * @retvalue   :None
******************************************************************************/
void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue)
{	
	LCD_WR_REG(LCD_Reg);  
	LCD_WR_DATA(LCD_RegValue);	    		 
}	   

/*****************************************************************************
 * @name       :void LCD_WriteRAM_Prepare(void)
 * @date       :2018-08-09 
 * @function   :Write GRAM
 * @parameters :None
 * @retvalue   :None
******************************************************************************/	 
void LCD_WriteRAM_Prepare(void)
{
	LCD_WR_REG(lcddev.wramcmd);
}	 

/*****************************************************************************
 * @name       :void Lcd_WriteData_16Bit(uint16_t Data)
 * @date       :2018-08-09 
 * @function   :Write an 16-bit command to the LCD screen
 * @parameters :Data:Data to be written
 * @retvalue   :None
******************************************************************************/	 
void Lcd_WriteData_16Bit(uint16_t Data)
{	
  //18Bit	
	LCD_WR_DATA((Data>>8)&0xF8);//RED
	LCD_WR_DATA((Data>>3)&0xFC);//GREEN
	LCD_WR_DATA(Data<<3);//BLUE
}

/*****************************************************************************
 * @name       :void LCD_DrawPoint(uint16_t x,uint16_t y)
 * @date       :2018-08-09 
 * @function   :Write a pixel data at a specified location
 * @parameters :x:the x coordinate of the pixel
                y:the y coordinate of the pixel
 * @retvalue   :None
******************************************************************************/	
void LCD_DrawPoint(uint16_t x,uint16_t y)
{
	LCD_SetCursor(x,y);//ÉèÖÃ¹â±êÎ»ÖÃ 
	Lcd_WriteData_16Bit(POINT_COLOR); 
}

/*****************************************************************************
 * @name       :void LCD_Clear(uint16_t Color)
 * @date       :2018-08-09 
 * @function   :Full screen filled LCD screen
 * @parameters :color:Filled color
 * @retvalue   :None
******************************************************************************/	
void LCD_Clear(uint16_t Color)
{
  unsigned int i,m;  
	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);   
	LCD_CS_CLR;
	LCD_RS_SET;
	for(i=0;i<lcddev.height;i++)
	{
    for(m=0;m<lcddev.width;m++)
    {	
			Lcd_WriteData_16Bit(Color);
		}
	}
	 LCD_CS_SET;
} 

/*****************************************************************************
 * @name       :void LCD_Clear(uint16_t Color)
 * @date       :2018-08-09 
 * @function   :Initialization LCD screen GPIO
 * @parameters :None
 * @retvalue   :None
******************************************************************************/	
void LCD_GPIOInit(void)
{
}

/*****************************************************************************
 * @name       :void LCD_RESET(void)
 * @date       :2018-08-09 
 * @function   :Reset LCD screen
 * @parameters :None
 * @retvalue   :None
******************************************************************************/	
void LCD_RESET(void)
{
	LCD_RST_CLR;
	mdelay(100);	
	LCD_RST_SET;
	mdelay(50);
}

/*****************************************************************************
 * @name       :void LCD_RESET(void)
 * @date       :2018-08-09 
 * @function   :Initialization LCD screen
 * @parameters :None
 * @retvalue   :None
******************************************************************************/	 	 
void LCD_Init(void)
{  
	// SPI1_Init(); //Ó²¼þSPI³õÊ¼»¯
//	SPI_SetSpeed(SPI1,SPI_BaudRatePrescaler_2);
	LCD_GPIOInit();//LCD GPIO³õÊ¼»¯										 
 	LCD_RESET(); //LCD ¸´Î»
//************* ILI9488³õÊ¼»¯**********//	
	LCD_WR_REG(0XF7);
	LCD_WR_DATA(0xA9);
	LCD_WR_DATA(0x51);
	LCD_WR_DATA(0x2C);
	LCD_WR_DATA(0x82);
	LCD_WR_REG(0xC0);
	LCD_WR_DATA(0x11);
	LCD_WR_DATA(0x09);
	LCD_WR_REG(0xC1);
	LCD_WR_DATA(0x41);
	LCD_WR_REG(0XC5);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x0A);
	LCD_WR_DATA(0x80);
	LCD_WR_REG(0xB1);
	LCD_WR_DATA(0xB0);
	LCD_WR_DATA(0x11);
	LCD_WR_REG(0xB4);
	LCD_WR_DATA(0x02);
	LCD_WR_REG(0xB6);
	LCD_WR_DATA(0x02);
	LCD_WR_DATA(0x42);
	LCD_WR_REG(0xB7);
	LCD_WR_DATA(0xc6);
	LCD_WR_REG(0xBE);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x04);
	LCD_WR_REG(0xE9);
	LCD_WR_DATA(0x00);
	LCD_WR_REG(0x36);
	LCD_WR_DATA((1<<3)|(0<<7)|(1<<6)|(1<<5));
	LCD_WR_REG(0x3A);
	LCD_WR_DATA(0x66);
	LCD_WR_REG(0xE0);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x07);
	LCD_WR_DATA(0x10);
	LCD_WR_DATA(0x09);
	LCD_WR_DATA(0x17);
	LCD_WR_DATA(0x0B);
	LCD_WR_DATA(0x41);
	LCD_WR_DATA(0x89);
	LCD_WR_DATA(0x4B);
	LCD_WR_DATA(0x0A);
	LCD_WR_DATA(0x0C);
	LCD_WR_DATA(0x0E);
	LCD_WR_DATA(0x18);
	LCD_WR_DATA(0x1B);
	LCD_WR_DATA(0x0F);
	LCD_WR_REG(0XE1);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x17);
	LCD_WR_DATA(0x1A);
	LCD_WR_DATA(0x04);
	LCD_WR_DATA(0x0E);
	LCD_WR_DATA(0x06);
	LCD_WR_DATA(0x2F);
	LCD_WR_DATA(0x45);
	LCD_WR_DATA(0x43);
	LCD_WR_DATA(0x02);
	LCD_WR_DATA(0x0A);
	LCD_WR_DATA(0x09);
	LCD_WR_DATA(0x32);
	LCD_WR_DATA(0x36);
	LCD_WR_DATA(0x0F);
	LCD_WR_REG(0x11);
	mdelay(120);
	LCD_WR_REG(0x29);
	
  LCD_direction(USE_HORIZONTAL);//ÉèÖÃLCDÏÔÊ¾·½Ïò
	LCD_LED;//µãÁÁ±³¹â	 
	LCD_Clear(WHITE);//ÇåÈ«ÆÁ°×É«
}
 
/*****************************************************************************
 * @name       :void LCD_SetWindows(uint16_t xStar, uint16_t yStar,uint16_t xEnd,uint16_t yEnd)
 * @date       :2018-08-09 
 * @function   :Setting LCD display window
 * @parameters :xStar:the bebinning x coordinate of the LCD display window
								yStar:the bebinning y coordinate of the LCD display window
								xEnd:the endning x coordinate of the LCD display window
								yEnd:the endning y coordinate of the LCD display window
 * @retvalue   :None
******************************************************************************/ 
void LCD_SetWindows(uint16_t xStar, uint16_t yStar,uint16_t xEnd,uint16_t yEnd)
{	
	LCD_WR_REG(lcddev.setxcmd);	
	LCD_WR_DATA(xStar>>8);
	LCD_WR_DATA(0x00FF&xStar);		
	LCD_WR_DATA(xEnd>>8);
	LCD_WR_DATA(0x00FF&xEnd);

	LCD_WR_REG(lcddev.setycmd);	
	LCD_WR_DATA(yStar>>8);
	LCD_WR_DATA(0x00FF&yStar);		
	LCD_WR_DATA(yEnd>>8);
	LCD_WR_DATA(0x00FF&yEnd);

	LCD_WriteRAM_Prepare();	//¿ªÊ¼Ð´ÈëGRAM			
}   

/*****************************************************************************
 * @name       :void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
 * @date       :2018-08-09 
 * @function   :Set coordinate value
 * @parameters :Xpos:the  x coordinate of the pixel
								Ypos:the  y coordinate of the pixel
 * @retvalue   :None
******************************************************************************/ 
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{	  	    			
	LCD_SetWindows(Xpos,Ypos,Xpos,Ypos);	
} 

/*****************************************************************************
 * @name       :void LCD_direction(uint8_t direction)
 * @date       :2018-08-09 
 * @function   :Setting the display direction of LCD screen
 * @parameters :direction:0-0 degree
                          1-90 degree
													2-180 degree
													3-270 degree
 * @retvalue   :None
******************************************************************************/ 
void LCD_direction(uint8_t direction)
{ 
			lcddev.setxcmd=0x2A;
			lcddev.setycmd=0x2B;
			lcddev.wramcmd=0x2C;
	switch(direction){		  
		case 0:						 	 		
			lcddev.width=LCD_W;
			lcddev.height=LCD_H;		
			LCD_WriteReg(0x36,(1<<3)|(0<<6)|(0<<7));//BGR==1,MY==0,MX==0,MV==0
		break;
		case 1:
			lcddev.width=LCD_H;
			lcddev.height=LCD_W;
			LCD_WriteReg(0x36,(1<<3)|(0<<7)|(1<<6)|(1<<5));//BGR==1,MY==1,MX==0,MV==1
		break;
		case 2:						 	 		
			lcddev.width=LCD_W;
			lcddev.height=LCD_H;	
			LCD_WriteReg(0x36,(1<<3)|(1<<6)|(1<<7));//BGR==1,MY==0,MX==0,MV==0
		break;
		case 3:
			lcddev.width=LCD_H;
			lcddev.height=LCD_W;
			LCD_WriteReg(0x36,(1<<3)|(1<<7)|(1<<5));//BGR==1,MY==1,MX==0,MV==1
		break;	
		default:break;
	}		
}

void LCD_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
	uint16_t i, j;
	uint16_t width = ex - sx + 1;	// 得到填充的宽度
	uint16_t height = ey - sy + 1;	// 高度
	LCD_SetWindows(sx, sy, ex, ey); // 设置显示窗口
	LCD_CS_CLR;
	LCD_RS_SET;
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
			Lcd_WriteData_16Bit(color); // 写入数据
	}
	LCD_CS_SET;
	LCD_SetWindows(0, 0, lcddev.width - 1, lcddev.height - 1); // 恢复窗口设置为全屏
}