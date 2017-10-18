#include "common.h"

void uart_putchar(char ch, FILE *f) {

	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)  ;
	USART_SendData(USART2, ch);
}

char uart_getchar() {

	while(!USART_GetFlagStatus(USART2, USART_FLAG_RXNE))  ;
	return (USART2->DR)&0xFF;
}

int fputc(int ch, FILE *f){

	if(ch != '\r') ST7735_OutChar(ch);
	/*if (ch == '\n') { fputc('\r', f); }
	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)  ;
	USART_SendData(USART2, ch);

*/
	return(ch);
}

void sleep(uint16_t time){

	MiscTime=time;
		do{}while(MiscTime>0);
}

void TIM2_IRQHandler(IRQ){

	if (BenchTime)BenchTime--;
	if (MiscTime) MiscTime--;
	ClearTimerINT(KHZTMR);
}
