#include "common.h"

void InitSys(){

	RCC_Configuration();

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	timer_IRQ_init();
	uart_init();
	SPIGPIFULLINIT();

	SetPinDir(    2, 13, 1); // Accled
	SetPinDir(SDCARD_CS, 1);
	SetPinDir(  LCD_RST, 1);
	SetPinDir(   LCD_DC, 1);
	SetPinDir(   LCD_CS, 1);

	WritePin (SDCARD_CS, 1); // Set both spi devices to inactive
	WritePin (   LCD_CS, 1);
	WritePin (    2, 13, 1); // Turn off led

	InitSPI();
	ST7735_InitR(INITR_GREENTAB);
	StTextColor = 0xFFFF;

	// Used this to check the timer. Couldn't believe the numbers i got :)
/*	printf("3!\n\r");
	BenchTime = 1000;
	while(BenchTime);
	printf("2!\n\r");
	BenchTime = 1000;
	while(BenchTime);
	printf("1!\n\r");
	BenchTime = 1000;
	while(BenchTime);
	printf("Start!\n\r");
	BenchTime = 100000;

	while(BenchTime>3000);
	printf("3!\n\r");
	while(BenchTime>2000);
	printf("2!\n\r");
	while(BenchTime>1000);
	printf("1!\n\r");
	while(BenchTime);
	printf("Stop!\n\r");*/
}

void uart_init(void) {

    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No ;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);
}

void timer_IRQ_init(void) {

	Install_Timer(1, KHZTMR, KHZTMR_PRIO, 0);
	Install_Timer(LED_TIMEOUT, LED_TIMER, LED_INT_PRI, 0);
}

void SPIGPIFULLINIT(){

	GPIO_InitTypeDef SPIBDMSTRUCT;
	SPIBDMSTRUCT.GPIO_Pin = SPIB_PIN_SCK | SPIB_PIN_MOSI;
	SPIBDMSTRUCT.GPIO_Mode = GPIO_Mode_AF_PP;
	SPIBDMSTRUCT.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPIB_GPIO, &SPIBDMSTRUCT);

	SPIBDMSTRUCT.GPIO_Pin = SPIA_PIN_SCK | SPIA_PIN_MOSI;
    GPIO_Init(SPIA_GPIO, &SPIBDMSTRUCT);
}



void InitSPI(){

	SPI_InitTypeDef   SPI_InitStructure;

	///< Common settings
    SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
    SPI_InitStructure.SPI_CRCPolynomial     = 0;

    ///< SPI 2 (BDM)
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_16b; ///< These three are the only things that differs from "normal" SPI to get BDM up and running.
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler = Systype!=3?SPI_BaudRatePrescaler_8:SPI_BaudRatePrescaler_4;
    SPI_Init(SPI2, &SPI_InitStructure);

    ///< SPI 1 (SDCARD)
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
   	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; // 28 Mhz
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_CalculateCRC(SPI1, DISABLE);
    SPI_CalculateCRC(SPI2, DISABLE);
    SPI_Cmd(SPI1, ENABLE);
    SPI_Cmd(SPI2, ENABLE);
}


void RCC_Configuration(void){

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB1PeriphClockCmd(0x24005, ENABLE); // Usart 2, spi, spi afio, more spi
	RCC_APB2PeriphClockCmd(0x0101D, ENABLE); // GPIO A,B,C, SPI, AFIO for SPI

}
