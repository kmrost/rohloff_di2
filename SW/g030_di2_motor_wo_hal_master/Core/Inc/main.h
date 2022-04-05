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
	
typedef enum {
  BUTTON_WAIT	= 0UL,
  BUTTON_UP 		= 1UL,
	BUTTON_DOWN	= 2UL,
	BUTTON_INIT	= 3UL,
} StateButtonTypeDef;

typedef enum {
  MODE_STOP		= 0UL,
  MODE_RUN 		= 1UL,
} ModeTypeDef;

typedef enum {
  SEND_WAIT		= 0UL,
  SEND_OK 		= 1UL,
} SendTypeDef;
static void Init_HW(void);
void main_while_sram(void) __attribute((section("sram")));

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
