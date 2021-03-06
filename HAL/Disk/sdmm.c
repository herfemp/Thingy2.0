/*------------------------------------------------------------------------/
/  Foolproof MMCv3/SDv1/SDv2 (in SPI mode) control module
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2013, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------/
  Features and Limitations:

  * Easy to Port Bit-banging SPI
    It uses only four GPIO pins. No complex peripheral needs to be used.

  * Platform Independent
    You need to modify only a few macros to control the GPIO port.

  * Low Speed
    The data transfer rate will be several times slower than hardware SPI.

  * No Media Change Detection
    Application program needs to perform a f_mount() after media change.

/-------------------------------------------------------------------------*/

#include <stdint.h>
#include "diskio.h"		/* Common include file for FatFs and disk I/O layer */
#include "../HAL.h"
#include "../../common.h"
///< Modified this code to _not_ use POS bitbanging. It's still slow but a lot faster than before. :)
// Oh and DMA on writes ;D

#define	CS_H()		SETBIT(((GPIO_TypeDef *) (GPIOA_BASE))->BSRR,  0x0010);//WritePin (SDCARD_CS, 1)//PORTD |= (1 << PORTD2)
#define CS_L()		SETBIT(((GPIO_TypeDef *) (GPIOA_BASE))->BRR,  0x0010); //WritePin (SDCARD_CS, 0)//PORTD &= ~(1 << PORTD2)


static void dly_us(uint32_t del){
	uint32_t tmr;
	uint32_t us;
	for(us=0; us<del; us++){
		for(tmr=0; tmr<2; tmr++){ __asm("nop"); } // More like I want more speed..
	}
}


/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/

/* MMC/SD command (SPI mode) */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define CMD13	(13)		/* SEND_STATUS */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */


static
DSTATUS Stat = STA_NOINIT;	/* Disk status */

static
BYTE CardType;			/* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */

/*-----------------------------------------------------------------------*/
/* Transmit bytes to the card (bitbanging)                               */
/*-----------------------------------------------------------------------*/

static
void xmit_mmc (
	const BYTE* buff,	/* Data to be sent */
	UINT bc				/* Number of bytes to send */
)
{
	BYTE d;
	SETBIT(((GPIO_TypeDef *) (GPIOA_BASE))->BRR,  0x0010);
do {


	//while(!(SPI1->SR & SPI_I2S_FLAG_TXE)); //
	SPI1->DR = *buff++;
	while(!(SPI1->SR & SPI_I2S_FLAG_RXNE));


	d= SPI1->DR;
} while (--bc);
	SETBIT(((GPIO_TypeDef *) (GPIOA_BASE))->BSRR,  0x0010);
	d=d;
}


/*-----------------------------------------------------------------------*/
/* Receive bytes from the card (bitbanging)                              */
/*-----------------------------------------------------------------------*/

static
void rcvr_mmc (
	BYTE *buff,	/* Pointer to read buffer */
	UINT bc		/* Number of bytes to receive */
){

	SETBIT(((GPIO_TypeDef *) (GPIOA_BASE))->BRR,  0x0010);
	do {//while(!(SPI1->SR & SPI_I2S_FLAG_TXE)); //
		SPI1->DR = 0xff;
		while(!(SPI1->SR & SPI_I2S_FLAG_RXNE));
		*buff++ = SPI1->DR;
	} while (--bc);
	SETBIT(((GPIO_TypeDef *) (GPIOA_BASE))->BSRR,  0x0010);
}



/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

static
int wait_ready (void)	/* 1:OK, 0:Timeout */
{
	BYTE d;
	UINT tmr;


	for (tmr = 500000; tmr; tmr--) {	/* Wait for ready in timeout of 500ms (+00)*/
		rcvr_mmc(&d, 1);
		if (d == 0xFF) break;
		dly_us(100);
	}

	return tmr ? 1 : 0;
}



/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

static
void deselect (void)
{
	BYTE d;

	CS_H();				/* Set CS# high */
	rcvr_mmc(&d, 1);	/* Dummy clock (force DO hi-z for multiple slave SPI) */
}



/*-----------------------------------------------------------------------*/
/* Select the card and wait for ready                                    */
/*-----------------------------------------------------------------------*/

static
int select (void)	/* 1:OK, 0:Timeout */
{
	BYTE d;

	CS_L();				/* Set CS# low */
	rcvr_mmc(&d, 1);	/* Dummy clock (force DO enabled) */
	if (wait_ready()) return 1;	/* Wait for card ready */

	deselect();
	return 0;			/* Failed */
}



DMA_InitTypeDef     DMA_InitStructureTX;
// DMA_InitTypeDef     DMA_InitStructureRX;
void preinit(){

	// TX
	DMA_InitStructureTX.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
//	DMA_InitStructureTX.DMA_MemoryBaseAddr     =
	DMA_InitStructureTX.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructureTX.DMA_BufferSize = 512;
	DMA_InitStructureTX.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructureTX.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructureTX.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructureTX.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructureTX.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructureTX.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructureTX.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel3, &DMA_InitStructureTX); // Init the constant stuff

/*
	// RX
	DMA_InitStructureRX.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
//	DMA_InitStructureRX.DMA_MemoryBaseAddr     =
	DMA_InitStructureRX.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructureRX.DMA_BufferSize = 512;
	DMA_InitStructureRX.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructureRX.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructureRX.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructureRX.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructureRX.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructureRX.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructureRX.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel2, &DMA_InitStructureRX);*/

}


/*-----------------------------------------------------------------------*/
/* Receive a data packet from the card                                   */
/*-----------------------------------------------------------------------*/
/*
inline void DMAr(BYTE *buff){

	uint16_t Stock = SPI1->CR1;
	SPI1->CR1 |= 0x0400;  // 0x8000

	DMA_InitStructureRX.DMA_MemoryBaseAddr = (uint32_t)buff,
	DMA_Init(DMA1_Channel2, &DMA_InitStructureRX);

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);

	SETBIT(((GPIO_TypeDef *) (GPIOA_BASE))->BRR,  0x0010);
	DMA_Cmd(DMA1_Channel2, ENABLE);

	while(DMA_GetCurrDataCounter(DMA1_Channel2) !=0);

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);
	DMA_DeInit(DMA1_Channel2);

	SETBIT(((GPIO_TypeDef *) (GPIOA_BASE))->BSRR,  0x0010);
	SPI1->CR1 = Stock;
	printf("Not stuck");

}*/

static
int rcvr_datablock (	/* 1:OK, 0:Failed */
	BYTE *buff,			/* Data buffer to store received data */
	UINT btr			/* Byte count */
)
{
	BYTE d[2];
	UINT tmr;


	for (tmr = 100000; tmr; tmr--) {	/* Wait for data packet in timeout of 100ms (+00)*/
		rcvr_mmc(d, 1);
		if (d[0] != 0xFF) break;
		dly_us(100);
	}
	if (d[0] != 0xFE) return 0;		/* If not valid data token, return with error */

	rcvr_mmc(buff, btr);			/* Receive the data block into buffer */
	rcvr_mmc(d, 2);					/* Discard CRC */

	return 1;						/* Return with success */
}



/*-----------------------------------------------------------------------*/
/* Send a data packet to the card                                        */
/*-----------------------------------------------------------------------*/

/* SPI */

static
int xmit_datablock (	/* 1:OK, 0:Failed */
	const BYTE *buff,	/* 512 byte data block to be transmitted */
	BYTE token			/* Data/Stop token */
)
{
	BYTE d[3];


	if (!wait_ready()) return 0;

	d[0] = token;
	xmit_mmc(d, 1);				/* Xmit a token */
	if (token != 0xFD) { /* Is it data token? */

		uint16_t Stock = SPI1->CR1;
		DMA_InitStructureTX.DMA_MemoryBaseAddr = (uint32_t)buff;
		SPI1->CR1 |= 0xC000;        // Temporarily force TX only (You must otherwise read every response to not crash the spi controller)
		DMA_Init(DMA1_Channel3, &DMA_InitStructureTX);
		SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
		SETBIT(((GPIO_TypeDef *) (GPIOA_BASE))->BRR,  0x0010); // Select card...
		DMA_Cmd(DMA1_Channel3, ENABLE); // And go!

		while(DMA_GetCurrDataCounter(DMA1_Channel3) !=0);

		SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
		DMA_DeInit(DMA1_Channel3);
		SETBIT(((GPIO_TypeDef *) (GPIOA_BASE))->BSRR,  0x0010); // Deselect card.
		SPI1->CR1 = Stock;// Restore full io capability

		// xmit_mmc(buff, 512);	/* Xmit the 512 byte data block to MMC */ //1.7-1.8

		//rcvr_mmc(d, 2);			/* Xmit dummy CRC (0xFF,0xFF) */
		//rcvr_mmc(d, 1);			/* Receive data response */
		// Receive crc and response at once instead
		rcvr_mmc(d, 3);
		if ((d[2] & 0x1F) != 0x05)	/* If not accepted, return with error */
			return 0;
	}

	return 1;
}



/*-----------------------------------------------------------------------*/
/* Send a command packet to the card                                     */
/*-----------------------------------------------------------------------*/

static
BYTE send_cmd (		/* Returns command response (bit7==1:Send failed)*/
	BYTE cmd,		/* Command byte */
	DWORD arg		/* Argument */
)
{
	BYTE n, d, buf[6];


	if (cmd & 0x80) {	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
		cmd &= 0x7F;
		n = send_cmd(CMD55, 0);
		if (n > 1) return n;
	}

	/* Select the card and wait for ready except to stop multiple block read */
	if (cmd != CMD12) {
		deselect();
		if (!select()) return 0xFF;
	}

	/* Send a command packet */
	buf[0] = 0x40 | cmd;			/* Start + Command index */
	buf[1] = (BYTE)(arg >> 24);		/* Argument[31..24] */
	buf[2] = (BYTE)(arg >> 16);		/* Argument[23..16] */
	buf[3] = (BYTE)(arg >> 8);		/* Argument[15..8] */
	buf[4] = (BYTE)arg;				/* Argument[7..0] */
	n = 0x01;						/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;		/* (valid CRC for CMD0(0)) */
	if (cmd == CMD8) n = 0x87;		/* (valid CRC for CMD8(0x1AA)) */
	buf[5] = n;
	xmit_mmc(buf, 6);

	/* Receive command response */
	if (cmd == CMD12) rcvr_mmc(&d, 1);	/* Skip a stuff byte when stop reading */
	n = 10;								/* Wait for a valid response in timeout of 10 attempts */
	do
		rcvr_mmc(&d, 1);
	while ((d & 0x80) && --n);

	return d;			/* Return with the response value */
}



/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE drv			/* Drive number (always 0) */
)
{
	if (drv) return STA_NOINIT;

	return Stat;
}



/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE drv		/* Physical drive nmuber (0) */
)
{
	preinit();
	BYTE n, ty, cmd, buf[4];
	UINT tmr;
	DSTATUS s;


	if (drv) return RES_NOTRDY;

	dly_us(1000000);			/* 10ms (+00)*/

	for (n = 10; n; n--) rcvr_mmc(buf, 1);	/* Apply 80 dummy clocks and the card gets ready to receive command */

	ty = 0;
	if (send_cmd(CMD0, 0) == 1) {			/* Enter Idle state */
		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2? */
			rcvr_mmc(buf, 4);							/* Get trailing return value of R7 resp */
			if (buf[2] == 0x01 && buf[3] == 0xAA) {		/* The card can work at vdd range of 2.7-3.6V */
				for (tmr = 100000; tmr; tmr--) {			/* Wait for leaving idle state (ACMD41 with HCS bit) */ //(+00)
					if (send_cmd(ACMD41, 1UL << 30) == 0) break;
					dly_us(1000); //(+00)
				}
				if (tmr && send_cmd(CMD58, 0) == 0) {	/* Check CCS bit in the OCR */
					rcvr_mmc(buf, 4);
					ty = (buf[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* SDv2 */
				}
			}
		} else {							/* SDv1 or MMCv3 */
			if (send_cmd(ACMD41, 0) <= 1) 	{
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
			}
			for (tmr = 100000; tmr; tmr--) {			/* Wait for leaving idle state */ //(+00)
				if (send_cmd(cmd, 0) == 0) break;
				dly_us(1000);//(+00)
			}
			if (!tmr || send_cmd(CMD16, 512) != 0)	/* Set R/W block length to 512 */
				ty = 0;
		}
	}
	CardType = ty;
	s = ty ? 0 : STA_NOINIT;
	Stat = s;

	deselect();

	return s;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE drv,			/* Physical drive nmuber (0) */
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	DWORD sector,		/* Start sector number (LBA) */
	UINT count			/* Sector count (1..128) */
)
{
	BYTE cmd;


	if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert LBA to byte address if needed */

	cmd = count > 1 ? CMD18 : CMD17;			/*  READ_MULTIPLE_BLOCK : READ_SINGLE_BLOCK */

	if (send_cmd(cmd, sector) == 0) {

		do {if (!rcvr_datablock(buff, 512)) break;
			buff += 512;
		} while (--count);
		if (cmd == CMD18) send_cmd(CMD12, 0);	/* STOP_TRANSMISSION */

	}

	deselect();
	return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0) */
	const BYTE *buff,	/* Pointer to the data to be written */
	DWORD sector,		/* Start sector number (LBA) */
	UINT count			/* Sector count (1..128) */
)
{
	if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert LBA to byte address if needed */

	if (count == 1) {	/* Single block write */
		if ((send_cmd(CMD24, sector) == 0)	/* WRITE_BLOCK */
			&& xmit_datablock(buff, 0xFE))
			count = 0;
	}
	else {				/* Multiple block write */
		if (CardType & CT_SDC) send_cmd(ACMD23, count);
		if (send_cmd(CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!xmit_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(0, 0xFD))	/* STOP_TRAN token */
				count = 1;
		}
	}
	deselect();

	return count ? RES_ERROR : RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	BYTE n, csd[16];
	DWORD cs;


	if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;	/* Check if card is in the socket */

	res = RES_ERROR;
	switch (ctrl) {
		case CTRL_SYNC :		/* Make sure that no pending write process */
			if (select()) res = RES_OK;
			break;

		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
			if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
				if ((csd[0] >> 6) == 1) {	/* SDC ver 2.00 */
					cs = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
					*(DWORD*)buff = cs << 10;
				} else {					/* SDC ver 1.XX or MMC */
					n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
					cs = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
					*(DWORD*)buff = cs << (n - 9);
				}
				res = RES_OK;
			}
			break;

		case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
			*(DWORD*)buff = 128;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
	}

	deselect();

	return res;
}


