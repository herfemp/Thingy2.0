#include "../common.h"
#include "bdm.h"

//< Mayhem..
uint16_t FMCR = (
0 << 15 | // stop
0 << 14 | // VEN

0 << 11 | // RLCK Lock set ram base
1 << 10 | // DLY1
0 <<  8   // RASP 0 urestricted, 1 unrestricted program, 2 sup prog and data, 3 sup prog
);
///0 Reserved/undef
///1 User data
///2 User program data
///3 Undef/reserved
///4 Undef/reserved
///5 Supervisor data
///6 Supervisor program data
///7 CPU
uint8_t DFC = 0x05;
uint8_t SFC = 0x05;


///< 0 = Discrete output
///< 1 = Alternate Function
///< 2 = Chip Select  8-bit port
///< 3 = Chip Select 16-bit port
uint16_t CSPAR0=(
3 << 12 | //CSPA0[6]
3 << 10 | //CSPA0[5]
3 <<  8 | //CSPA0[4]
3 <<  6 | //CSPA0[3]
3 <<  4 | //CSPA0[2]
3 <<  2 | //CSPA0[1]
3		  //CSBOOT (Can only be 2 or 3?)
);

///< Bsize / addr pin =
///0    2K Addr 23:11
///1    8K 13
///2   16K 14
///3   64K 16
///4  128K 17
///5  256K 18
///6  512K 19
///7 1024K / 1M
///Start Address 23-11 (bit 15-3) ///< Blocksize
 uint16_t CSBARBT = (
 ///Start Address 23-11 (bit 15-3)
 7 ///< Blocksize
 );

 ///< Chip-Select Option Register Boot ROM
 uint16_t CSORBT = (
 0<<15 | /// Mode: 0 Async, 1 Sync
 3<<13 | /// Byte: 0 Disable, 1 Low byte, 2 High Byte, 3 Both bytes. (Setting only useful when 16 bit port io is selected in pin assign reg.)
 1<<11 | /// RW: 0 Reserved, 1 Read only, 2 write only, 3 read and write. //default: 1
 0<<10 | /// STRB: 0 Addr, 1 Data strobe
 0<< 6 | /// DSACK:  no. of wait states in async mode, 14=Fast termination, 15=external dsack
 3<< 4 | /// SPACE:  0 = CPU, 1 = User, 2 = Supervisor, 3 = Supervisor/User
 0<< 1 | /// IPL:  Interrupt priority level. 0 = any, 1-7=level n
 0       /// AVEC: 0 External Int vec enable, 1 autovector enable
 );



void PrepTrionic82(){
	
	Exec_WriteCMD_s(0xFF, 0xfb04, WRITE16_BDM,   0,     8);
	Exec_WriteCMD_s(0xFF, 0xfb06, WRITE16_BDM,   0,     0);
	Exec_WriteCMD_s(0xFF, 0xfb00, WRITE16_BDM,   0, 0x800);
}

///< Set up SRAM. -Since the manual can't decide if it has 8 ,16, 32 or anything in between (or more) K of ram I can't complete this function. 
 ///< ..It has at least 32K btw. 
 void PrepTrionic81(){
	 
	///< Stop SRAM
	Exec_WriteCMD_s(0xFF, FASRAM_Base, WRITE16_BDM,   0, 0x8400);
	
	///< Stop DPTRAM
	Exec_WriteCMD_s(0xFF, DPTMCR_Base, WRITE16_BDM,   0, 0x8000);
	///< Start DPTRAM
	Exec_WriteCMD_s(0xFF, DPTMCR_Base, WRITE16_BDM,   0, 0);
	Exec_WriteCMD_s(0xFF, 0xF684, WRITE16_BDM,   0, 0x1000);
	 	 
	// Kill watchdog 
	Exec_WriteCMD_s(0xFF, 0xFA27, WRITE8_BDM,   0, 0x55);
	Exec_WriteCMD_s(0xFF, 0xFA27, WRITE8_BDM,   0, 0xAA);
	Exec_WriteCMD_s(0xFF, 0xFA58, WRITE16_BDM,   0, 0);

}

void PrepTrionic5(){

		 ///< T5 stuff.
		 Exec_WriteCMD_s(0xFF, 0xFA44, WRITE16_BDM,   0, CSPAR0);  ///< Set up CS pins.
		 Exec_WriteCMD_s(0xFF, 0xFA48, WRITE16_BDM,   0, CSBARBT); ///< CS Base Addr
		 Exec_WriteCMD_s(   0,      0, FILL16_BDM,    0, CSORBT);  ///< Chip-Select Option Register Boot ROM
		 
		 Exec_WriteCMD_s(0xFF, 0xFA50, WRITE16_BDM,   0,    0x7);  ///< CSBAR1, BS1 M, base @ 0 ? Addr 23:11 = 0
		 Exec_WriteCMD_s(   0,      0, FILL16_BDM,    0, 0x3030);  ///< CSOR 1
		 Exec_WriteCMD_s(   0,      0, FILL16_BDM,    0, 0x0007);  ///< CSBAR2, BS1 M,
		 Exec_WriteCMD_s(   0,      0, FILL16_BDM,    0, 0x5030);  ///< CSOR 2
		 
		 sleep(50);
		 Exec_WriteCMD_s(0xFF, 0xFB04, WRITE16_BDM,   0, 0x1000); ///< Link DPTRAM to Addr 0x10000
}

void PrepT(){

	Systype = 0;
	uint16_t ClockSet = 0;

	Exec_WriteCMD_s(   0,      0, W_SREG_BDM+0x0e, 0, SFC); ///< Source Function Register (SFC)
	Exec_WriteCMD_s(   0,      0, W_SREG_BDM+0x0f, 0, DFC); ///< Destination Function Register (DFC)
 
	///< Is it Trionic 8 CPU1?
	Exec_ReadCMD_s(0xFF, 0xFA08, READ16_BDM);
	// printf("FA08: %04X\n", bdmresp16);

	// Trionic 8, CPU1 (Main)
	if(bdmresp == 0xA908){
		Systype = 3;
		Exec_WriteCMD_s(0xFF, 0xFA08, WRITE16_BDM,   0, 0x7808);

	// If given the wrong response, assume T5, T7 or MCP
	}else{
	
		// Do another check. Is it MCP?
		Exec_ReadCMD_s(0xFF, 0xFA04, READ16_BDM);
		// printf("FA04: %04X\n", bdmresp16);
		
		if(bdmresp == 0x3008){       // Trionic 8 CPU2 (MCP)
			Systype = 4;
			// ClockSet = (1<<15|7<<12);//MCPClock;
			ClockSet = (
					1 << 15 | // X (Divide by 2=0)
					7 << 12 | // W (Multiplier)
					0 <<  8 | // Y (Divider)
					0 <<  7 | // EDIV (Divide by 16=1)
					0 <<  5); // LOSCD )
					//0xE000;

		}else{
			Exec_ReadCMD_s(0xFF, 0xFA24, READ16_BDM); // Read PITR to get status of MODCLK pin
			if(bdmresp16&0x100){ // Trionic 7
				Systype = 2;
				ClockSet=0x7F00; // 16,67 .. This is wrong but it works

			}else{ // Trionic 5
				Systype = 1;
				// ClockSet=0x7F00; // 16,67
				ClockSet=0xD300; // 20,9 mhz Overclock.
			}
		}
		if(ClockSet)
			Exec_WriteCMD_s(0xFF, 0xFA04, WRITE16_BDM,   0, ClockSet);
	}

	///< Couldn't get reliable answers by checking the locked-flag. Delay will have to do.
	sleep(100);

	if(Systype == 4){       // MCP
		printf("Trionic 8; MCP");
		PrepTrionic82();
	}else if(Systype == 3){ // T8
		printf("Trionic 8");
		PrepTrionic81();
	}else if(Systype == 2){ // T7
		printf("Trionic 7");
		PrepTrionic5();
	}else if(Systype == 1){ // T5
		printf("Trionic 5");
		PrepTrionic5();
	}else{                  // Unknown
		printf("Unknown sys!");
		while(1){};
	}
	sleep(1000);
	printf("\n\r\n\r");

	if(Systype < 3)
		Exec_WriteCMD_s(0xFF, 0xFA21, WRITE8_BDM , 0, 0);		///< Kill watchdog

}
