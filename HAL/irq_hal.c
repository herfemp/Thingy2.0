//-----------------------------------------------------------------------------
//	CAN/BDM adapter firmware
//	(C) Janis Silins, 2010
//	$id$
//-----------------------------------------------------------------------------
//Further hacked and dashed, most certainly also ruined, by Christian Ivarsson.


#include "../common.h"
#include "HAL.h"
#include "irq_hal.h"




//////////////////////////////////////////////////////////////////////////////////////
//Use any of these with EXTREME caution. Check table in irq_hal.h before using them//
//Seriously -I mean it! ;) 														  //



//Clears an interrupt
void ClearINT(uint32_t INT){

}

//Installs / activates an interrupt source.
//On lpc23xx it also takes care of linking those interrupts to their corresponding handlers.
void InstallINT(uint32_t INT, uint32_t prio){

	NVIC_InitStructure.NVIC_IRQChannel = Interrupts[INT];
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = prio;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

}


//Un-Installs / de-activates an interrupt source.
//On lpc23xx it also takes care of linking those interrupts to their corresponding handlers.
void UnInstallINT(uint32_t INT, uint32_t prio){

	NVIC_InitStructure.NVIC_IRQChannel = Interrupts[INT];
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = prio;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStructure);

}


//Disables an interrupt.
void DisableINT(uint32_t INT){

	ClearINT(INT);

}


//Interrupt handlers. Used only on STM32

void NMI_Handler(void){/*UNIPRF_LEDOn(CATASTROPHIC);*/}
void HardFault_Handler(void){while (1){/*UNIPRF_LEDOn(CATASTROPHIC);*/ }}
void MemManage_Handler(void){while (1){/*UNIPRF_LEDOn(CATASTROPHIC);*/}}
void BusFault_Handler(void){while (1){/*UNIPRF_LEDOn(CATASTROPHIC);*/}}
void UsageFault_Handler(void){while (1){/*UNIPRF_LEDOn(CATASTROPHIC);*/}}
void SVC_Handler(void){/*UNIPRF_LEDOn(CATASTROPHIC);*/}
void DebugMon_Handler(void){/*UNIPRF_LEDOn(CATASTROPHIC);*/}
void PendSV_Handler(void){/*UNIPRF_LEDOn(CATASTROPHIC);*/}
void SysTick_Handler(void){}


//void USB_LP_CAN1_RX0_IRQHandler(void){

//	USB_Istr();
//}

void USB_HP_CAN1_TX_IRQHandler(void){

//	CTR_HP();
}


void USBWakeUp_IRQHandler(void){

	EXTI_ClearITPendingBit(EXTI_Line18);
}



