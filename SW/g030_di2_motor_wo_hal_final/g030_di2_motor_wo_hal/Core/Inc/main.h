#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif
#include "stm32g0xx.h"


#define DMAMUX_CxCR_DMAREQ_USART2_RX 52UL
#define DMAMUX_CxCR_DMAREQ_USART2_TX 53UL
#define DMA_RX DMA1_Channel1
#define DMA_TX DMA1_Channel2
#define DMA_TX_CCR (DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_EN)
#define Send_TX() 		\
												DMA_TX->CCR = 0;\
												DMA_TX->CNDTR = 4;\
												CRC->CR |= CRC_CR_RESET;\
												CRC->DR = (*(unsigned int *)&tx_buf)&0xFFFFFF;\
												__NOP();\
												tx_buf[3] = (CRC->DR)&0xFF;\
												DMA_TX->CCR = DMA_TX_CCR
	
#define MOTOR_OFF() 	\
												TIM17->CR1 = 0;\
												TIM1->CCR1 = 0;\
												TIM1->CCR3 = 0;\
												TIM1->CCR4 = 0;\
												TIM17->CNT = 0;\
												hall_in = 0;\
												GPIOB->BSRR = GPIO_BSRR_BR9;\
												GPIOA->MODER &= ~(GPIO_MODER_MODE0_Msk|GPIO_MODER_MODE1_Msk|GPIO_MODER_MODE8_Msk|GPIO_MODER_MODE11_Msk|GPIO_MODER_MODE12_Msk);\
												GPIOA->MODER |= GPIO_MODER_MODE0|GPIO_MODER_MODE1|GPIO_MODER_MODE8|GPIO_MODER_MODE11|GPIO_MODER_MODE12;\
												GPIOB->MODER &= ~(GPIO_MODER_MODE6_Msk|GPIO_MODER_MODE7_Msk);\
												GPIOB->MODER |= GPIO_MODER_MODE6,GPIO_MODER_MODE7;\
												//GPIOA->BSRR = GPIO_BSRR_BR4
													
	
#define MOTOR_ON() 		\
												GPIOB->BSRR = GPIO_BSRR_BS9;\
												MODE = MODE_RUN;\
												TIM17->CR1 = TIM_CR1_CEN | TIM_CR1_OPM;\
												GPIOA->MODER &= ~(GPIO_MODER_MODE0_Msk|GPIO_MODER_MODE1_Msk|GPIO_MODER_MODE8_Msk|GPIO_MODER_MODE11_Msk|GPIO_MODER_MODE12_Msk);\
												GPIOA->MODER |= GPIO_MODER_MODE0_0|GPIO_MODER_MODE1_0|GPIO_MODER_MODE8_1|GPIO_MODER_MODE11_1|GPIO_MODER_MODE12_0;\
												GPIOB->MODER &= ~(GPIO_MODER_MODE6_Msk|GPIO_MODER_MODE7_Msk);\
												GPIOB->MODER |= GPIO_MODER_MODE6_1;\
												//GPIOA->BSRR = GPIO_BSRR_BS4

typedef enum {
  MODE_STOP		= 0UL,
  MODE_RUN 		= 1UL,
} ModeTypeDef;
typedef enum {
  Hall_OFF	= 0UL,
  Hall_ON		= 1UL,
} HallTypeDef;
static void Init_HW(void);
void main_while_sram(void)__attribute((section("sram")));

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
