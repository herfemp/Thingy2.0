#include "common.h"



void OhNoes(uint8_t Severity, const char *Reason){

	StTextColor = Severity<3?0x07FF:0x001F;

	printf("\n\r");
	printf(Reason);
	f_close(&Fil);

	if(Severity>2)
		while(1){};
}

// It's a pain in the a to change color
void printfLCD(const char *ch, uint16_t C){

	StTextColor = C;
	printf(ch);
	StTextColor = 0xFFFF;
}
