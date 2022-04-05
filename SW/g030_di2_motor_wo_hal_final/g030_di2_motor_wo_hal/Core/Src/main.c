
#include "main.h"

unsigned char pos_table[8] = {0,1,3,2,5,6,4,0};
unsigned int CCR_Table[8] = {0,0xFFFF00,0x00FF00,0x00FFFF,0x0000FF,0xFF00FF,0xFF0000,0};
unsigned short tx_pos = 1256;
unsigned char tx_div = 15;
unsigned char rx_div = 32;
short rx_pos,pos_enc;
unsigned short tim3_cnt_tmp;

HallTypeDef Hall;
ModeTypeDef MODE;
__align(4) unsigned char rx_buf[16];
__align(4) unsigned char tx_buf[4]={0x13,0x57,0x9B,0xDF};
__align(4) unsigned char rx_buf_tmp[4];

int main(void){
	Init_HW();
	main_while_sram();
}

void send_pos(void)__attribute((section("sram")));
unsigned char tx_cmd=0x41;
void send_pos(void){
	tx_buf[0]=tx_cmd;
	tx_buf[1]=(tx_div<<3)|((tx_pos>>8)&0x7);
	tx_buf[2]=tx_pos&0xFF;
	Send_TX();
};
unsigned char flag;
void main_while_sram(void){
	unsigned char hall_in;
	unsigned short rx_cnt,rx_cnt_next,tmp_pos;
	rx_cnt = DMA_RX->CNDTR;
	rx_cnt_next = ((rx_cnt-5)&0xF)+1;
	TIM14->CNT = 0;
	TIM14->CR1 = 0;
	MOTOR_OFF();
	MODE = MODE_STOP;
	while(!(PWR->CR1&PWR_CR1_FPD_LPRUN)){PWR->CR1 |= PWR_CR1_FPD_LPRUN;__NOP();__NOP();__NOP();}
	while(!(PWR->CR1&PWR_CR1_LPR)){PWR->CR1 |= PWR_CR1_LPR;__NOP();__NOP();__NOP();}
  while (1){		
		if(flag){
			flag = 0;
			send_pos();
		}
		pos_enc = TIM3->CNT;
		if (MODE==MODE_RUN){	
			if(hall_in != (GPIOA->IDR&0xE0)){
				hall_in = GPIOA->IDR&0xE0;
				TIM17->CNT = 0;
				unsigned int CCR_OUT;
				unsigned char hall_pos = pos_table[hall_in>>5];
				if ((pos_enc - rx_pos < 13)&( rx_pos - pos_enc < 13)){
					if((hall_pos<1)|(hall_pos>6))CCR_OUT = 0;
					else CCR_OUT = CCR_Table[hall_pos];
					TIM17->CNT = 45000;	
					TIM1->CCR1 = CCR_OUT & (rx_div >> 1);
					TIM1->CCR3 = (CCR_OUT>>8) & (rx_div >> 1);
					TIM1->CCR4 = (CCR_OUT>>16) & (rx_div >> 1);
				}				
				else if (pos_enc < rx_pos){		
					if (rx_pos - pos_enc > 120){
						if((hall_pos<1)|(hall_pos>6))CCR_OUT = 0;
						else if(hall_pos>2) CCR_OUT = CCR_Table[hall_pos-2];
						else CCR_OUT = CCR_Table[hall_pos+4];					
						TIM1->CCR1 = CCR_OUT & rx_div;
						TIM1->CCR3 = (CCR_OUT>>8) & rx_div;
						TIM1->CCR4 = (CCR_OUT>>16) & rx_div;
					}
					else{
						if((hall_pos<1)|(hall_pos>6))CCR_OUT = 0;
						else if(hall_pos>1) CCR_OUT = CCR_Table[hall_pos-1];
						else CCR_OUT = CCR_Table[6];					
						TIM1->CCR1 = CCR_OUT & (rx_div >> 1);
						TIM1->CCR3 = (CCR_OUT>>8) & (rx_div >> 1);
						TIM1->CCR4 = (CCR_OUT>>16) & (rx_div >> 1);
					}
				}
				else{
					if ( pos_enc - rx_pos  > 120){
						if((hall_pos<1)|(hall_pos>6))CCR_OUT = 0;
						else if(hall_pos<5) CCR_OUT = CCR_Table[hall_pos+2];
						else CCR_OUT = CCR_Table[hall_pos-4];					
						TIM1->CCR1 = CCR_OUT & rx_div;
						TIM1->CCR3 = (CCR_OUT>>8) & rx_div;
						TIM1->CCR4 = (CCR_OUT>>16) & rx_div;
					}
					else{
						if((hall_pos<1)|(hall_pos>6))CCR_OUT = 0;
						else if(hall_pos<6) CCR_OUT = CCR_Table[hall_pos+1];
						else CCR_OUT = CCR_Table[1];					
						TIM1->CCR1 = CCR_OUT & (rx_div >> 1);
						TIM1->CCR3 = (CCR_OUT>>8) & (rx_div >> 1);
						TIM1->CCR4 = (CCR_OUT>>16) & (rx_div >> 1);
					}
				}
			}		
		}
		if(TIM17->CNT > 60000){
			MOTOR_OFF();
			MODE = MODE_STOP;
			if ((pos_enc - rx_pos < 15)&( rx_pos - pos_enc < 15)){
				tx_buf[0]=0xC2;
			}
			else{
				tx_buf[0]=0xC3;
			}
			tx_buf[1]=(rx_div<<3)|((pos_enc>>10)&0x7);
			tx_buf[2]= (pos_enc>>2)&0xFF;
			Send_TX();
		}
		if(TIM14->CNT > 5000){
			TIM14->CR1 = 0;
			TIM14->CNT = 0;
			rx_cnt = DMA_RX->CNDTR;
			rx_cnt_next = ((rx_cnt-5)&0xF)+1;
		}
		if(MODE==MODE_STOP){
			if(TIM16->CNT > 57600){
				tim3_cnt_tmp = TIM3->CNT;
				TIM16->CR1 = 0;
				TIM16->CNT = 0;
				Hall = Hall_OFF;
				GPIOA->BSRR = GPIO_BSRR_BR4;
			}
		}
		if(rx_cnt != DMA_RX->CNDTR){
			if(TIM14->CR1){
				if(rx_cnt_next==DMA_RX->CNDTR){
					unsigned char i = 16 - rx_cnt;
					rx_buf_tmp[0] = rx_buf[i];
					rx_buf_tmp[1] = rx_buf[(i + 1) & 0xF];
					rx_buf_tmp[2] = rx_buf[(i + 2) & 0xF];
					rx_buf_tmp[3] = rx_buf[(i + 3) & 0xF]; 
					CRC->CR |= CRC_CR_RESET;
					CRC->DR = (*(unsigned int *)&rx_buf_tmp)&0xFFFFFF;
					__NOP();
					if(rx_buf_tmp[3] == ((CRC->DR)&0xFF)){					
						TIM14->CR1 = 0;
						TIM14->CNT = 0;
						rx_cnt = DMA_RX->CNDTR;
						rx_cnt_next = ((rx_cnt-5)&0xF)+1;	
						switch (rx_buf_tmp[0]){
							case 0x41:
								rx_div = rx_buf_tmp[1]>>3;
								tmp_pos = (((rx_buf_tmp[1] << 8) & 0x700) | rx_buf_tmp[2])<<2;										
								if(tmp_pos != pos_enc){
									rx_pos = tmp_pos;
									TIM16->CNT = 1;
									MOTOR_ON();
									if(Hall==Hall_OFF){
										GPIOA->BSRR = GPIO_BSRR_BS4;
										Hall = Hall_ON;
										TIM16->CR1 = TIM_CR1_CEN;
										__NOP(); __NOP(); __NOP(); __NOP();
										TIM3->CNT = tim3_cnt_tmp;
									}
									tx_buf[0]=0xC1;
									tx_buf[1]=rx_buf_tmp[1];
									tx_buf[2]=rx_buf_tmp[2];
									Send_TX();
								}
								else{
									tx_buf[0]=0xC2;
									tx_buf[1]=rx_buf_tmp[1];
									tx_buf[2]=rx_buf_tmp[2];
									Send_TX();
								}
							break;
							case 0x44:
								rx_div = rx_buf_tmp[1]>>3;
								TIM3->CNT = (rx_pos = (((rx_buf_tmp[1] << 8) & 0x700) | rx_buf_tmp[2])<<2);
								MODE = MODE_STOP;
								MOTOR_OFF();
								tx_buf[0]=0xC4;
								tx_buf[1]=rx_buf_tmp[1];
								tx_buf[2]=rx_buf_tmp[2];
								Send_TX();
							break;
							case 0x45:
								tx_buf[0]=0xC5;
								tx_buf[1]=(rx_div<<3)|((pos_enc>>10)&0x7);
								tx_buf[2]= (pos_enc>>2)&0xFF;
								Send_TX();
							break;
						}
					};		
				}
			}
			else{
				TIM14->CR1 = TIM_CR1_CEN | TIM_CR1_OPM;
			}
		}
	}
}


void Init_HW(void){	
	RCC->APBENR2 = RCC_APBENR2_TIM1EN | RCC_APBENR2_TIM17EN | RCC_APBENR2_TIM14EN | RCC_APBENR2_TIM16EN;//RCC_APBENR2_SYSCFGEN	
	RCC->APBENR1 = RCC_APBENR1_PWREN | RCC_APBENR1_DBGEN | RCC_APBENR1_USART2EN | RCC_APBENR1_TIM3EN;//RCC_APBENR1_DBGEN;//
	RCC->AHBENR  = RCC_AHBENR_DMA1EN | RCC_AHBENR_FLASHEN | RCC_AHBENR_CRCEN;	
	RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN;	
	MODIFY_REG(FLASH->ACR,FLASH_ACR_LATENCY,FLASH_ACR_LATENCY_0);
	MODIFY_REG(PWR->CR1, PWR_CR1_VOS, PWR_CR1_VOS_0);
	RCC->CR|= RCC_CR_HSION|(3UL<<RCC_CR_HSIDIV_Pos);//2MHz
  while(!(RCC->CR&RCC_CR_HSIRDY))__NOP();	
	
	
	MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPD13_Msk|GPIO_PUPDR_PUPD14_Msk,GPIO_PUPDR_PUPD14_1|GPIO_PUPDR_PUPD13_0 );
	MODIFY_REG(GPIOA->MODER,GPIO_MODER_MODE0_Msk|GPIO_MODER_MODE1_Msk|GPIO_MODER_MODE2_Msk|GPIO_MODER_MODE3_Msk|GPIO_MODER_MODE4_Msk|
													GPIO_MODER_MODE5_Msk|GPIO_MODER_MODE6_Msk|GPIO_MODER_MODE7_Msk|GPIO_MODER_MODE8_Msk|
													GPIO_MODER_MODE11_Msk|GPIO_MODER_MODE12_Msk|GPIO_MODER_MODE13_Msk|GPIO_MODER_MODE14_Msk,
													GPIO_MODER_MODE0_0|GPIO_MODER_MODE1_0|GPIO_MODER_MODE2_1|GPIO_MODER_MODE3_1|GPIO_MODER_MODE4_0|
													GPIO_MODER_MODE6_1|GPIO_MODER_MODE7_1|
													GPIO_MODER_MODE8_1|GPIO_MODER_MODE11_1|GPIO_MODER_MODE12_0|GPIO_MODER_MODE13_1|GPIO_MODER_MODE14_1);
	MODIFY_REG(GPIOA->AFR[0],GPIO_AFRL_AFSEL2_Msk|GPIO_AFRL_AFSEL3_Msk|GPIO_AFRL_AFSEL6_Msk|GPIO_AFRL_AFSEL7_Msk,
														GPIO_AFRL_AFSEL2_0|GPIO_AFRL_AFSEL3_0|GPIO_AFRL_AFSEL6_0|GPIO_AFRL_AFSEL7_0);
	MODIFY_REG(GPIOA->AFR[1],GPIO_AFRH_AFSEL8_Msk|GPIO_AFRH_AFSEL11_Msk,GPIO_AFRH_AFSEL8_1|GPIO_AFRH_AFSEL11_1);

	MODIFY_REG(GPIOB->PUPDR,GPIO_PUPDR_PUPD7_Msk,GPIO_PUPDR_PUPD7_0);
	MODIFY_REG(GPIOB->MODER,GPIO_MODER_MODE6_Msk|GPIO_MODER_MODE7_Msk|GPIO_MODER_MODE9_Msk,
													GPIO_MODER_MODE6_1|GPIO_MODER_MODE9_0);
	GPIOB->OSPEEDR|=GPIO_OSPEEDR_OSPEED0|GPIO_OSPEEDR_OSPEED6|GPIO_OSPEEDR_OSPEED8;
	MODIFY_REG(GPIOB->AFR[0],GPIO_AFRL_AFSEL6_Msk,GPIO_AFRL_AFSEL6_0);
	GPIOA->BSRR = GPIO_BSRR_BS0|GPIO_BSRR_BS1|GPIO_BSRR_BR4|GPIO_BSRR_BS12;
	Hall = Hall_OFF;
	TIM1->ARR = 31;
	TIM1->CCER |=TIM_CCER_CC1E|TIM_CCER_CC3E|TIM_CCER_CC4E;
	TIM1->BDTR = TIM_BDTR_MOE;
	TIM1->CCMR1 = TIM_CCMR1_OC1PE|(6UL<<TIM_CCMR1_OC1M_Pos);
	TIM1->CCMR2 = TIM_CCMR2_OC3PE|(6UL<<TIM_CCMR2_OC3M_Pos)|TIM_CCMR2_OC4PE|(6UL<<TIM_CCMR2_OC4M_Pos);
	TIM1->CR1 = TIM_CR1_CEN;
	CRC->INIT = 0;
	CRC->POL = 7;
	CRC->CR = CRC_CR_POLYSIZE_1 | CRC_CR_RESET;
	DMAMUX1_Channel0->CCR = DMAMUX_CxCR_DMAREQ_USART2_RX;	
	DMA_RX->CPAR = (unsigned int)&USART2->RDR;
	DMA_RX->CMAR = (unsigned int)&rx_buf;
	DMA_RX->CNDTR = 16;
	DMA_RX->CCR = DMA_CCR_CIRC | DMA_CCR_MINC | DMA_CCR_EN;
	DMAMUX1_Channel1->CCR = DMAMUX_CxCR_DMAREQ_USART2_TX;
	DMA_TX->CPAR = (unsigned int)&USART2->TDR;
	DMA_TX->CMAR = (unsigned int)&tx_buf;
	DMA_TX->CNDTR = 4;
	TIM17->ARR = 0xFFFF;
	TIM14->ARR = 0xFFFF;
	TIM16->ARR = 0xFFFF;
	TIM16->PSC = 62499;// 32Hz
	TIM16->CNT = 0;
	TIM16->CR1 = 0;
	TIM3->ARR = 0xFFFFFFFF;
	TIM3->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0;
	TIM3->CCER = TIM_CCER_CC2P;
	TIM3->SMCR = 3UL << TIM_SMCR_SMS_Pos;
	TIM3->CR1 = TIM_CR1_CEN;
	USART2->CR2 = USART_CR2_SWAP;
	USART2->CR3 = USART_CR3_DMAR | USART_CR3_DMAT;
	USART2->BRR = 104;//19200
	USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
	tim3_cnt_tmp = 0;
}
