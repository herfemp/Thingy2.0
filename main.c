#include "common.h"
#include "loader.bin.h"


#include "BDM/BDM.h"
uint16_t MCPLegionbytes = sizeof(loader_bin);



uint16_t CMFIMCR_Enable = (

	0<<13 |
	1<<12 |
	1<<11 |
	3<< 6
/*	0= Minimum bus cycles = 3 clocks, 1 inserted wait states
	1 = Minimum bus cycles = 4 clocks, 2 inserted wait states
	2 = Minimum bus cycles = 5 clocks, 3 inserted wait states
	3 = Minimum bus cycles = 2 clocks, 0 inserted wait states*/
);



uint16_t CMFIMCR_Stop = (
    1<<15 | /* stop */
	0<<13 | /* SIE (Shadow information enable)*/
	1<<12 | /* boot*/
	1<<11 | /* lock (1=disable)*/
	3<< 6
/*	0= Minimum bus cycles = 3 clocks, 1 inserted wait states
	1 = Minimum bus cycles = 4 clocks, 2 inserted wait states
	2 = Minimum bus cycles = 5 clocks, 3 inserted wait states
	3 = Minimum bus cycles = 2 clocks, 0 inserted wait states*/
);
void UploadLegionMCP(){

	uint16_t Location = 2;
	uint8_t i;

	printf("Uloading mcpleg\n\r");
	Exec_WriteCMD(0x8080C-4, WRITE32_BDM, 0);
	do{ Exec_FillCMD_p32((uint32_t *)&loader_bin[Location]);
		Location+=4;
	}while(Location<MCPLegionbytes);
	printf("Done\n\r");

	Exec_WriteCMD_s(0xFF, CMFIMCRAddr, WRITE16_BDM,   0, CMFIMCR_Stop);
	Exec_WriteCMD_s(0xFF, CMFIBAHAddr, WRITE16_BDM,   0, 0);
	Exec_WriteCMD_s(0xFF, CMFIBALAddr, WRITE16_BDM,   0, 0); ///< Should ALWAYS be 0 on 256k parts

	///< Enable write acc, start flash
	Exec_WriteCMD_s(0xFF, CMFIMCRAddr, WRITE16_BDM,   0, CMFIMCR_Enable);

	Output_Init();

	uint32_t Addr = 0;




	for(Addr=Addr; Addr<10; Addr+=4){
		Exec_ReadCMD(Addr, READ32_BDM);
		printf("%02X%04X: %04X%04X\n\r", (uint16_t)Addr>>16, (uint16_t)Addr, bdmresp32, bdmresp16);


	}


	Exec_WriteCMD(0, W_SREG_BDM, 0x100);     ///< Set PC to start of driver
	ShiftData(0);
	ShiftData_s(BDM_GO);
	LED_Trigger(ACCLED);

	while( ReadPin(P_FRZ) )
			;

	printf("Running\n\r");
	for(i=20; i>0; i--){ ///< De-bounce. Todo: Replace these with direct I/O

		if(!ReadPin(P_RST)) printf("Reset!\n\r");
		if(ReadPin(P_RST) && !ReadPin(P_FRZ) && i < 254) i += 2;

	}

	printf("Background!\n\r");

	 // while(1){}

}


int main(void){

	InitSys();
	PrepforBDM();
	InitSPI();

	printf("Starting\n\r");
	// UploadLegionMCP();

	// Testcan();
	// printf("Survived calc\n\r");
	// Conway();

	// TestT7();
	// TestT5();




	uint16_t SizeK = 0;
	f_mount(&FatFs, "", 0);

	if(Systype == 1){
		if(f_open( &Fil, "t5.bin", FA_OPEN_EXISTING | FA_READ ) == FR_OK)
			SizeK = 256;
	}else if(Systype == 2){
		if(f_open( &Fil, "t7.bin", FA_OPEN_EXISTING | FA_READ ) == FR_OK)
			SizeK = 512;
	}else if(Systype == 3){
		if(f_open( &Fil, "t8.bin", FA_OPEN_EXISTING | FA_READ ) == FR_OK)
			SizeK = 1024;
	}else if(Systype == 4){
		if(f_open( &Fil, "r11.bin", FA_OPEN_EXISTING | FA_READ ) == FR_OK)
			SizeK = 257;
	}

	if(SizeK!=257 && SizeK){
		if(!Flash(SizeK)) OhNoes(3, "Failed");
	}else if(SizeK == 257){
		if(!FlashMCP()) OhNoes(3, "Failed");
	}


	// BenchmarkDump(256);

	f_close(&Fil);

    while(1){}

}
