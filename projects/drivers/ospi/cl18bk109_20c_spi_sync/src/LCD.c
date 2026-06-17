#include "LCD.h"
#include "stdlib.h"
#include "FONT.h" 
// #include "usart.h"
// #include "delay.h"
#include "string.h"
#include <soc.h>
#include <dw_ospi_ll.h>
// #include "csi_gcc.h"

//LCD�Ļ�����ɫ�ͱ���ɫ	   
uint16_t POINT_COLOR=0x0000;	//������ɫ
uint16_t BACK_COLOR=0xFFFF;  //����ɫ 

//����LCD��Ҫ����
//Ĭ��Ϊ����
_lcd_dev lcddev;
	 					    
//д�Ĵ�������
//data:�Ĵ���ֵ
void LCD_WR_REG(uint16_t data)
{
	LCD_RS_CLR;//д��ַ  
 	LCD_CS_CLR; 
#if 0
	DATAOUT(data); 
	// udelay(5);
	// __NOP();
	// __NOP();
	while ((*(volatile uint32_t *)0x6000C028 &0x5) != 4)
	{
	}
#else
	 dw_ospi_transmit_data((dw_ospi_regs_t *)DW_OSPI0_BASE, data);
	 while((dw_ospi_get_status((dw_ospi_regs_t *)DW_OSPI0_BASE) & (DW_OSPI_SR_TFE | DW_OSPI_SR_BUSY)) != DW_OSPI_SR_TFE);
#endif
	// LCD_WR_CLR; 
	// LCD_WR_SET; 
 	LCD_CS_SET; 			
}
//д���ݺ���
//data:�Ĵ���ֵ
void LCD_WR_DATA(uint16_t data)
{
	LCD_RS_SET;
	LCD_CS_CLR;
#if 0
	DATAOUT(data);
	// udelay(5);
	// __NOP();
	// __NOP();
	while ((*(volatile uint32_t *)0x6000C028 &0x5) != 4)
	{
	}
#else
	 dw_ospi_transmit_data((dw_ospi_regs_t *)DW_OSPI0_BASE, data);
	 while((dw_ospi_get_status((dw_ospi_regs_t *)DW_OSPI0_BASE) & (DW_OSPI_SR_TFE | DW_OSPI_SR_BUSY)) != DW_OSPI_SR_TFE);
#endif
	// LCD_WR_CLR;
	// LCD_WR_SET;
	LCD_CS_SET;		
}

void LCD_WR_DATA2(uint16_t data)
{
#if 0
	DATAOUT(data);
	// udelay(5);
	// __NOP();
	// __NOP();
	while ((*(volatile uint32_t *)0x6000C028 &0x5) != 4)
	{
	}
#else
	 dw_ospi_transmit_data((dw_ospi_regs_t *)DW_OSPI0_BASE, data);
	 while((dw_ospi_get_status((dw_ospi_regs_t *)DW_OSPI0_BASE) & (DW_OSPI_SR_TFE | DW_OSPI_SR_BUSY)) != DW_OSPI_SR_TFE);
#endif
}
//д�Ĵ���
//LCD_Reg:�Ĵ������
//LCD_RegValue:Ҫд���ֵ
void LCD_WriteReg(uint16_t LCD_Reg,uint16_t LCD_RegValue)
{	
	LCD_WR_REG(LCD_Reg);  
	LCD_WriteRAM(LCD_RegValue);	    		 
}   

//��ʼдGRAM
void LCD_WriteRAM_Prepare(void)
{
	LCD_WR_REG(lcddev.wramcmd);
} 
//LCDдGRAM
//RGB_Code:��ɫֵ
void LCD_WriteRAM(uint16_t RGB_Code)
{							    
//дʮ��λGRAM
		LCD_WR_DATA(RGB_Code>>8);		//���͸�8Ϊ
	  LCD_WR_DATA(RGB_Code);			//���͵�8Ϊ
}

//��mdk -O1ʱ���Ż�ʱ��Ҫ����
//��ʱi
void opt_delay(uint8_t i)
{
	while(i--);
}

//LCD������ʾ
void LCD_DisplayOn(void)
{					   
	LCD_WR_REG(0X29);	//������ʾ
}	 
//LCD�ر���ʾ
void LCD_DisplayOff(void)
{	   
	LCD_WR_REG(0X28);	//�ر���ʾ
}   
//���ù��λ��
//Xpos:������
//Ypos:������
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{	 	    
   if(lcddev.dir==0)
	 { 
		LCD_WR_REG(lcddev.setxcmd); 
		LCD_WR_DATA((Xpos+2)>>8);LCD_WR_DATA((Xpos+2)&0XFF); 			 
		LCD_WR_REG(lcddev.setycmd); 
		LCD_WR_DATA((Ypos+1)>>8);LCD_WR_DATA((Ypos+1)&0XFF); 
		
	 }
  else	if(lcddev.dir==1)
		{ 
		LCD_WR_REG(lcddev.setxcmd); 
		LCD_WR_DATA((Xpos+2)>>8);LCD_WR_DATA((Xpos+2)&0XFF); 			 
		LCD_WR_REG(lcddev.setycmd); 
		LCD_WR_DATA((Ypos+1)>>8);LCD_WR_DATA((Ypos+1)&0XFF);
	 }
	else	if(lcddev.dir==2)
		{
		LCD_WR_REG(lcddev.setxcmd); 
		LCD_WR_DATA((Xpos+1)>>8);LCD_WR_DATA((Xpos+1)&0XFF); 			 
		LCD_WR_REG(lcddev.setycmd); 
		LCD_WR_DATA((Ypos+2)>>8);LCD_WR_DATA((Ypos+2)&0XFF);
    }
	else	if(lcddev.dir==3)
		{
		LCD_WR_REG(lcddev.setxcmd); 
		LCD_WR_DATA((Xpos+1)>>8);LCD_WR_DATA((Xpos+1)&0XFF); 			 
		LCD_WR_REG(lcddev.setycmd); 
		LCD_WR_DATA((Ypos+2)>>8);LCD_WR_DATA((Ypos+2)&0XFF);
    } 		 
} 		 
   
//����
//x,y:����
//POINT_COLOR:�˵����ɫ
void LCD_DrawPoint(uint16_t x,uint16_t y)
{
	LCD_SetCursor(x,y);		//���ù��λ�� 
	LCD_WriteRAM_Prepare();	//��ʼд��GRAM
	LCD_WriteRAM(POINT_COLOR); 
}	 
//���ٻ���
//x,y:����
//color:��ɫ
void LCD_Fast_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{	   
		//���ù��λ��
		LCD_SetCursor(x,y); 	 
		//д����ɫ
		LCD_WriteReg(lcddev.wramcmd,color);
}


//dir:����ѡ�� 	0-0����ת��1-180����ת��2-270����ת��3-90����ת
void LCD_Display_Dir(uint8_t dir)
{
	lcddev.dir=dir;
	if(lcddev.dir==0||lcddev.dir==1)			//����
	{
			lcddev.width=128;
			lcddev.height=160;

			lcddev.wramcmd=0X2C;
			lcddev.setxcmd=0X2A;
			lcddev.setycmd=0X2B;
		
		if(lcddev.dir==0)         //0-0����ת
		{
			LCD_WR_REG(0x36); 
			LCD_WR_DATA((1<<3)|(1<<7)|(1<<6)|(0<<5));
		}else							//1-180����ת
		{
			LCD_WR_REG(0x36); 
			LCD_WR_DATA((1<<3)|(0<<7)|(0<<6)|(0<<5));		
		}
		
	}else if(lcddev.dir==2||lcddev.dir==3)  //����
	{
		
			lcddev.width=160;
			lcddev.height=128;

			lcddev.wramcmd=0X2C;
			lcddev.setxcmd=0X2A;
			lcddev.setycmd=0X2B; 

				if(lcddev.dir==2)				//2-270����ת
				{
					LCD_WR_REG(0x36); 
					LCD_WR_DATA((1<<3)|(0<<7)|(1<<6)|(1<<5));

				}else							//3-90����ת
				{
					LCD_WR_REG(0x36); 
					LCD_WR_DATA((1<<3)|(1<<7)|(0<<6)|(1<<5));
				}		
	} 	
}	 
//���ô���,���Զ����û������굽�������Ͻ�(sx,sy).
//sx,sy:������ʼ����(���Ͻ�)
//width,height:���ڿ��Ⱥ͸߶�,�������0!!
//�����С:width*height. 
void LCD_Set_Window(uint16_t sx,uint16_t sy,uint16_t width,uint16_t height)
{    
	uint16_t ex,ey;
	ex=sx+width-1;
	ey=sy+height-1;
	
	if(lcddev.dir==0)
	{
		    sx=sx+2;
		    ex=ex+2;
		    sy=sy+1;
	      ey=ey+1;
	}else if(lcddev.dir==1)
		{
		    sx=sx+2;
		    ex=ex+2;
		    sy=sy+1;
	      ey=ey+1;
	  }else if(lcddev.dir==2)
			{
		    sx=sx+1;
		    ex=ex+1;
		    sy=sy+2;
	      ey=ey+2;
	    }else if(lcddev.dir==3)
				{
		    sx=sx+1;
		    ex=ex+1;
		    sy=sy+2;
	      ey=ey+2;
	      }
	
		LCD_WR_REG(lcddev.setxcmd); 
		LCD_WR_DATA(sx>>8); 
		LCD_WR_DATA((sx)&0XFF);	 
		LCD_WR_DATA(ex>>8); 
		LCD_WR_DATA(ex&0XFF);  
		LCD_WR_REG(lcddev.setycmd); 
		LCD_WR_DATA(sy>>8); 
		LCD_WR_DATA(sy&0XFF); 
		LCD_WR_DATA(ey>>8); 
		LCD_WR_DATA(ey&0XFF);  

}

#define delay_ms(m) mdelay(m)
//��ʼ��lcd
void LCD_Init(void)
{

	// RCC->APB2ENR|=1<<2;//��ʹ������PORTAʱ��
 	// RCC->APB2ENR|=1<<3;//��ʹ������PORTBʱ��
 	// RCC->APB2ENR|=1<<4;//��ʹ������PORTCʱ��

	// RCC->APB2ENR|=1<<0;    //��������ʱ��
	// JTAG_Set(SWD_ENABLE);  //����SWD
	
	// GPIOA->CRL&=0XFFFFF0FF;
	// GPIOA->CRL|=0X00000300;//LED(���⿪��) = PA2
	// GPIOA->CRH&=0X0FF0FFFF;
	// GPIOA->CRH|=0X30030000;//CS = PA12   RS = PA15
	// GPIOA->ODR|=0X9000;    //IO����
	
	// GPIOC->CRH&=0X000FFFFF;
	// GPIOC->CRH|=0X33300000;//WR = PC13  RD = PC14  RST = PC15 	
	// GPIOC->ODR|=0XE000;    //IO����
	
	// GPIOB->CRH=0X33333333;
	// GPIOB->CRL=0X33333333;//DBO-DB15
	// GPIOB->ODR=0XFFFF;    //IO����
	

	LCD_LED;
	LCD_RST_SET;
	delay_ms(1);
	LCD_RST_CLR;
	delay_ms(10);
	LCD_RST_SET;
	delay_ms(120);    


//************* Start Initial Sequence **********//
delay_ms(120);                //ms

LCD_WR_REG(0x11);     //Sleep out

delay_ms(120);                //delay_ms 120ms

LCD_WR_REG(0xB1);     //In normal mode
LCD_WR_DATA(0x00);   //frame rate=85.3Hz
LCD_WR_DATA(0x2C);   
LCD_WR_DATA(0x2B);   

LCD_WR_REG(0xB2);     //In Idle mode
LCD_WR_DATA(0x00);   
LCD_WR_DATA(0x01);   
LCD_WR_DATA(0x01);   

LCD_WR_REG(0xB3);     //In partial mode
LCD_WR_DATA(0x00);   
LCD_WR_DATA(0x01);   
LCD_WR_DATA(0x01);   
LCD_WR_DATA(0x00);   
LCD_WR_DATA(0x01);   
LCD_WR_DATA(0x01);   

LCD_WR_REG(0xB4);     //DOT inversion Display Inversion Control
LCD_WR_DATA(0x03);   

LCD_WR_REG(0xB9);     //In normal mode
LCD_WR_DATA(0xFF);   
LCD_WR_DATA(0x83);   
LCD_WR_DATA(0x47);   

LCD_WR_REG(0xC0);     //VRH: Set the GVDD
LCD_WR_DATA(0xA2);   
LCD_WR_DATA(0x02);   
LCD_WR_DATA(0x84);   

LCD_WR_REG(0xC1);     //set VGH/ VGL
LCD_WR_DATA(0x02);   //??02 VGH=16.6 VGL=-7.5  00 VGH=11.6 VGL=-7.5  06 VGH=16.6  VGL=-10

LCD_WR_REG(0xC2);     //APA: adjust the operational amplifier DCA: adjust the booster Voltage
LCD_WR_DATA(0x0A);   
LCD_WR_DATA(0x00);   

LCD_WR_REG(0xC3);     //In Idle mode (8-colors)
LCD_WR_DATA(0x8A);   
LCD_WR_DATA(0x2A);   

LCD_WR_REG(0xC4);     //In partial mode + Full color
LCD_WR_DATA(0x8A);   
LCD_WR_DATA(0xEE);   

LCD_WR_REG(0xC5);     //VCOM
LCD_WR_DATA(0x09);   

LCD_WR_REG(0x20);     //Display inversion

LCD_WR_REG(0xC7);     
LCD_WR_DATA(0x10);   

LCD_WR_REG(0x36);     //MX, MY, RGB mode
LCD_WR_DATA(0xC0); //08  

LCD_WR_REG(0xE0);     
LCD_WR_DATA(0x0C);   
LCD_WR_DATA(0x1C);   
LCD_WR_DATA(0x1B);   
LCD_WR_DATA(0x1A);   
LCD_WR_DATA(0x2F);   
LCD_WR_DATA(0x28);   
LCD_WR_DATA(0x20);   
LCD_WR_DATA(0x24);   
LCD_WR_DATA(0x23);   
LCD_WR_DATA(0x22);   
LCD_WR_DATA(0x2A);   
LCD_WR_DATA(0x36);   
LCD_WR_DATA(0x00);   
LCD_WR_DATA(0x05);   
LCD_WR_DATA(0x00);   
LCD_WR_DATA(0x10);   

LCD_WR_REG(0xE1);     
LCD_WR_DATA(0x0C);   
LCD_WR_DATA(0x1A);   
LCD_WR_DATA(0x1A);   
LCD_WR_DATA(0x1A);   
LCD_WR_DATA(0x2E);   
LCD_WR_DATA(0x27);   
LCD_WR_DATA(0x21);   
LCD_WR_DATA(0x24);   
LCD_WR_DATA(0x24);   
LCD_WR_DATA(0x22);   
LCD_WR_DATA(0x2A);   
LCD_WR_DATA(0x35);   
LCD_WR_DATA(0x00);   
LCD_WR_DATA(0x05);   
LCD_WR_DATA(0x00);   
LCD_WR_DATA(0x10);   

LCD_WR_REG(0x35);     //65k mode
LCD_WR_DATA(0x00);   

LCD_WR_REG(0x3A);     //65k mode
LCD_WR_DATA(0x05);   

LCD_WR_REG(0x29);     //Display on
} 


//��LCD����
//����ֵ:������ֵ
uint16_t LCD_RD_DATA(void)
{	
	uint16_t t;
 	// GPIOB->CRL=0X88888888; //PB0-7  ��������
	// GPIOB->CRH=0X88888888; //PB8-15 ��������
	// GPIOB->ODR=0X0000;     //ȫ�����0

	LCD_RS_SET;
	LCD_CS_CLR;
	//��ȡ����(���Ĵ���ʱ,������Ҫ��2��)
	LCD_RD_CLR;		   
	t=DATAIN;  
	LCD_RD_SET;
	LCD_CS_SET; 

	// GPIOB->CRL=0X33333333; //PB0-7  �������
	// GPIOB->CRH=0X33333333; //PB8-15 �������
	// GPIOB->ODR=0XFFFF;    //ȫ�������
	return t=t>>8; 
}

//����ʾ�� ID
uint16_t LCD_Read_ID(uint8_t reg)
{
	LCD_WR_REG(reg);
	LCD_RD_DATA(); 				//dummy read 	
	LCD_RD_DATA();   	    	
	lcddev.id=LCD_RD_DATA();   									   
	lcddev.id<<=8;
	lcddev.id|=LCD_RD_DATA();  	
	
	return lcddev.id;

}


//��ȡ��ĳ�����ɫֵ	 
//x,y:����
//����ֵ:�˵����ɫ
uint16_t LCD_ReadPoint(uint16_t x,uint16_t y)
{
 	uint16_t r,g,b;
	if(x>=lcddev.width||y>=lcddev.height)return 0;	//�����˷�Χ,ֱ�ӷ���		   
	LCD_SetCursor(x,y);
//		LCD_SetCursor(28,28);
	LCD_WR_REG(0X2E);//9341/6804/3510/1963 ���Ͷ�GRAMָ��
	
	// GPIOB->CRL=0X88888888; 							//PB0-7  ��������
	// GPIOB->CRH=0X88888888; 							//PB8-15 ��������
	// GPIOB->ODR=0XFFFF;     							//ȫ�������

	LCD_RS_SET;
	LCD_CS_CLR;		
	//��ȡ����(�ٶ�)	
	LCD_RD_CLR;	
  opt_delay(2);//��ʱ		
 	//r=DATAIN;  //dummy READ							
	LCD_RD_SET;
		
	//��ȡ����(�ٶ�)	
	LCD_RD_CLR;
  opt_delay(2);//��ʱ		
 	//r=DATAIN;  //dummy READ							
	LCD_RD_SET;
	
		//��ȡ����(�ٶ�)	
	LCD_RD_CLR;
  opt_delay(2);//��ʱ		
 	//r=DATAIN;  //dummy READ							
	LCD_RD_SET;
	
			//��ȡ����(��GRAMʱ,ǰ�Ĵ�Ϊ�ٶ�)	
	LCD_RD_CLR;
  opt_delay(2);//��ʱ		
 	//r=DATAIN;  //dummy READ							
	LCD_RD_SET;

	LCD_RD_CLR;	
	opt_delay(2);//��ʱ		
 	r=DATAIN;  	//��ȡ��ɫֵ
	r=r>>8;
	LCD_RD_SET;
	 
	LCD_RD_CLR;					   
	opt_delay(2);//��ʱ			   
	g=DATAIN;//��ȡ��ɫֵ  
	g=g>>8;
	LCD_RD_SET;
	
	
	LCD_RD_CLR;					   
	opt_delay(2);//��ʱ			   
	b=DATAIN;//��ȡ��ɫֵ  
	b=b>>8;
	LCD_RD_SET;
	
	LCD_CS_SET;
	
	r=r&0xFF;
	g=g&0xFF;	
	b=b&0xFF;
	
	// GPIOB->CRL=0X33333333; 		//PB0-7  �������
	// GPIOB->CRH=0X33333333; 		//PB8-15 �������
	// GPIOB->ODR=0XFFFF;    		//ȫ�������  
	
return (((r>>3)<<11)|((g>>2)<<5)|(b>>3));
//	return r;
}		



//��������
//color:Ҫ���������ɫ
void LCD_Clear(uint16_t color)
{
	uint32_t index=0;      
	uint32_t totalpoint=lcddev.width;
	totalpoint*=lcddev.height; 			                    //�õ��ܵ���
	LCD_Set_Window(0,0,lcddev.width,lcddev.height);	    //���ù��λ�� 
	LCD_WriteRAM_Prepare();     		                    //��ʼд��GRAM	  	  
	for(index=0;index<totalpoint;index++)LCD_WriteRAM(color);
}  

void LCD_Fill3(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color)
{          
	uint16_t i,j;
	uint16_t xlen=0;
	uint16_t ylen=0;
		xlen=ex-sx+1;
		ylen=ey-sy+1;
		LCD_Set_Window(sx,sy,xlen,ylen); //LCD_Set_Window(sx,i,lcddev.width,lcddev.height);     //���ù��λ��
		LCD_WriteRAM_Prepare();
		LCD_RS_SET;
		LCD_CS_CLR;
		for(i=0;i<=xlen*ylen;i++)
		{
			// for(j=0;j<xlen;j++)
			 {
				LCD_WR_DATA2(color[i]>>8);		//���͸�8Ϊ
				LCD_WR_DATA2(color[i]);			//���͵�8Ϊ
			}//LCD_WriteRAM(color);              //���ù��λ�� 	    
		}
}  

void LCD_Fill2(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color)
{          
	uint16_t i,j;
	uint16_t xlen=0;
	uint16_t ylen=0;
		xlen=ex-sx+1;
		ylen=ey-sy+1;
		LCD_Set_Window(sx,sy,xlen,ylen); //LCD_Set_Window(sx,i,lcddev.width,lcddev.height);     //���ù��λ��
		LCD_WriteRAM_Prepare();
		LCD_RS_SET;
		LCD_CS_CLR;
		for(i=sy;i<=ey;i++)
		{
			for(j=0;j<xlen;j++) {
				LCD_WR_DATA2(color>>8);		//���͸�8Ϊ
				LCD_WR_DATA2(color);			//���͵�8Ϊ
			}//LCD_WriteRAM(color);              //���ù��λ�� 	    
		}
}  

//��ָ�����������ָ����ɫ
//�����С:(xend-xsta+1)*(yend-ysta+1)
//xsta
//color:Ҫ������ɫ
void LCD_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color)
{          
	uint16_t i,j;
	uint16_t xlen=0;
	
		xlen=ex-sx+1;	 
		for(i=sy;i<=ey;i++)
		{
		 	LCD_Set_Window(sx,i,lcddev.width,lcddev.height);     //���ù��λ��
			LCD_WriteRAM_Prepare();     			                   //��ʼд��GRAM	  
			for(j=0;j<xlen;j++)LCD_WriteRAM(color);              //���ù��λ�� 	    
		}
}  
//��ָ�����������ָ����ɫ��			 
//(sx,sy),(ex,ey):�����ζԽ�����,�����СΪ:(ex-sx+1)*(ey-sy+1)   
//color:Ҫ������ɫ
void LCD_Color_Fill(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t color)
{  
	uint16_t height,width;
	uint16_t i,j;
	width=ex-sx+1; 			//�õ����Ŀ���
	height=ey-sy+1;			//�߶�
 	for(i=0;i<height;i++)
	{
 		LCD_SetCursor(sx,sy+i);   	//���ù��λ�� 
		LCD_WriteRAM_Prepare();     //��ʼд��GRAM
		for(j=0;j<width;j++)LCD_WriteRAM(color);//д������ 
	}  
}
//����
//x1,y1:�������
//x2,y2:�յ�����  
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	uint16_t t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	delta_x=x2-x1; //������������ 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //���õ������� 
	else if(delta_x==0)incx=0;//��ֱ�� 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//ˮƽ�� 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //ѡȡ�������������� 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//������� 
	{  
		LCD_DrawPoint(uRow,uCol);//���� 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
}    
//������	  
//(x1,y1),(x2,y2):���εĶԽ�����
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	LCD_DrawLine(x1,y1,x2,y1);
	LCD_DrawLine(x1,y1,x1,y2);
	LCD_DrawLine(x1,y2,x2,y2);
	LCD_DrawLine(x2,y1,x2,y2);
}
//��ָ��λ�û�һ��ָ����С��Բ
//(x,y):���ĵ�
//r    :�뾶
void LCD_Draw_Circle(uint16_t x0,uint16_t y0,uint8_t r)
{
	int a,b;
	int di;
	a=0;b=r;	  
	di=3-(r<<1);             //�ж��¸���λ�õı�־
	while(a<=b)
	{
		LCD_DrawPoint(x0+a,y0-b);             //5
 		LCD_DrawPoint(x0+b,y0-a);             //0           
		LCD_DrawPoint(x0+b,y0+a);             //4               
		LCD_DrawPoint(x0+a,y0+b);             //6 
		LCD_DrawPoint(x0-a,y0+b);             //1       
 		LCD_DrawPoint(x0-b,y0+a);             
		LCD_DrawPoint(x0-a,y0-b);             //2             
  	LCD_DrawPoint(x0-b,y0-a);             //7     	         
		a++;
		//ʹ��Bresenham�㷨��Բ     
		if(di<0)di +=4*a+6;	  
		else
		{
			di+=10+4*(a-b);   
			b--;
		} 						    
	}
} 									  
//��ָ��λ����ʾһ���ַ�
//x,y:��ʼ����
//num:Ҫ��ʾ���ַ�:" "--->"~"
//size:�����С 12/16/24
//mode:���ӷ�ʽ(1)���Ƿǵ��ӷ�ʽ(0)
void LCD_ShowChar(uint16_t x,uint16_t y,uint8_t num,uint8_t size,uint8_t mode)
{  							  
    uint8_t temp,t1,t;
	uint16_t y0=y;
	uint8_t csize=(size/8+((size%8)?1:0))*(size/2);		//�õ�����һ���ַ���Ӧ������ռ���ֽ���	
 	num=num-' ';//�õ�ƫ�ƺ��ֵ��ASCII�ֿ��Ǵӿո�ʼȡģ������-' '���Ƕ�Ӧ�ַ����ֿ⣩
	for(t=0;t<csize;t++)
	{   
		if(size==12)temp=asc2_1206[num][t]; 	 	//����1206����
		else if(size==16)temp=asc2_1608[num][t];	//����1608����
		else if(size==24)temp=asc2_2412[num][t];	//����2412����
		else return;								//û�е��ֿ�
		for(t1=0;t1<8;t1++)
		{			    
			if(temp&0x80)LCD_Fast_DrawPoint(x,y,POINT_COLOR);
			else if(mode==0)LCD_Fast_DrawPoint(x,y,BACK_COLOR);
			temp<<=1;
			y++;
			if(y>=lcddev.height)return;		//��������
			if((y-y0)==size)
			{
				y=y0;
				x++;
				if(x>=lcddev.width)return;	//��������
				break;
			}
		}  	 
	}  	    	   	 	  
}   
//m^n����
//����ֵ:m^n�η�.
uint32_t LCD_Pow(uint8_t m,uint8_t n)
{
	uint32_t result=1;	 
	while(n--)result*=m;    
	return result;
}			 
//��ʾ����,��λΪ0,����ʾ
//x,y :�������	 
//len :���ֵ�λ��
//size:�����С
//color:��ɫ 
//num:��ֵ(0~4294967295);	 
void LCD_ShowNum(uint16_t x,uint16_t y,uint32_t num,uint8_t len,uint8_t size)
{         	
	uint8_t t,temp;
	uint8_t enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				LCD_ShowChar(x+(size/2)*t,y,' ',size,0);
				continue;
			}else enshow=1; 
		 	 
		}
	 	LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,0); 
	}
} 
//��ʾ����,��λΪ0,������ʾ
//x,y:�������
//num:��ֵ(0~999999999);	 
//len:����(��Ҫ��ʾ��λ��)
//size:�����С
//mode:
//[7]:0,�����;1,���0.
//[6:1]:����
//[0]:0,�ǵ�����ʾ;1,������ʾ.
void LCD_ShowxNum(uint16_t x,uint16_t y,uint32_t num,uint8_t len,uint8_t size,uint8_t mode)
{  
	uint8_t t,temp;
	uint8_t enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/LCD_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				if(mode&0X80)LCD_ShowChar(x+(size/2)*t,y,'0',size,mode&0X01);  
				else LCD_ShowChar(x+(size/2)*t,y,' ',size,mode&0X01);  
 				continue;
			}else enshow=1; 
		 	 
		}
	 	LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,mode&0X01); 
	}
} 
//��ʾ�ַ���
//x,y:�������
//width,height:�����С  
//size:�����С
//*p:�ַ�����ʼ��ַ		  
void LCD_ShowString(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint8_t size,uint8_t *p)
{
	uint8_t x0=x;
	width+=x;
	height+=y;
    while((*p<='~')&&(*p>=' '))//�ж��ǲ��ǷǷ��ַ�!
    {       
        if(x>=width){x=x0;y+=size;}
        if(y>=height)break;//�˳�
        LCD_ShowChar(x,y,*p,size,0);
        x+=size/2;
        p++;
    }
}


//���� 16*16
void GUI_DrawFont16(uint16_t x, uint16_t y,uint8_t *s,uint8_t mode)
{
	uint8_t i,j;
	uint16_t k;
	uint16_t HZnum;
	uint16_t x0=x;
	HZnum=sizeof(tfont16)/sizeof(typFNT_GB16);	//�Զ�ͳ�ƺ�����Ŀ
	
			
	for (k=0;k<HZnum;k++) 
	{
	  if ((tfont16[k].Index[0]==*(s))&&(tfont16[k].Index[1]==*(s+1)))
	  { 	LCD_Set_Window(x,y,16,16);
				LCD_WriteRAM_Prepare();
		    for(i=0;i<16*2;i++)
		    {
				for(j=0;j<8;j++)
		    	{	
					if(!mode) //�ǵ��ӷ�ʽ
					{
						if(tfont16[k].Msk[i]&(0x80>>j))	LCD_WriteRAM(POINT_COLOR);
						else LCD_WriteRAM(BACK_COLOR);
					}
					else
					{
						//POINT_COLOR=fc;
						if(tfont16[k].Msk[i]&(0x80>>j))	LCD_DrawPoint(x,y);//��һ����
						x++;
						if((x-x0)==16)
						{
							x=x0;
							y++;
							break;
						}
					}

				}
				
			}
			
			
		}				  	
		continue;  //���ҵ���Ӧ�����ֿ������˳�����ֹ��������ظ�ȡģ����Ӱ��
	}

	LCD_Set_Window(0,0,lcddev.width,lcddev.height);//�ָ�����Ϊȫ��  
} 

//���� 24*24
void GUI_DrawFont24(uint16_t x, uint16_t y, uint8_t *s,uint8_t mode)
{
	uint8_t i,j;
	uint16_t k;
	uint16_t HZnum;
	uint16_t x0=x;
	HZnum=sizeof(tfont24)/sizeof(typFNT_GB24);	//�Զ�ͳ�ƺ�����Ŀ
		
			for (k=0;k<HZnum;k++) 
			{
			  if ((tfont24[k].Index[0]==*(s))&&(tfont24[k].Index[1]==*(s+1)))
			  { 	LCD_Set_Window(x,y,24,24);
						LCD_WriteRAM_Prepare();
				    for(i=0;i<24*3;i++)
				    {
							for(j=0;j<8;j++)
							{
								if(!mode) //�ǵ��ӷ�ʽ
								{
									if(tfont24[k].Msk[i]&(0x80>>j))	LCD_WriteRAM(POINT_COLOR);
									else LCD_WriteRAM(BACK_COLOR);
								}
							else
							{
								//POINT_COLOR=fc;
								if(tfont24[k].Msk[i]&(0x80>>j))	LCD_DrawPoint(x,y);//��һ����
								x++;
								if((x-x0)==24)
								{
									x=x0;
									y++;
									break;
								}
							}
						}
					}
					
					
				}				  	
				continue;  //���ҵ���Ӧ�����ֿ������˳�����ֹ��������ظ�ȡģ����Ӱ��
			}

	LCD_Set_Window(0,0,lcddev.width,lcddev.height);//�ָ�����Ϊȫ��  
}

//���� 32*32
void GUI_DrawFont32(uint16_t x, uint16_t y, uint8_t *s,uint8_t mode)
{
	uint8_t i,j;
	uint16_t k;
	uint16_t HZnum;
	uint16_t x0=x;
	HZnum=sizeof(tfont32)/sizeof(typFNT_GB32);	//�Զ�ͳ�ƺ�����Ŀ
	for (k=0;k<HZnum;k++) 
			{
			  if ((tfont32[k].Index[0]==*(s))&&(tfont32[k].Index[1]==*(s+1)))
			  { 	LCD_Set_Window(x,y,32,32);
						LCD_WriteRAM_Prepare();
				    for(i=0;i<32*4;i++)
				    {
						for(j=0;j<8;j++)
				    	{
							if(!mode) //�ǵ��ӷ�ʽ
							{
								if(tfont32[k].Msk[i]&(0x80>>j))	LCD_WriteRAM(POINT_COLOR);
								else LCD_WriteRAM(BACK_COLOR);
							}
							else
							{
								//POINT_COLOR=fc;
								if(tfont32[k].Msk[i]&(0x80>>j))	LCD_DrawPoint(x,y);//��һ����
								x++;
								if((x-x0)==32)
								{
									x=x0;
									y++;
									break;
								}
							}
						}
					}
					
					
				}				  	
				continue;  //���ҵ���Ӧ�����ֿ������˳�����ֹ��������ظ�ȡģ����Ӱ��
			}
	
	LCD_Set_Window(0,0,lcddev.width,lcddev.height);//�ָ�����Ϊȫ��  
} 



//��ʾ���ֻ����ַ���
void Show_Str(uint16_t x, uint16_t y,uint8_t *str,uint8_t size,uint8_t mode)
{			
	uint16_t x0=x;							  	  
  	uint8_t bHz=0;     //�ַ��������� 
    while(*str!=0)//����δ����
    { 
        if(!bHz)
        {
			if(x>(lcddev.width-size/2)||y>(lcddev.height-size)) 
			return; 
	        if(*str>0x80)bHz=1;//���� 
	        else              //�ַ�
	        {          
		        if(*str==0x0D)//���з���
		        {         
		            y+=size;
		            x=x0;
		            str++; 
		        }  
		        else
				{
					if(size>=24)//�ֿ���û�м���12X24 16X32��Ӣ������,��8X16����
					{ 						
					LCD_ShowChar(x,y,*str,24,mode);
					x+=12; //�ַ�,Ϊȫ�ֵ�һ�� 
					}
					else
					{
					LCD_ShowChar(x,y,*str,size,mode);
					x+=size/2; //�ַ�,Ϊȫ�ֵ�һ�� 
					}
				} 
				str++; 
		        
	        }
        }else//���� 
        {   
				if(x>(lcddev.width-size)||y>(lcddev.height-size)) 
				return;  
							bHz=0;//�к��ֿ�    
				if(size==32)
				GUI_DrawFont32(x,y,str,mode);	 	
				else if(size==24)
				GUI_DrawFont24(x,y,str,mode);	
				else
				GUI_DrawFont16(x,y,str,mode);
					
						str+=2; 
						x+=size;//��һ������ƫ��	    
        }						 
    }   
}


//��ʾ40*40ͼƬ
void Gui_Drawbmp16(uint16_t x,uint16_t y,const unsigned char *p) //��ʾ40*40ͼƬ
{
  	int i; 
	unsigned char picH,picL; 
	LCD_Set_Window(x,y,40,40);
	LCD_WriteRAM_Prepare();	
	
    for(i=0;i<40*40;i++)
	{	
	 	picL=*(p+i*2);	//���ݵ�λ��ǰ
		picH=*(p+i*2+1);				
		LCD_WriteRAM(picH<<8|picL);  						
	}	
	LCD_Set_Window(0,0,lcddev.width,lcddev.height);//�ָ���ʾ����Ϊȫ��	

}

//������ʾ
void Gui_StrCenter(uint16_t x, uint16_t y, uint8_t *str,uint8_t size,uint8_t mode)
{
	uint16_t x1;
	uint16_t len=strlen((const char *)str);
	if(size>16)
	{
		x1=(lcddev.width-len*(size/2))/2;
	}else
	{
		x1=(lcddev.width-len*8)/2;
	}
	
	Show_Str(x+x1,y,str,size,mode);
} 


void Load_Drow_Dialog(void)
{
	LCD_Clear(WHITE);//����   
 	POINT_COLOR=BLUE;//��������Ϊ��ɫ
	BACK_COLOR=WHITE;
	LCD_ShowString(lcddev.width-24,0,200,16,16,(uint8_t *)"RST");//��ʾ��������
  POINT_COLOR=RED;//���û�����ɫ 
}
////////////////////////////////////////////////////////////////////////////////
//���ݴ�����ר�в���
//��ˮƽ��
//x0,y0:����
//len:�߳���
//color:��ɫ
void gui_draw_hline(uint16_t x0,uint16_t y0,uint16_t len,uint16_t color)
{
	if(len==0)return;
	LCD_Fill(x0,y0,x0+len-1,y0,color);	
}
//��ʵ��Բ
//x0,y0:����
//r:�뾶
//color:��ɫ
void gui_fill_circle(uint16_t x0,uint16_t y0,uint16_t r,uint16_t color)
{											  
	uint32_t i;
	uint32_t imax = ((uint32_t)r*707)/1000+1;
	uint32_t sqmax = (uint32_t)r*(uint32_t)r+(uint32_t)r/2;
	uint32_t x=r;
	gui_draw_hline(x0-r,y0,2*r,color);
	for (i=1;i<=imax;i++) 
	{
		if ((i*i+x*x)>sqmax)// draw lines from outside  
		{
 			if (x>imax) 
			{
				gui_draw_hline (x0-i+1,y0+x,2*(i-1),color);
				gui_draw_hline (x0-i+1,y0-x,2*(i-1),color);
			}
			x--;
		}
		// draw lines from inside (center)  
		gui_draw_hline(x0-x,y0+i,2*x,color);
		gui_draw_hline(x0-x,y0-i,2*x,color);
	}
}  

//��һ������
//(x1,y1),(x2,y2):��������ʼ����
//size�������Ĵ�ϸ�̶�
//color����������ɫ
void lcd_draw_bline(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint8_t size,uint16_t color)
{
	uint16_t t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	if(x1<size|| x2<size||y1<size|| y2<size)return; 
	delta_x=x2-x1; //������������ 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //���õ������� 
	else if(delta_x==0)incx=0;//��ֱ�� 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//ˮƽ�� 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //ѡȡ�������������� 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//������� 
	{  
		gui_fill_circle(uRow,uCol,size,color);//���� 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
}































