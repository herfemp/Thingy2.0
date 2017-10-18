

#include "../common.h"
#include "HAL.h"
#include "tim_hal.h"



void Install_Timer(uint32_t ms, uint32_t timer, uint32_t prio, uint8_t mtchstop){

	TIM_ITConfig(((TIM_TypeDef *) (APB1PERIPH_BASE + ((timer-2)*0x400))), TIM_IT_Update, DISABLE);

	EnableClk(timer+6); ///< Just to use created functions
//	RCC_APB1PeriphClockCmd(1<<(timer-2), ENABLE);

	TIM_TimeBaseStructure.TIM_Prescaler = (ms);
	TIM_TimeBaseStructure.TIM_Period = (uint16_t) ((SystemCoreClock / 2000)) - 1;//(ms);
	TIM_TimeBaseInit(((TIM_TypeDef *) (APB1PERIPH_BASE + ((timer-2)*0x400))), &TIM_TimeBaseStructure);

	TIM_ClearITPendingBit(((TIM_TypeDef *) (APB1PERIPH_BASE + ((timer-2)*0x400))), TIM_IT_Update);
	//TODO: Install Interrupt here? Well it actually SHOULD be initialized before activating TIM_IT
	//but that would force me to create more doubles. Keep an eye on this
	TIM_ITConfig(((TIM_TypeDef *) (APB1PERIPH_BASE + ((timer-2)*0x400))), TIM_IT_Update, ENABLE);

	//Must really do something neater than this
	if(mtchstop==0) mtchstop=1;
	else 			mtchstop=0;

	InstallINT(timer, prio);
	TIM_Cmd(((    TIM_TypeDef *)     (timers[timer])), mtchstop);

}





void ResetTimer(uint32_t Timer){

	TIM_Cmd(((    TIM_TypeDef *)     timers[Timer]), DISABLE);

	ClearTimerINT(Timer);

}


void StopTimer(uint32_t Timer){
#ifndef AVR
	TIM_Cmd(((    TIM_TypeDef *)     (timers[Timer])), DISABLE);
	ClearTimerINT(Timer);
#endif
}


void StartTimer(uint32_t Timer){

	ClearTimerINT(Timer);
	TIM_Cmd(((    TIM_TypeDef *)     (timers[Timer])), ENABLE);

}



void ClearTimerINT(uint32_t INT){

	TIM_ClearITPendingBit(((TIM_TypeDef *) (timers[INT])), TIM_IT_Update);

	ClearINT(INT); //Not really used by the stm ports but left here for simplicity in case it's needed later on.

}



//DWT timers. Used only on STM32

void DWT_Init(void){
	if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)){

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	}
}

uint32_t DWT_Get(void){return DWT->CYCCNT;}

__inline uint8_t DWT_Compare(int32_t tp){
	return (((int32_t)DWT_Get() - tp) < 0);
}

void DWT_Delay(uint32_t hz){
	int32_t tp = DWT_Get() + hz;
	while (DWT_Compare(tp));
}

void usleep(uint32_t us){
	DWT_Delay(us * (SystemCoreClock/1000000));
}


