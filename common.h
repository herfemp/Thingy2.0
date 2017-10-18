#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_can.h"
#include "stm32f10x_flash.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"

#include "HAL/HAL.h"
#include "SupportFuncs.h"


void TestT7();
CanRxMsg RxMessage;

void Testcan();
void TIM2_IRQHandler() __attribute__ ((interrupt ("IRQ")));
// BDM
// 4: Trionic 8, CPU2 (MCP)
// 3: Trionic 8, CPU1 (Main)
// 2: Trionic 7
// 1: Trionic 5
// 0: Unknown, FAIL!
uint8_t  Systype;


uint8_t Flash(uint16_t  SizeK);
inline void BenchmarkDump(uint16_t SizeK);
uint8_t ResetTarget();
uint8_t PrepforBDM();
uint8_t FlashMCP();


void InitSys();
void OhNoes(uint8_t Severity, const char *Reason);


#define EMB_FLASH
#define Gpio_speed 			    GPIO_Speed_50MHz

#define SPIB                    SPI2
#define SPIB_CLK                RCC_APB1Periph_SPI2
#define SPIB_GPIO               GPIOB
#define SPIB_GPIO_CLK           RCC_APB2Periph_GPIOB
#define SPIB_PIN_SCK            GPIO_Pin_13
#define SPIB_PIN_MISO           GPIO_Pin_14
#define SPIB_PIN_MOSI           GPIO_Pin_15

#define SPIA_CLK                RCC_APB2Periph_SPI1
#define SPIA_GPIO               GPIOA
#define SPIA_GPIO_CLK           RCC_APB2Periph_GPIOA
#define SPIA_PIN_SCK            GPIO_Pin_5
#define SPIA_PIN_MISO           GPIO_Pin_6
#define SPIA_PIN_MOSI           GPIO_Pin_7
///< SD / LCD
#define SDCARD_CS 0, 4
#define LCD_CS    0, 1
#define LCD_DC    1, 1
#define LCD_RST   1, 0
///< BDM
#define P_RST     1, 11
#define P_FRZ     1, 12
#define P_BKPT    1, 13
#define P_DSO     1, 14
#define P_DSI     1, 15

#define SETBIT(x, y) 		(x |= (y))
#define CHECKBIT(x, y) 		(((x) & (y)) == (y))

#define CAN_INT_RX0_PRI         0
#define LED_INT_PRI        		2
#define KHZTMR_PRIO				3

#define KHZTMR					2
#define LED_TIMER				3
#define LED_TIMEOUT			    5


#define ACCLED GPIO_Pin_13
void LED_Trigger(uint16_t led);
