#include "HAL/Disk/ff.h"

FATFS FatFs;
FIL Fil;
UINT bw;

#include "ST7735.h"
uint16_t StTextColor;

void uart_putchar(char c, FILE *stream);
char uart_getchar(/*FILE *stream*/);
void uart_init(void);
void timer_IRQ_init(void);
void RCC_Configuration(void);
void SPIGPIFULLINIT();

void sleep(uint16_t time);
volatile uint32_t BenchTime;
volatile uint32_t MiscTime;

void printfLCD(const char *ch, uint16_t C);



