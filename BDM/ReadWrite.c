#include "../common.h"
#include "BDM.h"
#include "Drivers/TxDriver.bin.h"
#include "Drivers/driver.bin.h"

#define TxBUFSTART    0x100000-4
#define TxDRVSTART    0x100400
#define TxBUFSIZE     0x000400
#define MCPBUFSTART   0x80000
#define MCPDRVSTART   0x81C00

#define DSICLKLOW SETBIT(((GPIO_TypeDef *) (GPIOA_BASE + 0x400))->BRR,  0xA000)
#define DSOREAD CHECKBIT(((GPIO_TypeDef *) (GPIOA_BASE + 0x400))->IDR,  ((uint16_t)0x4000) )

const uint16_t Txloaderbytes  = sizeof(TxDriver_bin);
const uint16_t mcploaderbytes = sizeof(driver_bin);

uint8_t  Slowdown = 0;
uint32_t Fbuf[256];

///< Supercharged, over the top, stupidly fast, bdm functions!
inline void ShiftData(uint16_t data){

	DSICLKLOW;
	(GPIOB->CRH) |= 0x80800000;
	SPI2->DR = data;
	while(!(SPI2->SR & SPI_I2S_FLAG_RXNE)) ;
	data = SPI2->DR;
	(GPIOB->CRH) &= 0x7F7FFFFF;
}


inline void ShiftData32(uint32_t data){

	uint32_t Stock = GPIOB->CRH;
	uint32_t temp  = GPIOB->CRH | 0x80800000;

	DSICLKLOW;
	(GPIOB->CRH) = temp;
	SPI2->DR = data<<8|((data>>8)&0xFF);
	data =(data>>16)<<8|((data>>24)&0xFF);
	while(!(SPI2->SR & SPI_I2S_FLAG_RXNE)) ;
	uint16_t discard = SPI2->DR;
	(GPIOB->CRH) = Stock;

	DSICLKLOW;
	(GPIOB->CRH) = temp;
	SPI2->DR = data;
	while(!(SPI2->SR & SPI_I2S_FLAG_RXNE)) ;
	discard = SPI2->DR;
	discard = discard; // Get rid of compiler warning
	(GPIOB->CRH) = Stock;
}

void Exec_WriteCMD(uint32_t Addr, uint16_t cmd, uint32_t Data){

	ShiftData(cmd);
	if(Addr){ ShiftData(Addr>>16); ShiftData(Addr&0xFFFF); }

	///< Bit 7 set; Send two payloads
	if(cmd&0x80) ShiftData(Data>>16);
	ShiftData(Data&0xFFFF);

	uint32_t Stock = GPIOB->CRH;
	uint32_t temp  = GPIOB->CRH | 0x80800000;

	if(!Slowdown){
		do{ DSICLKLOW; // Pull down clock and DSI
			if(!DSOREAD) return;
			(GPIOB->CRH) = temp;
			SPI2->DR = 0;
			while(!(SPI2->SR & SPI_I2S_FLAG_RXNE)) ;
			uint16_t spitmp = SPI2->DR;
			spitmp = spitmp; // Get rid of compiler waring. The register must be read even if the data isn't needed
			(GPIOB->CRH) = Stock;
		}while(1);
	}else{
		do{ DSICLKLOW; // Pull down clock and DSI
			if(!ReadPin(P_DSO)) return;
			(GPIOB->CRH) = temp;
			SPI2->DR = 0;
			while(!(SPI2->SR & SPI_I2S_FLAG_RXNE)) ;
			uint16_t spitmp = SPI2->DR;
			spitmp = spitmp; // Get rid of compiler waring. The register must be read even if the data isn't needed
			(GPIOB->CRH) = Stock;
		}while(1);
	}
}

inline void Exec_FillCMD_p32(const uint32_t *data){

	ShiftData(FILL32_BDM);
	ShiftData32(*data);

	uint32_t Stock = GPIOB->CRH;
	uint32_t temp  = GPIOB->CRH | 0x80800000;

	if(!Slowdown){
		do{ DSICLKLOW; // Pull down clock and DSI
			if(!DSOREAD) return;
			(GPIOB->CRH) = temp;
			SPI2->DR = 0;
			while(!(SPI2->SR & SPI_I2S_FLAG_RXNE)) ;
			uint16_t spitmp = SPI2->DR;
			spitmp = spitmp; // Get rid of compiler waring. The register must be read even if the data isn't needed
			(GPIOB->CRH) = Stock;
		}while(1);
	}else{
		do{ DSICLKLOW; // Pull down clock and DSI
			if(!ReadPin(P_DSO)) return;
			(GPIOB->CRH) = temp;
			SPI2->DR = 0;
			while(!(SPI2->SR & SPI_I2S_FLAG_RXNE)) ;
			uint16_t spitmp = SPI2->DR;
			spitmp = spitmp; // Get rid of compiler waring. The register must be read even if the data isn't needed
			(GPIOB->CRH) = Stock;
		}while(1);
	}
}

inline void ShiftWait(const uint16_t *data){

	// Oh I will hate myself for this one...
	uint32_t Stock = GPIOB->CRH;
	uint32_t temp  = GPIOB->CRH | 0x80800000;
	uint32_t Attn;

	if(!Slowdown){
		do{ DSICLKLOW; // Pull down clock and DSI
			Attn = DSOREAD;
			(GPIOB->CRH) = temp;
			SPI2->DR = 0;
			while(!(SPI2->SR & SPI_I2S_FLAG_RXNE)) ;
			*(uint16_t *)data = SPI2->DR;
			(GPIOB->CRH) = Stock;
		}while(Attn);
	}else{
		do{ DSICLKLOW; // Pull down clock and DSI
			Attn = ReadPin(P_DSO);
			(GPIOB->CRH) = temp;
			SPI2->DR = 0;
			while(!(SPI2->SR & SPI_I2S_FLAG_RXNE)) ;
			*(uint16_t *)data = SPI2->DR;
			(GPIOB->CRH) = Stock;
		}while(Attn);
	}
}

void Exec_ReadCMD(uint32_t Addr, uint16_t cmd){

	ShiftData(cmd);
	if(Addr){ ShiftData(Addr>>16); ShiftData(Addr&0xFFFF);}

	if(cmd&0x80) ShiftWait(&bdmresp32);
	ShiftWait(&bdmresp16);
}

void ResetPeskyFlash(){

	uint32_t data;

	Exec_WriteCMD(4, WRITE32_BDM, 0xFFFF0000); // Send 0xFFFF and then 0000; Enter read mode.
	Exec_WriteCMD(4, WRITE32_BDM, 0xFFFF0000);
	do{ Exec_ReadCMD(0,READ32_BDM);
		data = bdmresp32<<16|bdmresp16;
		Exec_ReadCMD(0,READ32_BDM);
		sleep(50);
	}while(data != (bdmresp32<<16|bdmresp16));
}


///< Oh, MCP... Why?
inline void DumpMCP(){

	f_close(&Fil);
	printf("Dump..  ");

	BenchTime      = 65535;
	uint16_t Size  = 1024;
	uint32_t Addr  =    4;
	uint32_t adder =    0;
    uint8_t  i     =    0;
	Slowdown       =    0;
	
	if(f_open(&Fil, "MCP.bin", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {OhNoes(2, "Could not create f"); return;}

	Exec_ReadCMD(0xFFF800, READ16_BDM);
	uint16_t Shadow = bdmresp16 & 0xDFFF;
	Exec_WriteCMD(0xFFF800, WRITE16_BDM, Shadow);

	do{ Exec_ReadCMD(0,READ32_BDM); ///< Since shadow starts from the same address as regular flash, this command has to be sent twice.
		*(uint32_t*)&Fbuf[i] = bdmresp16<<24|(bdmresp16>>8)<<16|(bdmresp32&0xFF)<<8|bdmresp32>>8;
		adder += (bdmresp32&0xFF) + ((bdmresp32>>8)&0xFF) + (bdmresp16&0xFF) + ((bdmresp16>>8)&0xFF);
		i++;
		do{ do{ ShiftData(DUMP32_BDM);
				ShiftWait(&bdmresp32);
				ShiftWait(&bdmresp16);
				adder += (bdmresp32&0xFF) + ((bdmresp32>>8)&0xFF) + (bdmresp16&0xFF) + ((bdmresp16>>8)&0xFF);
				*(uint32_t *)&Fbuf[i++] = bdmresp16<<24|(bdmresp16>>8)<<16|(bdmresp32&0xFF)<<8|bdmresp32>>8;
			}while((i && Size!=256) || (i<64 && Size==256));

			if(f_write(&Fil, &Fbuf, Size, &bw)!=FR_OK) OhNoes(3, "SDC Int Err");
			Addr+=Size;
			LED_Trigger(ACCLED);
		}while(Addr<0x40000);

		Exec_WriteCMD(0xFFF800, WRITE16_BDM, Shadow|0x2000);
		Size = 256;
	}while(Addr<0x40100);

	Exec_WriteCMD(0xFFF800, WRITE16_BDM, Shadow);
	printf("%ums \n\rCsum32  %04X%04X\n\r", (uint16_t)(65535 - BenchTime), (uint16_t)(adder>>16), (uint16_t)adder);
	f_sync(&Fil);
}


inline void BenchmarkDump(uint16_t SizeK){

	Slowdown       = Systype<3?1:0;
	uint32_t adder = 0;
	uint8_t      i = 1;

	if(Systype == 1) ResetPeskyFlash(); ///< Make sure Flash enters normal mode on Trionic 5.

	f_close(&Fil);
	printf("Dump..  ");
	BenchTime = 65535;

	if(f_open(&Fil, "New.bin", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {OhNoes(2, "Could not create f"); return;}
	Exec_ReadCMD(0,READ32_BDM);
	*(uint32_t*)&Fbuf[0] = bdmresp16<<24|(bdmresp16>>8)<<16|(bdmresp32&0xFF)<<8|bdmresp32>>8;
	adder = (bdmresp32&0xFF) + (bdmresp32>>8) + (bdmresp16&0xFF) + (bdmresp16>>8);

	do{ do{ ShiftData(DUMP32_BDM);
			ShiftWait(&bdmresp32);
			ShiftWait(&bdmresp16);
			adder += (bdmresp32&0xFF) + (bdmresp32>>8) + (bdmresp16&0xFF) + (bdmresp16>>8);
			*(uint32_t *)&Fbuf[i++] = bdmresp16<<24|(bdmresp16>>8)<<16|(bdmresp32&0xFF)<<8|bdmresp32>>8;
		}while(i);

		if(f_write(&Fil, &Fbuf, 1024, &bw)!=FR_OK) OhNoes(3, "SDC Int Err");
		LED_Trigger(ACCLED);
	}while(--SizeK);

	printf("%ums \n\rCsum32  %04X%04X\n\r", (uint16_t)(65535 - BenchTime), (uint16_t)(adder>>16), (uint16_t)adder);
	f_sync(&Fil);
}

// Write flash data
inline uint8_t LDRWrite(uint8_t start, uint16_t SizeK){

	uint16_t K = 0;
	uint8_t  i = 0;
	uint16_t temp;

	f_read(&Fil, &Fbuf, TxBUFSIZE, &bw);     ///< Prefill buffer.
	Exec_WriteCMD(0, W_DREG_BDM,         1); ///< Store command in D0.
	Exec_WriteCMD(0, W_AREG_BDM, start<<16); ///< Store address in A0. (Will auto-increment in the driver)

	do{ if(bw!=TxBUFSIZE) return 0;                ///< Check number of read bytes.
		Exec_WriteCMD(TxBUFSTART, WRITE32_BDM, 0); ///< Ugly solution to start the fill command at the right address..

		do{ Exec_FillCMD_p32(&Fbuf[i++]);
		}while(i); ///< Let it overflow to 0.

		Exec_WriteCMD(0, W_SREG_BDM, TxDRVSTART);  ///< Set PC to start of driver.
		ShiftData(0);
		ShiftData_s(BDM_GO);
		LED_Trigger(ACCLED);

		if(++K!=SizeK)                             ///< Do not prefill buffer if this is the last iteration.
			f_read(&Fil, &Fbuf, TxBUFSIZE, &bw);

		for(i=20; i>0; i--)                        ///< De-bounce. Todo: Replace these with direct I/O
			if(ReadPin(P_RST) && !ReadPin(P_FRZ) && i < 20) i += 2;

		ShiftData(R_DREG_BDM); ///< Read D0 for status.
		ShiftWait(&temp);      ///< Discard..
		ShiftWait(&temp);      ///< And read the real word we want

	}while(K<SizeK && temp == 1);

	return temp == 1 ? 1:0;
}




// Write flash data
uint8_t LDRWriteMCP(){

	uint32_t Addr = 0;
	uint16_t Len  = 1024;
	uint8_t  i    = 0;


	// 0x00.. -Check
	// 0xFF.. -Check
	// 0xAA.. -Check
	// 0x55.. -Check (Made it angry)
	// 0x55AA55AA -


	uint32_t Testpat = 0x55AA55AA;
	uint32_t t;

	for(t=0; t<256; t++)
		Fbuf[t] = Testpat;
	// f_read(&Fil, &Fbuf, Len, &bw);   ///< Prefill buffer.
	Exec_WriteCMD(0, W_DREG_BDM, 1); ///< Store command in D0

	do{ // if(bw!=Len){ printfLCD("NoF",0x07E0); return 0;}
		Exec_WriteCMD(0, W_SREG_BDM, MCPDRVSTART);     ///< Set PC to start of driver
		Exec_WriteCMD(MCPBUFSTART, WRITE32_BDM, Addr); ///< Store address in first buffer location

		do{ Exec_FillCMD_p32(&Fbuf[i++]);
		}while(i); ///< Let it overflow to 0.

		ShiftData(0);
		ShiftData_s(BDM_GO);


		sleep(10);



		LED_Trigger(ACCLED);
		Addr+=1024;
		if(Addr == 0x40000)
			Len = 256;
		if(Addr <0x40100){
			for(t=0; t<256; t++)
				Fbuf[t] = Testpat;
			//f_read(&Fil, &Fbuf, Len, &bw);
		}
		for(i=200; i>0; i--) ///< De-bounce. Todo: Replace these with direct I/O
			if(ReadPin(P_RST) && !ReadPin(P_FRZ) && i < 254) i += 2;

	    // if(!MiscTime){ printf(".."); MiscTime=8000; }
	    Exec_ReadCMD(0, R_DREG_BDM); ///< Read D0 for status
	}while(Addr <0x40100 && bdmresp16 == 1);

	return bdmresp16 == 1 ? 1:0;
}

/* Commands:
 * 3: Init hardware
 * 2: Format flash
 * 1: Write data
 *
 * Stores result in D0
 * 1: OK
 * 0: Fail*/
uint8_t LDRDemand(uint8_t Command, uint32_t drvstart){

	uint8_t i;

	Exec_WriteCMD(0, W_DREG_BDM  ,    Command); ///< Store command in D0
	Exec_WriteCMD(0, W_SREG_BDM  ,   drvstart); ///< Set PC to start of driver
	ShiftData(0);
	ShiftData_s(BDM_GO);
	LED_Trigger(ACCLED);
	for(i=20; i>0; i--) ///< De-bounce. Todo: Replace these with direct I/O
		if(ReadPin(P_RST) && !ReadPin(P_FRZ) && i < 254) i += 2;

	Exec_ReadCMD_s(0, 0, R_DREG_BDM); ///< Read D0 for status
	return bdmresp16 == 1 ? 1:0;
}

uint8_t UploadDRV(){

	uint16_t Location = 0;

	Exec_WriteCMD(TxDRVSTART-4, WRITE32_BDM, 0);
	do{ Exec_FillCMD_p32((uint32_t *)&TxDriver_bin[Location]);
		Location+=4;
	}while(Location<(Txloaderbytes));

	// This is useless atm..
	return Location >= Txloaderbytes ? 1:0;
}


uint8_t UploadDRVMCP(){

	uint16_t Location = 0;

	Exec_WriteCMD(MCPDRVSTART-4, WRITE32_BDM, 0);
	do{ Exec_FillCMD_p32((uint32_t *)&driver_bin[Location]);
		Location+=4;
	}while(Location<mcploaderbytes);

	// This is useless atm..
	return Location >= mcploaderbytes ? 1:0;
}

void Exec_DumpCMD(){

	ShiftData(DUMP32_BDM);
	ShiftWait(&bdmresp32);
	ShiftWait(&bdmresp16);
}
// Check for unreliable connection
uint8_t VerifyLDR(){

	uint16_t Location = 0;

	Exec_ReadCMD(TxDRVSTART-4, READ32_BDM);
	do{ Exec_DumpCMD();
		if(bdmresp32!=(TxDriver_bin[Location]<<8|TxDriver_bin[Location+1])||
				bdmresp16!=(TxDriver_bin[Location+2]<<8|TxDriver_bin[Location+3])) return 0;
		Location+=4;
	}while(Location<Txloaderbytes);

	return 1;
}



// Check for unreliable connection
uint8_t VerifyLDRMCP(){

	uint16_t Location = 0;

	Exec_ReadCMD(MCPDRVSTART-4, READ32_BDM);
	do{ Exec_DumpCMD();
		if(bdmresp32!=(driver_bin[Location]<<8|driver_bin[Location+1])||
				bdmresp16!=(driver_bin[Location+2]<<8|driver_bin[Location+3])) return 0;
		Location+=4;
	}while(Location<mcploaderbytes);

	return 1;
}

uint8_t ResetSafe(){
	ResetTarget();
	return 0;
}

// This one will NOT be fun; Trionic 5
uint8_t T5Shenanigans(){



	uint8_t Copies;
	uint8_t  i;




	// char fname[] = "t52.bin";
	char fname[] = "t5.bin";
	// printf("Int5\n");
	// Trionic 5 can be "overclocked" to 20,1 MHz if we're dealing with toggle-flash
	Exec_ReadCMD(0, R_DREG_BDM+6); ///< Read D6 to see if it is.
	if(bdmresp16 != 1){            ///< Yeah, baby! Go full blast
		// Exec_WriteCMD_s(0xFF, 0xFA04, WRITE16_BDM,   0, 0xD300);
		printfLCD("Toggle flash found:\n\rGoing 20,9 MHz\n\r",0x07FF);
		sleep(5);
	}

	Exec_ReadCMD(0, R_DREG_BDM+9); ///< Read A1 to check size
	uint16_t SizeK = bdmresp32 * 64;
	// printf("FlashSize: %u\n\r", SizeK);
	// char fname[] = "t5.bin";
	uint16_t FSize = 256;

	if (SizeK == 512 || SizeK == 256){}
	else if (SizeK == 128)
	{
		// char fname[] = "t52.bin";
		FSize = 128;
	}
	else
		return 0;

	// while (1);


	// Exec_ReadCMD(0x20000, READ16_BDM);
	// printf("Addr0: %04X\n\r", bdmresp16);

	// Exec_ReadCMD(0x20002, READ16_BDM);
	// printf("Addr2: %04X\n\r", bdmresp16);

	// Exec_ReadCMD(0x20004, READ16_BDM);
	// printf("Addr4: %04X\n\r", bdmresp16);

	Copies = SizeK / FSize; // Catch too big files before this!

	Exec_ReadCMD(0, R_AREG_BDM+1); // Read A1
	printf("A1 %02X%04X\n\r", bdmresp32, bdmresp16);

	Exec_ReadCMD(0, R_DREG_BDM + 6); // Read A1
	printf("D6 %02X%04X\n\r", bdmresp32, bdmresp16);

	printf("Erase..");
	// LDRDemand(2, TxDRVSTART);
	if(!LDRDemand(2, TxDRVSTART)) return 0;
	printfLCD(" OK ",0x07E0);


	Exec_ReadCMD(0, R_AREG_BDM); // Read A0
	printf("A0 %02X%04X\n\rFlash..", bdmresp32, bdmresp16);
	// while (1) ;

	for(i=0; i<Copies; i++){
		f_close(&Fil);

		if(f_open( &Fil, fname, FA_OPEN_EXISTING | FA_READ ) != FR_OK || !LDRWrite(i*(FSize/64), FSize))
			return 0;
		// printf("It\n\r");
	}
	f_close(&Fil);
	return 1;
}

// This function was REALLY nice once; Then came Trionic 5..
uint8_t Flash(uint16_t  SizeK){

	BenchTime = 65535;
	Slowdown = Systype < 3 ? 1:0;


	// Exec_ReadCMD(0, READ16_BDM);
	// printf("Addr0: %04X\n\r", bdmresp16);




	printf("Upload");
	if(!UploadDRV() || !VerifyLDR()) return ResetSafe();
	printfLCD("  Verified\n\r",0x07E0);

	printf("Init.. ");
	if(!LDRDemand(3, TxDRVSTART)) return ResetSafe();
	printfLCD(" OK ",0x07E0);
	Exec_ReadCMD(0, R_DREG_BDM+7); // Read D7
	printf("D7   %04X\n\r", bdmresp16);
	Exec_ReadCMD(0, R_DREG_BDM+6); // Read D7
	printf("D6   %04X\n\r", bdmresp16);
	Exec_ReadCMD(0, R_DREG_BDM+3); // Read D7
	printf("D3   %04X\n\r", bdmresp16);
	// while (1);

	if(Systype == 1){
		if(!T5Shenanigans()) return 0;
	}else{

		printf("Erase..");
		if(!LDRDemand(2, TxDRVSTART)) return 0;


		printfLCD(" OK ",0x07E0);
		Exec_ReadCMD(0, R_AREG_BDM + 1); // Read A0
		printf("A1 %02X%04X\n\r", bdmresp32, bdmresp16);
		Exec_ReadCMD(0, R_AREG_BDM); // Read A0

		printf("A0 %02X%04X\n\rFlash..", bdmresp32, bdmresp16);
		if(!LDRWrite(0, SizeK)) return 0;
	}
	printf(" %ums\n\r", (uint16_t)(65535 - BenchTime));
	// SizeK = 128;
	// BenchmarkDump(SizeK);

	ResetTarget();
	return 1;
}

uint8_t FlashMCP(){

	BenchTime=65535;
	Slowdown = 0;

	printf("Upload");
	if(!UploadDRVMCP() || !VerifyLDRMCP())  return 0;
	printfLCD("  Verified\n\r",0x07E0);

	printf("Init.. ");
	if(!LDRDemand(3, MCPDRVSTART)) return 0;// Ask loader to configure everything
	printfLCD(" OK\n\r",0x07E0);

	printf("Erase..");
	if(!LDRDemand(2, MCPDRVSTART)) return 0; // Format flash
	printfLCD(" OK\n\r",0x07E0);

	printf("Flash..");
	if(!LDRWriteMCP()) return 0;
	printfLCD(" OK\n\r",0x07E0);
	printf("%ums\n\r", (uint16_t)(65535 - BenchTime));

	DumpMCP();
	ResetTarget();
	return 1;
}

uint8_t PrepforBDM(){

	InitBDMpins();

	do{	printf("R.");
		ResetTarget();
		printf("S.");
	}while (!StopTarget());
	printf("\n\rTarget stopped\n\r");

	PrepT();

	return 1; // for now..
}
