
#include "main.h"
#define DMA_ADC 					DMA1_Channel3
#define TIM_GEN 					TIM3

//846//1795//132//60
unsigned short pos_up[14]={160, 292, 424, 556, 688, 820, 952, 1084, 1216, 1348, 1480, 1612, 1744, 1856};
unsigned short pos_down[14]={120, 232, 364, 496, 628, 760, 892, 1024, 1156, 1288, 1420, 1552, 1684, 1816};

unsigned short tx_pos = 1256;
unsigned char tx_div = 26;
unsigned char rx_div = 32;
unsigned short rx_pos;
unsigned char pos = 0;
unsigned short adc_data[3];
StateButtonTypeDef Button;
SendTypeDef SendState;
__align(4) unsigned char rx_buf[16];
__align(4) unsigned char tx_buf[4]={0x13,0x57,0x9B,0xDF};
__align(4) unsigned char rx_buf_tmp[4];

unsigned flag=0;
int main(void)
{
	Init_HW();
	main_while_sram();
}
void send_pos(void)__attribute((section("sram")));
char tx_cmd=0x41;

void send_pos(void){
	tx_buf[0]=tx_cmd;
	tx_buf[1]=(tx_div<<3)|((tx_pos>>8)&0x7);
	tx_buf[2]=tx_pos&0xFF;
	Send_TX();
};

void main_while_sram(void){
	unsigned short rx_cnt,rx_cnt_next;
	rx_cnt = DMA_RX->CNDTR;
	rx_cnt_next = ((rx_cnt-5)&0xF)+1;
	TIM14->CNT = 0;
	TIM14->CR1 = 0;
	Button = BUTTON_INIT;
	TIM17->CR1 = 0;
	TIM17->CNT = 50000;
	pos = 0;
	while(!(PWR->CR1&PWR_CR1_FPD_LPRUN)){PWR->CR1 |= PWR_CR1_FPD_LPRUN;__NOP();__NOP();__NOP();}
	while(!(PWR->CR1&PWR_CR1_LPR)){PWR->CR1 |= PWR_CR1_LPR;__NOP();__NOP();__NOP();}
  while (1)
  {
		if(flag){
			flag = 0;
			send_pos();
		}
		if(TIM17->CNT >= 50000){
			if(Button == BUTTON_WAIT){
				if((adc_data[0] == adc_data[1]) & (adc_data[0] == adc_data[2]) & (adc_data[0] > 18) & (adc_data[0] < 22)){
					Button = BUTTON_DOWN;					
					TIM17->CNT = 0;
					TIM17->CR1 = TIM_CR1_CEN | TIM_CR1_OPM;
					if(pos){
						pos--;
						tx_pos = pos_down [pos];
						send_pos();
						SendState = SEND_WAIT;
						TIM1->CNT = 0;
						TIM1->CR1 = TIM_CR1_CEN;
						rx_buf[16 - rx_cnt] = 0;
					}
				}					
				else if((adc_data[0] == adc_data[1]) & (adc_data[0] == adc_data[2]) & (adc_data[0] > 40) & (adc_data[0] < 44)){
					if(pos<13){
						pos++;
						tx_pos = pos_up[pos];
						send_pos();
						SendState = SEND_WAIT;
						TIM1->CNT = 0;
						TIM1->CR1 = TIM_CR1_CEN;
						rx_buf[16 - rx_cnt] = 0;
					}
					Button = BUTTON_UP;
					TIM17->CNT = 0;
					TIM17->CR1 = TIM_CR1_CEN | TIM_CR1_OPM;
				}
				else{
					TIM17->CR1 = 0;
				}
			}			
			else if(Button == BUTTON_DOWN){
				if((adc_data[0] == adc_data[1]) & (adc_data[0] == adc_data[2]) & (adc_data[0] == 63)){
					Button = BUTTON_WAIT;
					TIM17->CNT = 0;
					TIM17->CR1 = TIM_CR1_CEN | TIM_CR1_OPM;
				}		
				else{
					TIM17->CR1 = 0;
				}
			}
			else if(Button == BUTTON_UP){
				if((adc_data[0] == adc_data[1]) & (adc_data[0] == adc_data[2]) & (adc_data[0] == 63)){
					Button = BUTTON_WAIT;
					TIM17->CNT = 0;
					TIM17->CR1 = TIM_CR1_CEN | TIM_CR1_OPM;
				}		
				else{
					TIM17->CR1 = 0;
				}
			}
			else if(Button == BUTTON_INIT){
				if((adc_data[0] == adc_data[1]) & (adc_data[0] == adc_data[2]) & (adc_data[0] > 18)  & (adc_data[0] < 44)){
					tx_pos = 1900;
					tx_div = 15;
					send_pos();
					TIM17->CNT = 0;
					TIM17->CR1 = TIM_CR1_CEN | TIM_CR1_OPM;
				}		
				else{
					TIM17->CR1 = 0;
				}
			}
		}
		if(TIM14->CNT > 5000){
			TIM14->CR1 = 0;
			TIM14->CNT = 0;
			rx_cnt = DMA_RX->CNDTR;
			rx_cnt_next = ((rx_cnt-5)&0xF)+1;
		}
		if(TIM1->CNT > 10000){
			if (SendState == SEND_WAIT){
					send_pos();
					TIM1->CNT = 1;
			}
			else{
					TIM1->CNT = 1;
					TIM1->CR1 = 0;
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
						rx_pos =((rx_buf_tmp[1] << 8) & 0x700) | rx_buf_tmp[2];
						if(rx_buf_tmp[0] == 0xC2){
							SendState = SEND_OK;
							TIM16->CR1 = 0;
							TIM16->CNT = 1;
						}
						if((Button == BUTTON_INIT) & (rx_buf_tmp[0] == 0xC3)){
							tx_div = 31;
							tx_pos = 1885;
							tx_cmd = 0x44;
							send_pos();
							tx_cmd = 0x41;
							pos = 13;
							Button = BUTTON_WAIT;
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
	RCC->APBENR2 = RCC_APBENR2_TIM1EN | RCC_APBENR2_TIM17EN | RCC_APBENR2_TIM14EN | RCC_APBENR2_TIM16EN | RCC_APBENR2_ADCEN;//RCC_APBENR2_SYSCFGEN
	
	RCC->APBENR1 = RCC_APBENR1_PWREN | RCC_APBENR1_DBGEN | RCC_APBENR1_USART2EN | RCC_APBENR1_TIM3EN;//RCC_APBENR1_DBGEN;//
	RCC->AHBENR  = RCC_AHBENR_DMA1EN | RCC_AHBENR_FLASHEN | RCC_AHBENR_CRCEN;	
	RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN;	
	MODIFY_REG(FLASH->ACR,FLASH_ACR_LATENCY,FLASH_ACR_LATENCY_0);
	//MODIFY_REG(PWR->CR1, PWR_CR1_VOS, PWR_CR1_VOS_0);
	RCC->CR|= RCC_CR_HSION|(5UL<<RCC_CR_HSIDIV_Pos);//0.5MHz
  while(!(RCC->CR&RCC_CR_HSIRDY))__NOP();	
	
	
	MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPD13_Msk|GPIO_PUPDR_PUPD14_Msk,GPIO_PUPDR_PUPD14_1|GPIO_PUPDR_PUPD13_0 );
	MODIFY_REG(GPIOA->MODER,GPIO_MODER_MODE0_Msk|GPIO_MODER_MODE1_Msk|GPIO_MODER_MODE2_Msk|GPIO_MODER_MODE3_Msk|GPIO_MODER_MODE4_Msk|
													GPIO_MODER_MODE5_Msk|GPIO_MODER_MODE6_Msk|GPIO_MODER_MODE7_Msk|GPIO_MODER_MODE8_Msk|
													GPIO_MODER_MODE11_Msk|GPIO_MODER_MODE12_Msk|GPIO_MODER_MODE13_Msk|GPIO_MODER_MODE14_Msk,
													GPIO_MODER_MODE0_0|GPIO_MODER_MODE1_0|GPIO_MODER_MODE2_1|GPIO_MODER_MODE3_1|GPIO_MODER_MODE4_0|
													GPIO_MODER_MODE8_1|GPIO_MODER_MODE11_1|GPIO_MODER_MODE12_0|GPIO_MODER_MODE13_1|GPIO_MODER_MODE14_1);
	MODIFY_REG(GPIOA->AFR[0],GPIO_AFRL_AFSEL2_Msk|GPIO_AFRL_AFSEL3_Msk,GPIO_AFRL_AFSEL2_0|GPIO_AFRL_AFSEL3_0);
	MODIFY_REG(GPIOA->AFR[1],GPIO_AFRH_AFSEL8_Msk|GPIO_AFRH_AFSEL11_Msk,GPIO_AFRH_AFSEL8_1|GPIO_AFRH_AFSEL11_1);

	MODIFY_REG(GPIOB->PUPDR,GPIO_PUPDR_PUPD7_Msk,GPIO_PUPDR_PUPD7_0);
	MODIFY_REG(GPIOB->MODER,GPIO_MODER_MODE6_Msk|GPIO_MODER_MODE7_Msk|GPIO_MODER_MODE9_Msk,
													GPIO_MODER_MODE6_1|GPIO_MODER_MODE9_0);
	GPIOB->OSPEEDR|=GPIO_OSPEEDR_OSPEED0|GPIO_OSPEEDR_OSPEED6|GPIO_OSPEEDR_OSPEED8;
	MODIFY_REG(GPIOB->AFR[0],GPIO_AFRL_AFSEL6_Msk,GPIO_AFRL_AFSEL6_0);
	GPIOA->BSRR = GPIO_BSRR_BS0|GPIO_BSRR_BS1|GPIO_BSRR_BS4|GPIO_BSRR_BS12;
	GPIOB->BSRR = GPIO_BSRR_BS9;
	TIM1->ARR = 65000;
//	TIM1->CCER |=TIM_CCER_CC1E|TIM_CCER_CC3E|TIM_CCER_CC4E;
//	TIM1->BDTR = TIM_BDTR_MOE;
//	TIM1->CCMR1 = TIM_CCMR1_OC1PE|(6UL<<TIM_CCMR1_OC1M_Pos);
//	TIM1->CCMR2 = TIM_CCMR2_OC3PE|(6UL<<TIM_CCMR2_OC3M_Pos)|TIM_CCMR2_OC4PE|(6UL<<TIM_CCMR2_OC4M_Pos);
//	TIM1->CR1 = TIM_CR1_CEN;
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
	TIM16->ARR = 60000;
	TIM16->PSC = 3;
	USART2->CR2 = USART_CR2_SWAP;
	USART2->CR3 = USART_CR3_DMAR | USART_CR3_DMAT;
	USART2->BRR = 26;//PPRE=4 104;//19200
	USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
	//DMA_TX->CCR = DMA_TX_CCR;
	
////TIM1 Init
//	TIM_GEN->CCMR1=TIM_CCMR1_OC2M_3|(0x6U<<TIM_CCMR1_IC1F_Pos)|(0x6U<<TIM_CCMR1_IC2F_Pos)|(0x6U<<TIM_CCMR1_OC1M_Pos)|(0x6U<<TIM_CCMR1_OC2M_Pos);
//	TIM_GEN->CCMR2=TIM_CCMR2_OC3M_3|(0x6U<<TIM_CCMR2_IC3F_Pos)|(0x6U<<TIM_CCMR2_IC4F_Pos)|(0x6U<<TIM_CCMR2_OC4M_Pos)|(0x6U<<TIM_CCMR2_OC3M_Pos);
//	TIM_GEN->CCER=TIM_CCER_CC1E|TIM_CCER_CC1P|TIM_CCER_CC2E|TIM_CCER_CC2P|TIM_CCER_CC3E|TIM_CCER_CC3P|TIM_CCER_CC4E|TIM_CCER_CC4P;	
	TIM_GEN->BDTR|=TIM_BDTR_MOE;
	TIM_GEN->CR2|=TIM_CR2_MMS_1;
	TIM_GEN->CR1=TIM_CR1_CMS_0;
//=========================================================
	TIM_GEN->ARR = 499;	

//=========================================================
//ADC Init
	DMA_ADC->CPAR=(unsigned int)&(ADC1->DR);
	DMA_ADC->CMAR=(unsigned int)&adc_data[0];
	DMA_ADC->CNDTR=3;
	DMA_ADC->CCR=DMA_CCR_CIRC|DMA_CCR_MINC|DMA_CCR_PL_1|DMA_CCR_MSIZE_0|DMA_CCR_PSIZE_0|DMA_CCR_EN;
	DMAMUX1_Channel2->CCR = (5UL << DMAMUX_CxCR_DMAREQ_ID_Pos) ;//Enable request from ADC
	ADC1->CFGR2=ADC_CFGR2_TOVS|ADC_CFGR2_CKMODE_0;	

	ADC1->CFGR1=ADC_CFGR1_CHSELRMOD|ADC_CFGR1_DISCEN|ADC_CFGR1_DMAEN|ADC_CFGR1_DMACFG|(0x01UL<<ADC_CFGR1_EXTEN_Pos)|(0x3UL<<ADC_CFGR1_EXTSEL_Pos)|(3UL<<ADC_CFGR1_RES_Pos);
	ADC1->CR=ADC_CR_ADVREGEN|ADC_CR_ADEN;	
	ADC1->CHSELR=(7UL<<ADC_CHSELR_SQ1_Pos)|(15UL<<ADC_CHSELR_SQ2_Pos);//|(3UL<<ADC_CHSELR_SQ3_Pos)|(3UL<<ADC_CHSELR_SQ4_Pos)|
								//(0UL<<ADC_CHSELR_SQ5_Pos)|(1UL<<ADC_CHSELR_SQ6_Pos)|(4UL<<ADC_CHSELR_SQ7_Pos)|(15UL<<ADC_CHSELR_SQ8_Pos);

	ADC1->CR|=ADC_CR_ADSTART;
	TIM_GEN->CR1|=TIM_CR1_CEN;//Start generator 
//=========================================================
	
}
