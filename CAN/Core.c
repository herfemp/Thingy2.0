#include "../common.h"



#define RCC_APB2Periph_GPIO_CAN1   RCC_APB2Periph_GPIOB
#define GPIO_Remapping_CAN         GPIO_Remap1_CAN1
#define GPIO_CAN                   GPIOB
#define GPIO_Pin_CAN_RX            GPIO_Pin_8
#define GPIO_Pin_CAN_TX            GPIO_Pin_9

CanTxMsg TxMessage;
CAN_InitTypeDef        CAN_InitStructure;
CAN_FilterInitTypeDef  CAN_FilterInitStructure;


uint16_t prescaler=0;
uint8_t  tseg1=0;
uint8_t  tseg2=0;
uint8_t  sync=0;

/////////////////////////////////////////
//These are parameters for 'checkcalc'//
#define DES_SAMP_PNT	75 //Desired sampling point.
#define MAX_SAMP_DEVI	 10 //Allowable sampling point deviation.
#define MAX_BTR_DEVI	 0.5 //Allowable percentage of bitrate deviation



uint8_t CANMsgInBuffer(){
	if(CAN_MessagePending(CAN1, CAN_FIFO0)){

		CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
		//printf("msg in buffer\n\r");
		return 1;
	}

	return 0;
}



uint8_t waitmsg(uint16_t msgid){
	MiscTime=500;
	while(!CANMsgInBuffer() && MiscTime)
		;

	if  (MiscTime && RxMessage.StdId==msgid){
		// printf("Rec: %x\n\rReq: %x\n\r",RxMessage.StdId, msgid );
		return 1;
	}


	return 0;
	/*
	if  (MiscTime && RxMessage.StdId==msgid){ return 1; }
	else return 0;

*/

}



void CAN_Config(void){
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIO_CAN1, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_CAN_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIO_CAN, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_CAN_TX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_CAN, &GPIO_InitStructure);

	GPIO_PinRemapConfig(GPIO_Remapping_CAN , ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

	CAN_DeInit(CAN1);
	CAN_StructInit(&CAN_InitStructure);

	CAN_InitStructure.CAN_TTCM = DISABLE;
	CAN_InitStructure.CAN_ABOM = DISABLE;
	CAN_InitStructure.CAN_AWUM = DISABLE;
	CAN_InitStructure.CAN_NART = DISABLE;
	CAN_InitStructure.CAN_RFLM = DISABLE;
	CAN_InitStructure.CAN_TXFP = DISABLE;
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;

	CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
	CAN_InitStructure.CAN_BS1 = tseg1;
	CAN_InitStructure.CAN_BS2 = tseg2;
	CAN_InitStructure.CAN_Prescaler = prescaler;
	CAN_Init(CAN1, &CAN_InitStructure);
/*
	CAN_FilterInitStructure.CAN_FilterNumber = 0;

	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;//
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_16bit;
	CAN_FilterInitStructure.CAN_FilterIdHigh = 0;//0x7ff<<5
	CAN_FilterInitStructure.CAN_FilterIdLow =  0;//0x0<<5
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x7FF<<5; //0x7ff<<5
	CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0; //0x0<<5

	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
	CAN_FilterInit(&CAN_FilterInitStructure);

*/

	CAN_FilterInitStructure.CAN_FilterNumber	 = 0;
	CAN_FilterInitStructure.CAN_FilterMode 		 = CAN_FilterMode_IdList;
	CAN_FilterInitStructure.CAN_FilterScale 	 = CAN_FilterScale_16bit;
	CAN_FilterInitStructure.CAN_FilterIdHigh 	 = (0x238<<5);
	CAN_FilterInitStructure.CAN_FilterIdLow 	 = (0x258<<5);
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x266<<5;
	CAN_FilterInitStructure.CAN_FilterMaskIdLow  = 0;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
	CAN_FilterInit(&CAN_FilterInitStructure);




	TxMessage.StdId = 0x00;
	TxMessage.ExtId = 0x0;
	TxMessage.RTR = CAN_RTR_DATA;
	TxMessage.IDE = CAN_ID_STD;
	TxMessage.DLC = 0;



}

#define PCLK1 SystemCoreClock
uint8_t checkcalc(float desiredrate){

	//Check if current settings leads to our desired bitrate.
	if 		((float) ((PCLK1)/prescaler) / (sync+1+tseg1+1+tseg2+1) > (float)(desiredrate+(((float)desiredrate/100)*MAX_BTR_DEVI))){return 0;}
	else if ( (float)((PCLK1)/prescaler) / (sync+1+tseg1+1+tseg2+1) < (float)(desiredrate-(((float)desiredrate/100)*MAX_BTR_DEVI))){return 0;}

	//If we have a bitrate match, check if those settings leads to a reasonable sampling point.
	else if ((float)((100*(sync+1+tseg1+1)) / (sync+1+tseg1+1+tseg2+1)) > DES_SAMP_PNT+MAX_SAMP_DEVI){return 0;}
	else if ((float)((100*(sync+1+tseg1+1)) / (sync+1+tseg1+1+tseg2+1)) < DES_SAMP_PNT-MAX_SAMP_DEVI){return 0;}

	//We've passed!
	else{
		return 1;
	}
}



void STM32_Calc_BTR(float btr){

	sync=0;
	tseg1=0;
	tseg2=0;
	prescaler=0;

	if (btr>10){ //Just to make sure we ask for a sensible bitrate.
		do{ sync=0;
			tseg2++;

			if (tseg2>=7){
				tseg2=0;
				tseg1++;

				if(tseg1>=15){
					tseg1=0;
					prescaler++;

					if(prescaler>=1024){
						prescaler=0;
						printf("BTR Err!\n\r");

		}}}}while(!checkcalc(btr));
	}
	CAN_Config(); //We have our settings. Let's apply them.
	// CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);

}

CanTxMsg TxMessage;

uint8_t SendCAN(uint16_t ID, const uint8_t *MSG){
/*
	msg.length = 8;
	msg.id = ID;*/
	uint8_t i;
	TxMessage.StdId = ID;
	TxMessage.ExtId = 0x0;
	TxMessage.RTR = CAN_RTR_DATA;
	TxMessage.IDE = CAN_ID_STD;
	TxMessage.DLC = 8;
	for(i = 0; i < 8; i++ ){ TxMessage.Data[i]=MSG[i]; }
	CAN_Transmit(CAN1, &TxMessage);



	return  1; ///<-Fix this!

}


void InitCAN(uint8_t interface, float BTR){
	CAN_DeInit(CAN1);

	STM32_Calc_BTR(BTR);
}



void LED_Trigger(uint16_t led){
	TIM_Cmd(TIM3, ENABLE);
	GPIOC->ODR &= ~led;

}
void LED_Off(uint16_t led){
	GPIOC->ODR |=  led;
}

uint8_t t7initms[] = {0x3F,0x81,0x00,0x11,0x02,0x40,0x00,0x00};
void Testcan(){

	// InitCAN(1, 500000);




	InitCAN(1, 615384.615384);
	printf("BTR found\n\r");

	//InstallINT(1, CAN_INT_RX0_PRI);

	//sleep(1000);
	//SendCAN(0x220, t7initms);

}


void TIM3_IRQHandler(void){
	LED_Off(ACCLED);
	TIM_Cmd(TIM3, DISABLE);
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
}






void USB_LP_CAN1_RX0_IRQHandler(void){

	CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
	// printf("Got one!\n\r");

	LED_Trigger(ACCLED);
}
