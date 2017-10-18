#include "../common.h"
#include "BDM.h"

void InitBDMpins(){

	SetPinDir(P_RST, 0); ///< Set all of them to floating inputs
	SetPinDir(P_BKPT,0);
	SetPinDir(P_FRZ, 2); ///< Except this fella; Input, pull down
	SetPinDir(P_DSI, 0);
	SetPinDir(P_DSO, 0);
}


uint8_t ResetTarget(){

	SetPinDir(P_RST, 1);
	SetPinDir(P_BKPT,1);


	WritePin(P_BKPT,1);
	WritePin(P_RST,1);
	sleep(50);
	WritePin(P_RST,0);

	InitBDMpins();

	///< Give it up to 500~ ms to wake up
	MiscTime=500;
	while(!ReadPin(P_RST) && MiscTime)	;

	//SetPinDir(P_RST, 0);

	return ReadPin(P_RST);
}


uint8_t StopTarget(){

	SetPinDir(P_BKPT,1);
	SetPinDir(P_DSI, 1);

	WritePin(P_BKPT,1);
	WritePin(P_BKPT,0);

	///< Give it up to 500~ ms to hit the brakes..
	MiscTime=500;
	while(!ReadPin(P_FRZ) &&  MiscTime)	;

	///< It appears someone is stubborn..  STOP!
	if(!ReadPin(P_FRZ)){
		WritePin(P_RST,0);
		sleep(100);
		WritePin(P_RST,1);

		MiscTime=500;
		while(!ReadPin(P_FRZ) &&  MiscTime)	;
	}
	// sleep(10);
	return ReadPin(P_FRZ);
}

///< SPI may be pretty effing fast, but it has a few drawbacks. This is a fallback used during certain conditions to circumvent problems.
void ShiftData_s(uint16_t package){

	uint8_t i;
	uint8_t d;
	Attn=0;
	for (i=17; i>0; i--){
		WritePin(P_BKPT, 0);
		if(i<17){
			WritePin(P_DSI, (package>>(i-1))&0x1 );
			bdmresp<<=1;  bdmresp|= ReadPin(P_DSO);
			}else{
			WritePin(P_DSI, 0);
			Attn=ReadPin(P_DSO);
		}
		WritePin(P_BKPT, 1);
		for(d=0; d<6; d++){__asm("nop");} ///<
	}
}

void Exec_WriteCMD_s(uint16_t AddrH, uint16_t AddrL, uint16_t cmd, uint16_t DataH, uint16_t DataL){

	ShiftData_s(cmd);

	if(AddrH || AddrL){
		ShiftData_s(AddrH);
		ShiftData_s(AddrL);
	}
	///< Bit 7 set; Send two payloads
	if(cmd&0x80)
		ShiftData_s(DataH);
	ShiftData_s(DataL);

	///< Fault checks, what is that? Time will tell if there is enough space left. Not gonna touch it yet..
	do{ ShiftData_s(0); }while(Attn);
}

void Exec_ReadCMD_s(uint16_t AddrH, uint16_t AddrL, uint16_t cmd){

	ShiftData_s(cmd);
	if(AddrH || AddrL){
		ShiftData_s(AddrH);
		ShiftData_s(AddrL);
	}

	if(cmd&0x80){
		do{ if(bdmresp && Attn)		;
			ShiftData_s(0);
		}while(Attn);
		bdmresp32=bdmresp;
	}

	do{ if(bdmresp && Attn)		;
		ShiftData_s(0);
	}while(Attn);

	bdmresp16=bdmresp;
}

