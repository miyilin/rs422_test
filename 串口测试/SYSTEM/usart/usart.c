#include "sys.h"
#include "usart.h"	  
////////////////////////////////////////////////////////////////////////////////// 	 
//���ʹ��ucos,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos ʹ��	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//����1��ʼ��		   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/8/18
//�汾��V1.5
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved
//********************************************************************************
//V1.3�޸�˵�� 
//֧����Ӧ��ͬƵ���µĴ��ڲ���������.
//�����˶�printf��֧��
//�����˴��ڽ��������.
//������printf��һ���ַ���ʧ��bug
//V1.4�޸�˵��
//1,�޸Ĵ��ڳ�ʼ��IO��bug
//2,�޸���USART_RX_STA,ʹ�ô����������ֽ���Ϊ2��14�η�
//3,������USART_REC_LEN,���ڶ��崮�����������յ��ֽ���(������2��14�η�)
//4,�޸���EN_USART1_RX��ʹ�ܷ�ʽ
//V1.5�޸�˵��
//1,�����˶�UCOSII��֧��
////////////////////////////////////////////////////////////////////////////////// 	  
 

//////////////////////////////////////////////////////////////////
//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
_sys_exit(int x) 
{ 
	x = x; 
} 
//�ض���fputc���� 
int fputc(int ch, FILE *f)
{      
	while((USART3->SR&0X40)==0);//ѭ������,ֱ���������   
    USART3->DR = (u8) ch;      
	return ch;
}
#endif 

/*ʹ��microLib�ķ���*/
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
 
#if EN_USART3_RX   //���ʹ���˽���
//����3�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 USART_RX_BUF[USART_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
u8 USART_TX_BUF[50];
u16 USART_TX_LEN;
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART_RX_STA=0;       //����״̬���	  
u32 error_cnt=0; //���մ������
u32 valid_cnt=0; //������ȷ����
u8 sum_check=0;
u8 command_type = 0;	
u8 receive_finish = FALSE;
u8 data_valid = FALSE;
u8 data_len = 0;
u8 show_len = 0;
u32 systick_counter = 0;
u32 time_start = 0;

  
void uart_init(u32 bound){
  //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//ʹ��GPIOBʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);	//ʹ��USART3ʱ��
  
	//USART3_TX   GPIOB.10
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PA.9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
  GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��GPIOA.9
   
  //USART3_RX	  GPIOB.11��ʼ��
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;//PA10
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
  GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��GPIOA.10  
  
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //PA.9
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	//�����������
  	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��GPIOB.5

  //Usart1 NVIC ����
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
   //USART ��ʼ������

	USART_InitStructure.USART_BaudRate = bound;//���ڲ�����
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

  USART_Init(USART3, &USART_InitStructure); //��ʼ������1
  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//�������ڽ����ж�
  USART_Cmd(USART3, ENABLE);                    //ʹ�ܴ���1 

}

void USART3_IRQHandler(void)                	//����1�жϷ������
	{
	u8 Res;
	char a[] = "hello\r\n";
	int i;
	int size;
	int index;	
	
#if SYSTEM_SUPPORT_OS 		//���SYSTEM_SUPPORT_OSΪ�棬����Ҫ֧��OS.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
	{
		Res =USART_ReceiveData(USART3);	//��ȡ���յ�������

		if((USART_RX_STA&0x8000)==0) 	//û�н��յ�֡ͷ
		{
			if(Res == 0xEB)			//���յ�֡ͷ
			{
				    	 
				time_start = SysTick->VAL;
				USART_RX_STA|=0x8000;	//���յ�֡ͷ
			}
		}
		else			//�Ѿ����յ�֡ͷ
		{
			if((USART_RX_STA&0x4000)==0)	//û���յ�֡ͷ�ڶ�
			{
				if(Res == 0x90)
				{
					USART_RX_STA|=0x4000;
				}
				else
				{
					USART_RX_STA = 0;//���ݴ�����ձ�־λ�����¿�ʼ����
					data_len = 0;
					command_type = 0;
					receive_finish = FALSE;
					error_cnt++;
				}
			}
			else		//���ճ�֡ͷ�������
			{
				USART_RX_BUF[USART_RX_STA&0X3FFF]=Res;

				if((USART_RX_STA&0X3FFF)==0)		//����������
				{
					command_type = Res;
				}
				if((USART_RX_STA&0X3FFF)==1)		//���ݳ���
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
				USART_RX_STA = 0;//���ݴ�����ձ�־λ�����¿�ʼ����
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
				USART_RX_STA = 0;//���ݴ�����ձ�־λ�����¿�ʼ����
				data_len = 0;
				command_type = 0;
				receive_finish = FALSE;
				data_valid = FALSE;
				error_cnt++;
			}
			
			systick_counter = time_start -(SysTick->VAL);
		
		}
		
     } 
#if SYSTEM_SUPPORT_OS 	//���SYSTEM_SUPPORT_OSΪ�棬����Ҫ֧��OS.
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
			
		//ң������
		case 0x18:
			data_back[0] = 0xEB;
			data_back[1] = 0x90;
			data_back[2] = 0xFF-command_type;
			data_back[3] = 21;		// 21���ֽ�

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
			data_back[3] = 21;		// 21���ֽ�
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
			data_back[3] = 21;		// 21���ֽ�
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
			data_back[3] = 21;		// 21���ֽ�
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
			data_back[3] = 21;		// 21���ֽ�
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
			data_back[3] = 21;		// 21���ֽ�
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

