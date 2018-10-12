#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"

 
/************************************************
 ʵ��0��ALIENTEK STM32F103�����幤��ģ��
 ע�⣬�����ֲ��еĵ����½�ʹ�õ�main�ļ�
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/

int main(void)
 {		
 	u8 t=0;
	 u8 x=0;
	u8 lcd_id[12];			//���LCD ID�ַ���
	u8 tmp_show[100];
	u8 show_check[15];
	u8 time_interval_show[26];
	u32 time_in_us;
	u8 clear_flag = 1, clear_flag2 = 1;
	u32 i;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	delay_init();	    	 //��ʱ������ʼ��	  
	uart_init(115200);	 //���ڳ�ʼ��Ϊ115200
	LCD_Init();
	POINT_COLOR=BLACK;
	LCD_Clear(GRAY);
	sprintf((char*)lcd_id,"LCD ID:%04X",lcddev.id);//��LCD ID��ӡ��lcd_id���顣				 	
  	
  while(1)
	{
		//printf("t:%d\r\n",t);
	/*	char a[] = "hello\r\n";
		int i;
		int size;
		size = sizeof(a);
		for(i=0;i<size;i++)
		{
			USART_SendData(USART3,a[i]);
			while(USART_GetFlagStatus(USART3,USART_FLAG_TC)!=SET)
				;
		}
		USART_SendData(USART3,t);
		while(USART_GetFlagStatus(USART3,USART_FLAG_TC)!=SET)
				;
	*/
		if(show_len > 6)
		{
			clear_flag = 1;
		}
		else
		{
			if(clear_flag == 1)
			{
				LCD_Clear(GRAY);
				clear_flag = 0;
			}
		}

		if(USART_TX_LEN > 10)
		{
			clear_flag2 = 1;
		}
		else
		{
			if(clear_flag2 == 1)
			{
				LCD_Clear(GRAY);
				clear_flag2 = 0;
			}
		}
		
		
		LCD_ShowString(30,40,200,16,16,"HLY Simulation"); 
		LCD_ShowString(30,60,200,16,16,"RS422 Communication Test"); 
		LCD_ShowString(30,80,200,16,16,"RXB TEST");
		LCD_ShowString(30,100,200,16,16,"MYL@THU");
 		//LCD_ShowString(30,120,200,16,16,lcd_id);		//��ʾLCD ID
		LCD_ShowString(30,120,200,12,12,"2018/10/9");	  

		sprintf((char*)tmp_show,"valid data count:%d",valid_cnt);
		LCD_ShowString(30,160,200,16,16,tmp_show);

		sprintf((char*)tmp_show,"error data count:%d",error_cnt);
		LCD_ShowString(30,180,200,16,16,tmp_show);

		LCD_ShowString(30,220,200,16,16,"Rx raw data");
		if(show_len > 6)
		{
			for(i=0;i<sizeof(tmp_show);i++)
			{
				tmp_show[i] = 0;
			}
			for(i=0;i<(show_len+2)/2;i++)
			{
				sprintf(&tmp_show[3*i],"%02X ",USART_RX_BUF[i]);
			}
			LCD_ShowString(30,240,200,16,16,tmp_show);
			for(i=0;i<(show_len+2)/2;i++)
			{
				sprintf(&tmp_show[3*i],"%02X ",USART_RX_BUF[(show_len+2)/2+i]);
			}
			LCD_ShowString(30,260,200,16,16,tmp_show);
			sprintf((char*)show_check,"checksum is %02X",sum_check);
			LCD_ShowString(30,280,200,16,16,show_check);
		}
		else
		{
			
			for(i=0;i<sizeof(tmp_show);i++)
			{
				tmp_show[i] = 0;
			}
			
			for(i=0;i<show_len+2;i++)
			{
				sprintf(&tmp_show[3*i],"%02X ",USART_RX_BUF[i]);
			}
			LCD_ShowString(30,240,200,16,16,tmp_show);
			sprintf((char*)show_check,"checksum is %02X",sum_check);
			LCD_ShowString(30,280,200,16,16,show_check);
		}

		//��ʾ�����ֽ� 

		LCD_ShowString(30,320,200,16,16,"Tx raw data");

		if(USART_TX_LEN>10)
		{
			for(i=0;i<sizeof(tmp_show);i++)
			{
				tmp_show[i] = 0;
			}
			//��һ������
			for(i=0;i<8;i++)
			{
				sprintf(&tmp_show[3*i],"%02X ",USART_TX_BUF[i]);
			}
			LCD_ShowString(30,340,220,16,16,tmp_show);
			//�ڶ�������
			for(i=0;i<9;i++)
			{
				sprintf(&tmp_show[3*i],"%02X ",USART_TX_BUF[i+8]);
			}
			LCD_ShowString(30,360,220,16,16,tmp_show);
			//����������
			for(i=0;i<9;i++)
			{
				sprintf(&tmp_show[3*i],"%02X ",USART_TX_BUF[i+17]);
			}
			LCD_ShowString(30,380,220,16,16,tmp_show);
			
		}
		else
		{
			for(i=0;i<USART_TX_LEN;i++)
			{
				sprintf(&tmp_show[3*i],"%02X ",USART_TX_BUF[i]);
			}
			LCD_ShowString(30,340,220,16,16,tmp_show);
		}

		time_in_us = systick_counter/9;
		
		sprintf((char*)time_interval_show,"time_interval is %6dus",time_in_us);
		LCD_ShowString(30,420,220,16,16,time_interval_show);
		
		if((t-t/2*2)==1)
			GPIO_SetBits(GPIOB,GPIO_Pin_5);
		else
			GPIO_ResetBits(GPIOB,GPIO_Pin_5);
		delay_ms(500);
		t++;
	}	 
} 

