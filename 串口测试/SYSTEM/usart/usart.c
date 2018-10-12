#include "sys.h"
#include "usart.h"	  
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos 使用	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//串口1初始化		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/8/18
//版本：V1.5
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
//********************************************************************************
//V1.3修改说明 
//支持适应不同频率下的串口波特率设置.
//加入了对printf的支持
//增加了串口接收命令功能.
//修正了printf第一个字符丢失的bug
//V1.4修改说明
//1,修改串口初始化IO的bug
//2,修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
//3,增加了USART_REC_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
//4,修改了EN_USART1_RX的使能方式
//V1.5修改说明
//1,增加了对UCOSII的支持
////////////////////////////////////////////////////////////////////////////////// 	  
 

//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART3->SR&0X40)==0);//循环发送,直到发送完毕   
    USART3->DR = (u8) ch;      
	return ch;
}
#endif 

/*使用microLib的方法*/
 /* 
int fputc(int ch, FILE *f)
{
	USART_SendData(USART1, (uint8_t) ch);

	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}
int GetKey (void)  { 

    while (!(USART1->SR & USART_FLAG_RXNE));

    return ((int)(USART1->DR & 0x1FF));
}
*/
 
#if EN_USART3_RX   //如果使能了接收
//串口3中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
u8 USART_TX_BUF[50];
u16 USART_TX_LEN;
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	  
u32 error_cnt=0; //接收错误计数
u32 valid_cnt=0; //接收正确计数
u8 sum_check=0;
u8 command_type = 0;	
u8 receive_finish = FALSE;
u8 data_valid = FALSE;
u8 data_len = 0;
u8 show_len = 0;
u32 systick_counter = 0;
u32 time_start = 0;

  
void uart_init(u32 bound){
  //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//使能GPIOB时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);	//使能USART3时钟
  
	//USART3_TX   GPIOB.10
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PA.9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
  GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOA.9
   
  //USART3_RX	  GPIOB.11初始化
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;//PA10
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
  GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOA.10  
  
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //PA.9
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//复用推挽输出
  	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOB.5

  //Usart1 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
   //USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

  USART_Init(USART3, &USART_InitStructure); //初始化串口1
  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启串口接受中断
  USART_Cmd(USART3, ENABLE);                    //使能串口1 

}

void USART3_IRQHandler(void)                	//串口1中断服务程序
	{
	u8 Res;
	char a[] = "hello\r\n";
	int i;
	int size;
	int index;	
	
#if SYSTEM_SUPPORT_OS 		//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
		Res =USART_ReceiveData(USART3);	//读取接收到的数据

		if((USART_RX_STA&0x8000)==0) 	//没有接收到帧头
		{
			if(Res == 0xEB)			//接收到帧头
			{
				    	 
				time_start = SysTick->VAL;
				USART_RX_STA|=0x8000;	//接收到帧头
			}
		}
		else			//已经接收到帧头
		{
			if((USART_RX_STA&0x4000)==0)	//没接收到帧头第二
			{
				if(Res == 0x90)
				{
					USART_RX_STA|=0x4000;
				}
				else
				{
					USART_RX_STA = 0;//数据错误，清空标志位，重新开始接收
					data_len = 0;
					command_type = 0;
					receive_finish = FALSE;
					error_cnt++;
				}
			}
			else		//接收除帧头外的数据
			{
				USART_RX_BUF[USART_RX_STA&0X3FFF]=Res;

				if((USART_RX_STA&0X3FFF)==0)		//数据类型码
				{
					command_type = Res;
				}
				if((USART_RX_STA&0X3FFF)==1)		//数据长度
				{
					data_len = Res;
					show_len = data_len;
				}
				if((USART_RX_STA&0X3FFF)>data_len+1)
				{
					receive_finish = TRUE;
					sum_check = sum(&USART_RX_BUF[0],data_len+2);
					
					if(sum_check == USART_RX_BUF[data_len+2])
					{
						data_valid = TRUE;
					}
					else
					{
						data_valid = FALSE;
					}
				}
				
				USART_RX_STA++;
				
				
			}
		}

		if(receive_finish == TRUE)
		{
			if(data_valid == TRUE)
			{
				data_process(data_valid, command_type, &USART_TX_BUF[0], &USART_TX_LEN);
				usart_tx_feedback(&USART_TX_BUF[0], USART_TX_LEN);
				USART_RX_STA = 0;//数据错误，清空标志位，重新开始接收
				data_len = 0;
				command_type = 0;
				receive_finish = FALSE;
				data_valid = FALSE;
				valid_cnt++;
			}
			else
			{
				data_process(data_valid, command_type, &USART_TX_BUF[0], &USART_TX_LEN);
				usart_tx_feedback(&USART_TX_BUF[0], USART_TX_LEN);
				USART_RX_STA = 0;//数据错误，清空标志位，重新开始接收
				data_len = 0;
				command_type = 0;
				receive_finish = FALSE;
				data_valid = FALSE;
				error_cnt++;
			}
			
			systick_counter = time_start -(SysTick->VAL);
		
		}
		
     } 
#if SYSTEM_SUPPORT_OS 	//如果SYSTEM_SUPPORT_OS为真，则需要支持OS.
	OSIntExit();  											 
#endif
} 
#endif	

u8 sum(u8 *data, u8 len)
{
	u8 result =  0;
	u32 i = 0;

	for(i=0;i<len;i++)
	{
		result += data[i];
	}
	
	return result;
}

void usart_tx_feedback(const u8 *data_back, const u16 len_data_back)
{
	u16 i;
	for(i=0;i<len_data_back;i++)
	{
		USART_SendData(USART3,data_back[i]);
		while(USART_GetFlagStatus(USART3,USART_FLAG_TC)!=SET)
			;
	}
}


void data_process(u8 valid_flag, u8 command_type, u8 *data_back, u16 *len_data_back)
{
	u32 i;
	u8 tmp_checksum=0;
	
	switch(command_type)
	{
		case 0x67:
		case 0x68:
		case 0x69:
		case 0x6A:
		case 0x6B:
		case 0x6C:
		case 0x6D:
		case 0x6E:
		case 0x6F:
		case 0x70:
		case 0x71:
			if(valid_flag==TRUE)
			{
				data_back[0] = 0xEB;
				data_back[1] = 0x90;
				data_back[2] = 0xFF-command_type;
				data_back[3] = 1;
				data_back[4] = 0xAA;
			}
			else
			{
				data_back[0] = 0xEB;
				data_back[1] = 0x90;
				data_back[2] = 0xFF-command_type;
				data_back[3] = 1;
				data_back[4] = 0x55;
			}
			for(i = 2;i<5;i++)
				tmp_checksum += data_back[i];
			
			data_back[5] = tmp_checksum;
			*len_data_back = 6;
			break;
			
		//遥测数据
		case 0x18:
			data_back[0] = 0xEB;
			data_back[1] = 0x90;
			data_back[2] = 0xFF-command_type;
			data_back[3] = 21;		// 21个字节

			for(i=0;i<21;i++)
			{
				data_back[4+i] = 0x00;
			}

			for(i = 0;i<23;i++)
				tmp_checksum += data_back[2+i];
			
			data_back[25] = tmp_checksum;

			*len_data_back = 26;

			break;
			
		case 0x17:
			data_back[0] = 0xEB;
			data_back[1] = 0x90;
			data_back[2] = 0xFF-command_type;
			data_back[3] = 21;		// 21个字节
			data_back[4] = 0x00;
			data_back[5] = 0x00;
			data_back[6] = 0x00;
			data_back[7] = 0xF8;
			data_back[8] = 0xF8;
			data_back[9] = 0xF8;
			data_back[10] = 0xF8;
			data_back[11] = 0x00;
			data_back[12] = 0x00;
			data_back[13] = 0x00;
			data_back[14] = 0x00;
			data_back[15] = 0x00;
			data_back[16] = 0x00;
			data_back[17] = 0x00;
			data_back[18] = 0x00;
			data_back[19] = 0x00;
			data_back[20] = 0x00;
			data_back[21] = 0x00;
			data_back[22] = 0x00;
			data_back[23] = 0x00;
			data_back[24] = 0x00;
			
			for(i = 2;i<25;i++)
				tmp_checksum += data_back[i];
			
			data_back[25] = tmp_checksum;
			*len_data_back = 26;

			break;
			
		case 0x16:
			data_back[0] = 0xEB;
			data_back[1] = 0x90;
			data_back[2] = 0xFF-command_type;
			data_back[3] = 21;		// 21个字节
			data_back[4] = 0x00;
			data_back[5] = 0x00;
			data_back[6] = 0x00;
			data_back[7] = 0x00;
			data_back[8] = 0x00;
			data_back[9] = 0x00;
			data_back[10] = 0x00;
			data_back[11] = 0x00;
			data_back[12] = 0xF8;
			data_back[13] = 0xF8;
			data_back[14] = 0xF8;
			data_back[15] = 0xF8;
			data_back[16] = 0xF9;
			data_back[17] = 0xF8;
			data_back[18] = 0xF8;
			data_back[19] = 0xF8;
			data_back[20] = 0xF9;
			data_back[21] = 0xF8;
			data_back[22] = 0xF8;
			data_back[23] = 0xF9;
			data_back[24] = 0xF8;
			
			for(i = 2;i<25;i++)
				tmp_checksum += data_back[i];
			
			data_back[25] = tmp_checksum;
			*len_data_back = 26;

			break;
			
		case 0x15:
			data_back[0] = 0xEB;
			data_back[1] = 0x90;
			data_back[2] = 0xFF-command_type;
			data_back[3] = 21;		// 21个字节
			data_back[4] = 0xF9;
			data_back[5] = 0x00;
			data_back[6] = 0x00;
			data_back[7] = 0x00;
			data_back[8] = 0x00;
			data_back[9] = 0x00;
			data_back[10] = 0x00;
			data_back[11] = 0x00;
			data_back[12] = 0x00;
			data_back[13] = 0x00;
			data_back[14] = 0x00;
			data_back[15] = 0x00;
			data_back[16] = 0x00;
			data_back[17] = 0x00;
			data_back[18] = 0x00;
			data_back[19] = 0x00;
			data_back[20] = 0x00;
			data_back[21] = 0x00;
			data_back[22] = 0x00;
			data_back[23] = 0x00;
			data_back[24] = 0x00;
			
			for(i = 2;i<25;i++)
				tmp_checksum += data_back[i];
			
			data_back[25] = tmp_checksum;
			*len_data_back = 26;

			break;
			
		case 0x14:
			data_back[0] = 0xEB;
			data_back[1] = 0x90;
			data_back[2] = 0xFF-command_type;
			data_back[3] = 21;		// 21个字节
			data_back[4] = 0x00;
			data_back[5] = 0x00;
			data_back[6] = 0x00;
			data_back[7] = 0x00;
			data_back[8] = 0xF9;
			data_back[9] = 0xF9;
			data_back[10] = 0xF8;
			data_back[11] = 0xF8;
			data_back[12] = 0xF8;
			data_back[13] = 0xF9;
			data_back[14] = 0x00;
			data_back[15] = 0xAA;
			data_back[16] = 0x00;
			data_back[17] = 0x10;
			data_back[18] = 0x55;
			data_back[19] = 0x00;
			data_back[20] = 0xAA;
			data_back[21] = 0xAA;
			data_back[22] = 0xAA;
			data_back[23] = 0x00;
			data_back[24] = 0xE0;
			
			for(i = 2;i<25;i++)
				tmp_checksum += data_back[i];
			
			data_back[25] = tmp_checksum;
			*len_data_back = 26;

			break;
			
		case 0x13:
			data_back[0] = 0xEB;
			data_back[1] = 0x90;
			data_back[2] = 0xFF-command_type;
			data_back[3] = 21;		// 21个字节
			data_back[4] = 0x00;
			data_back[5] = 0x00;
			data_back[6] = 0x84;
			data_back[7] = 0x84;
			data_back[8] = 0x30;
			data_back[9] = 0x00;
			data_back[10] = 0x00;
			data_back[11] = 0x75;
			data_back[12] = 0x75;
			data_back[13] = 0x68;
			data_back[14] = 0x73;
			data_back[15] = 0x4A;
			data_back[16] = 0x78;
			data_back[17] = 0x00;
			data_back[18] = 0x00;
			data_back[19] = 0x00;
			data_back[20] = 0x00;
			data_back[21] = 0x00;
			data_back[22] = 0x00;
			data_back[23] = 0x00;
			data_back[24] = 0x00;
			
			for(i = 2;i<25;i++)
				tmp_checksum += data_back[i];
			
			data_back[25] = tmp_checksum;
			*len_data_back = 26;

			break;

		default:
				data_back[0] = 0xEB;
				data_back[1] = 0x90;
				data_back[2] = 0xFF;
				data_back[3] = 1;
				data_back[4] = 0x55;
				for(i = 2;i<5;i++)
				tmp_checksum += data_back[i];
			
				data_back[5] = tmp_checksum;
				*len_data_back = 6;
			break;
			
	}
}

