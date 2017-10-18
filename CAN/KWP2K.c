#include "../common.h"
#include "legion.bin.h"
uint8_t t7loadlen = sizeof(legion_bin);





uint8_t t7initmsg[] = {0x3F,0x81, 0x00,0x11,0x02,0x40,0x00,0x00};



uint8_t kwmsg[8]={0};
uint8_t databuffer[132] = {0}; ///< Jeeeez. Decrease this one. -> It seems Trionic only use 130/132?
///< Save resources


///< Not sure if these are needed after I found the problem
uint8_t RespDly =  0; ///< Response delay in ms
uint8_t Intfrmdel= 0; ///< Inter-frame delay in ms
uint8_t CMDDel	 = 0; ///< Delay between commands

uint8_t kwsys=0;
uint8_t Retries = 20;



///< ( data[1] )
/**************************///Chunk size?
//CMB_240::41:A1:08:2C:F0:03: 00:80
/**************///AddH, AddL
//CMB_240::00:A1:   00, 00,80,  00:00:00
///< ( data[3] - [5] )
///< Did it this way to prepare for a crazy idea I have. Still not sure if I can pull it off though.. (Nor if that system actually talks in a similar way) :)

///<Note: Size is not the actual bytes of the whole package, only the number of command bytes.
///</********************/size, S_ID, PIDH, PIDL/**********************/
uint8_t T7_Sec_ACC[]/**/={0x02, 0x27, 0x05};/*****////< 0: Req Security Acc
uint8_t T7_Sec_Key[]/**/={0x02, 0x27, 0x06};/*****////< 1: Send key
uint8_t T7_SUpRd[]/****/={0x03, 0x2C, 0xF0, 0x03}; ///< 2: Setup read
uint8_t T7_Rd[]/*******/={0x02, 0x21, 0xF0};/*****////< 3: Read data
uint8_t T7_ErB0[]/*****/={0x02, 0x31, 0x52};/*****////< 4: Erase Bank 0
uint8_t T7_ErB1[]/*****/={0x02, 0x31, 0x53};/*****////< 5: Erase Bank 1

uint8_t T7_CnfEr0[]/***/={0x02, 0x3E, 0x52};/*****////< 6: Confirm.. Bank 0? Found no trace of this in the logs but it should theoretically exist
uint8_t T7_CnfEr1[]/***/={0x02, 0x3E, 0x53};/*****////< 7: Confirm.. Bank 1? (Could only find one in the log)
uint8_t T7_SUWr[]/*****/={0x01, 0x35};/***********////< 8: Setup Write
uint8_t T7_Wr[]/*******/={0x01, 0x3D};/***********////< 9: Write

uint8_t T7_EndSes[]/***/={0x01, 0x82};/***********////< A: End Session
uint8_t T7_Succ[]/*****/={0x40};/*****************////< B: Response should be S_ID + 0x40, it's otherwise a fail.
uint8_t T7_SysM[]/*****/={0xA1};/*****************////< C: System mode
uint8_t T7_ResECU[]/***/={0x02, 0x11, 0x01};/*****////< D: Reset ECU

uint8_t *Reqs_T7[]={
	T7_Sec_ACC,
	T7_Sec_Key,
	T7_SUpRd,
	T7_Rd,
	T7_ErB0,
	T7_ErB1,
	T7_CnfEr0,
	T7_CnfEr1,
	T7_SUWr,
	T7_Wr,
	T7_EndSes,
	T7_Succ,
	T7_SysM,
	T7_ResECU
};


///< ...
void DiehardAndDie(){

	printf("Died!!!!");
	while(1){;}
}


void CalcKey(uint8_t method){

	uint16_t seed;
	seed = databuffer[2] << 8 | databuffer[3];
	seed  = seed << 2 & 0xffff;
	seed ^= (method ? 0x4081 : 0x8142);
	seed -= (method ? 0x1f6f : 0x2356);
	databuffer[2] = seed >> 8 &0xff;
	databuffer[3] = seed &0xff;
}

///< Rewrite this one..
void SendKWData(uint8_t Nobytes){
	uint8_t i=0;
	uint8_t frames=0;
	uint8_t c=0;

	kwmsg[1]=Reqs_T7[0xC][0]; ///< Sysmode

	if(Nobytes>5){
		i=Nobytes-5; ///< We can send 5 bytes in the first message.
		frames++;    ///< There went one frame
	}
	///< Figure out how many frames this transmission needs
	do{ frames++;
		if(i>5) i-=6;
		else i=0;
	} while (i);

	kwmsg[0]=0x40+(frames-1);
	kwmsg[2]=Nobytes;

	for (i=3; i<8; i++){
		if((i-3)<Nobytes) kwmsg[i]=databuffer[i-3];
		else kwmsg[i]=0;
	}
	SendCAN(0x240, kwmsg);
	frames--;
	c+=5;
	if(frames){
		do{ kwmsg[0]=(frames-1);
			for (i=2; i<8; i++){
				if(c<Nobytes) kwmsg[i]=databuffer[(i-2)+c];
				else kwmsg[i]=0;
			}
			sleep(5);
			SendCAN(0x240, kwmsg);
			frames--;
			c+=6;
		} while (frames);
}}

void SendAck(uint8_t Ack){

	kwmsg[0]=0x40; ///< 1-message frame
	kwmsg[1]=0xA1; ///< Already set; system mode.
	kwmsg[2]=0x3F;
	kwmsg[3]=Ack; ///< Which frame number to acknowledge
	kwmsg[4]=0;
	kwmsg[5]=0;
	kwmsg[6]=0;
	kwmsg[7]=0;
	sleep(2);
	SendCAN(0x266, kwmsg); ///< Send ack

}


///< It may be resilient while sending. Not so much during reception. :) NTS: Fix this.
uint8_t ReceiveKWData(uint8_t Request){
	uint8_t i;
	uint8_t c;   ///< c is the number of bytes left in this frame
	uint8_t e=0; ///< e should tell us number of received bytes

	///< Received something.
	if(waitmsg(0x258)){

		///< Fail:
		if (RxMessage.Data[3]!= Reqs_T7[Request][1] + Reqs_T7[0xB][0]){

			///< No matter what is received, one should ALWAYS acknowledge the reception.
			SendAck(RxMessage.Data[0]-0x40);

		///< Success:
		}else{

			///< How many bytes will we receive?
			c=RxMessage.Data[2];
			for (i=3; i<8; i++){
				if(c){
					databuffer[e]=RxMessage.Data[i];
					c--;   e++;
			}}
			SendAck(RxMessage.Data[0]-0x40);

			///< More data incoming?
			while (c){
				if(waitmsg(0x258)){
					for (i=2; i<8; i++){
						if(c){
							databuffer[e]=RxMessage.Data[i];
							c--;   e++;
					}}
					SendAck(RxMessage.Data[0]);
			}}
	}}
	return e; ///< Return number of received bytes or 0 if we failed.
}


void LoadSendCMD(uint8_t Request, uint8_t Nobytes){
	uint8_t i;

	///< Load command into buffer.
	for (i=0; i<Reqs_T7[Request][0]; i++){
		databuffer[i] = Reqs_T7[Request][i+1];
	}
	SendKWData(Nobytes);
}

uint8_t HandleReq(uint8_t Request, uint8_t AutoRetry, uint8_t Nobytes){

	uint8_t c;
	uint8_t retrs=0;

	if(!AutoRetry){
		LoadSendCMD(Request, Nobytes);
		return ReceiveKWData(Request);
	}else{
		do{ LoadSendCMD(Request, Nobytes);
			c=ReceiveKWData(Request);
			if(!c) sleep(1000);
			retrs++;
		} while (!c && retrs<Retries);
	}

	return c;
}



uint8_t InitSes(){

	//InitCAN(1, 500000); ///< Reset the can-controller to empty buffers
	// SetCANFilter(1);			 ///< Enable T7-filters
	sleep(100);									 ///< Let the controller rest. -Only necessary on mcp2515.

	SendCAN(0x220, t7initmsg);
	return waitmsg(0x238);
}

///< Request Sec access.
uint8_t GetSecACC(){


	if(!HandleReq(0, 0, 2)) ;// return 0;

	CalcKey(0); ///< Try method 1
	if(!HandleReq(1, 0, 4)) {
		HandleReq(0, 1, 2);
		CalcKey(1); ///< Try method 2
		if(!HandleReq(1, 0, 4)) return 0;
	}
	return 1;
}



void FlashT7();
void TestT7(){



	FlashT7();

	/*
	if (InitSes())
		printf("Ses OK\n\r");

	else
		printf("Ses Failed");



	if (GetSecACC())
		printf("We're in!");

	else
		printf("Failed");
*/
}


//ID__GATE_PCI_req_Addr___Len
//240 41a1  07  3d f05fac 02
//240 00a1 010500000000
void PrepT7Write(uint32_t Addr, uint32_t EndAddr){

	databuffer[1]=(Addr>>16)&0xFF;
	databuffer[2]=(Addr>>8)&0xFF;
	databuffer[3]=Addr&0xFF;
	databuffer[4]=0; ///< What is this byte?

	databuffer[5]=(EndAddr>>16)&0xFF;
	databuffer[6]=(EndAddr>>8)&0xFF;
	databuffer[7]=EndAddr&0xFF;

	if(!HandleReq(8, 0, 8)) DiehardAndDie();
}


void EraseT7(){


//	if(!HandleReq(4, 1, 2)) DiehardAndDie(); // Erase
//	if(!HandleReq(5, 1, 2)) DiehardAndDie();

//	if(!HandleReq(6, 1, 2)) DiehardAndDie(); // Confirm erase
//	if(!HandleReq(7, 1, 2)) DiehardAndDie();

}
uint8_t test[8] = {
	0x10,
	0xf0,
	0x36,
	0x00,
	0x00,
	0x02,
	0x00,
	0x00

};


void SendReq(uint16_t Gate, uint16_t PCI, uint8_t REQ, uint16_t Sub, uint8_t test){

	uint8_t Arr[8];
	uint8_t i;
	sleep(50);
	// Clean array
	for(i=8; i>0; i--)
		Arr[i] = 0;

	// Sorry about the stupid name.
	if(Gate){
		*(uint16_t *)&Arr = Gate<<8|Gate>>8; // Let's break some rules ;)
		i=2;
	}
	if(PCI>0xFF){
		*(uint16_t *)&Arr[i++] = PCI<<8|PCI>>8;
	}else Arr[i]=PCI&0xff;

	Arr[++i]=REQ;
	if(Sub>0xFF){
		*(uint16_t *)&Arr[++i] = Sub<<8|Sub>>8;
	}else Arr[++i]=Sub&0xFF;
	Arr[++i]=test;
//	Arr[6] = 0x10;
//	Arr[7] = test;
	sleep(50);
	SendCAN(0x240, Arr);
}

//Found:


// F0 1000 free?



uint8_t ReqDataTrans(uint32_t Addr, uint32_t Len){




	return 1;
}


// 0x33 securityAccessDenied
void FlashT7(){

	///< Set some stuff up:
	uint32_t Addr    = 0x04E800;
	uint32_t EndAddr = 0x04E880;

	uint8_t Blocksize=0x80;


	uint16_t i;
	printf("StartingSes\n\r");
	if (InitSes()){
		printf("Ses Started\n\rReq SecAcc\n\r");
		if(GetSecACC()){
			printf("We're in!\n\r");
			// 0xFF E800
//			SendReq(0x40A1, 1, 0x3E, 0x00, 0x00); // TesterPresent             OK
/*			SendReq(0x40A1, 2, 0x10, 0x80, 0x00); // Init ses. SubParams are unknown
			SendReq(0x40A1, 2, 0x10, 0x81, 0x00); // Standard Ses
			SendReq(0x40A1, 2, 0x10, 0x82, 0x00); // ??
			SendReq(0x40A1, 2, 0x10, 0x83, 0x00); // ??
			SendReq(0x40A1, 2, 0x10, 0x84, 0x00); // Nosup? End of line sup mode
			SendReq(0x40A1, 2, 0x10, 0x85, 0x00); // Nosup? Download mode (To ECU)
			SendReq(0x40A1, 2, 0x10, 0x86, 0x00); // Nosup? Upload mode   (From ECU)

			*/

//			SendReq(0x40A1, 2, 0x28, 0x80, 0x00); // CommunicationControl      Not sup
//			SendReq(0x40A1, 2, 0x1A, 0x90, 0x00); // ReadECUIdentification     OK
//			SendReq(0x40A1, 2, 0x31, 0x50, 0x00); // Routine WHAT?? 50,51      OK
//			SendReq(0x40A1, 1, 0x33, 0x52, 0x00); // Request status of routine OK

			/* Supports a metric sh!tload of subparams: 0x10 - 0x20. 0x15, 1A, 1E gave special responses.. */
//			SendReq(0x40A1, 1, 0x30, 0xYY, 0x00); // InputOutputControlByLocalID



//			for(i=0; i<0x50; i++)
//				SendReq(0x40A1, i, 0x34, 0x00, 0x00); // InputOutputControlByLocalID

			sleep(100);
/*

		SendReq(0x40A1, 2, 0x31, 0x40, 0x00);
		sleep(100);
		SendReq(0x40A1, 2, 0x31, 0x41, 0x00);
		sleep(100);
		SendReq(0x40A1, 2, 0x31, 0x42, 0x00);
		sleep(100);
		SendReq(0x40A1, 2, 0x31, 0x43, 0x00);
		sleep(100);
		SendReq(0x40A1, 2, 0x31, 0x50, 0x00);
*/





		// 0xFFE800: DPTRAM
		sleep(100);

	/*	uint8_t Arr[8] = {
				0x4B,0xA1, // "Gate"
				0x45,0x3D, // PCI and REQ

				0xF0,0x80,0x00,

				0x40
		};
		SendCAN(0x240, Arr);
		sleep(10);

		for(i=0x0B; i>0; i--){
			uint8_t Arr[8] = {
				i-1,0xA1, // "Gate"
				0,0,0,0,0,0
			};
			SendCAN(0x240, Arr);
			sleep(10);
		}


		SendAck(0x80);*/
		sleep(10);


		// Dowload
		uint8_t Arr3[8] = {
				0x41,0xA1,
				0x08,0x34,
				0xF0,0x80,0x00,
				0x00
		};
		SendCAN(0x240, Arr3);

		sleep(10);
		uint8_t Arr4[8] = {
				0x00,0xA1,
				0x00,0x10,0x00,
				0x00,0x00,0x00
		};
		SendCAN(0x240, Arr4);

		sleep(10);
		SendAck(0x80);




		uint16_t Location = 0;
		uint8_t  sndarr[8];
		uint8_t Transf = 0;
		// = Sub<<8|Sub>>8;
		do{ sndarr[0] = 0x55;
			sndarr[1] = 0xA1;
			sndarr[2] = 0x81;
			sndarr[3] = 0x36;
			sndarr[4] = Transf++;
			sndarr[5] = 0xF0;
			sndarr[6] = 0x80;//
			sndarr[7] = 0x00;//

			// 0x0065 0x0F71
			// E671 0C92
			SendCAN(0x240, sndarr);
			Location += 4;

			// if(!waitmsg(0x258))
			// DiehardAndDie();

			sleep(10);


			for(i=0x15; i>0; i--){

				sndarr[0] = i-1;
				sndarr[1] = 0xA1;

				sndarr[2] = 1;//legion_bin[Location];
				sndarr[3] = 2;//legion_bin[Location+1];
				sndarr[4] = 3;//legion_bin[Location+2];
				sndarr[5] = 4;//legion_bin[Location+3];
				sndarr[6] = 5;//legion_bin[Location+4];
				sndarr[7] = 6;//legion_bin[Location+5];

/*
				if(Location > t7loadlen && i == 1){
					// while(1);
					sndarr[6] = 0x1;
					sndarr[7] = 0x0;
				}*/


				SendCAN(0x240, sndarr);
				Location += 6;
				sleep(5);
			}

			if(!waitmsg(0x258)){
				printf("No 0x258\n\r");
				return;
			}
			SendAck(0x80);

		}while(Location < 0x100);

		sndarr[0] = 0x40;
		sndarr[1] = 0xA1;
		sndarr[2] = 0x02;
		sndarr[3] = 0x31;
		sndarr[4] = 0x50;
		sndarr[5] = 0xF0;
		sndarr[6] = 0x80;//
		sndarr[7] = 0x00;//

					// 0x0065 0x0F71
					// E671 0C92
		SendCAN(0x240, sndarr);



		}

	}
}
