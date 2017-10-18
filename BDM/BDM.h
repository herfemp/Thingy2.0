#ifndef BDM_H_
#define BDM_H_

uint8_t  Attn;
uint16_t bdmresp;
uint16_t bdmresp16;
uint16_t bdmresp32;

uint8_t StopTarget();
uint8_t ResetTarget();
void InitBDMpins();

void PrepT();
void ShiftData_s(uint16_t package);
void Exec_WriteCMD_s(uint16_t AddrH, uint16_t AddrL, uint16_t cmd, uint16_t DataH, uint16_t DataL);
void Exec_ReadCMD_s(uint16_t AddrH, uint16_t AddrL, uint16_t cmd);
inline void Exec_DumpCMD();


///< BDM Commands >///

///< Run CPU
#define BDM_GO		0x0c00
///< Reset peripherals
#define BDM_RESET	0x0400

///< D/A reg
#define R_DREG_BDM	0x2180
#define W_DREG_BDM	0x2080
///< 0-7 = Data Reg
///< 8-F = Address Reg

#define R_AREG_BDM	R_DREG_BDM + 8
#define W_AREG_BDM	W_DREG_BDM + 8

///< Sysreg
#define R_SREG_BDM	0x2580
#define W_SREG_BDM	0x2480
///< 0 = PC
///< 1 = PCC?

///< 8 = Tempreg A
///< 9 = Fault Address Reg
///< A = Vector Base reg
///< B = Status reg
///< C = User Stack Pointer
///< D = Supervisor Stack Pointer
///< E = SFC
///< F = DFC

///< Misc..
#define NULL_BDM	0x0000
#define PATCH_USRC	0x0800

///< Read
#define READ8_BDM   0x1900
#define READ16_BDM  0x1940
#define READ32_BDM  0x1980
///< Read/Increment address
#define DUMP8_BDM 	0x1D00
#define DUMP16_BDM  0x1D40
#define DUMP32_BDM  0x1D80

///< Write
#define WRITE8_BDM  0x1800
#define WRITE16_BDM 0x1840
#define WRITE32_BDM 0x1880
///< Write/Increment address
#define FILL8_BDM 	0x1C00
#define FILL16_BDM  0x1C40
#define FILL32_BDM  0x1C80



///< Trionic 8 CPU2 (MCP)
uint16_t CMFIMCR_Enable_shadow;
uint16_t CMFIMCR_Enable;
uint16_t CMFIMCR_Stop;

///< CMFIMCR
#define CMFIMCRAddr  0xF800
#define CMFITSTAddr  0xF804
#define CMFIBAHAddr  0xF808
#define CMFIBALAddr  0xF80A
#define CMFICTL1Addr 0xF80C
#define CMFICTL2Addr 0xF80E

///< External clock @ 4 MHz
#define MCPClock 0xE000
/*  1 << 15 | // X (Divide by 2=0)
    6 << 12 | // W (Multiplier) ///< It did NOT like 7 so even 6 is probably a bit too much.
    0 <<  8 | // Y (Divider)
    0 <<  7 | // EDIV (Divide by 16=1)
    0 <<  5   // LOSCD */


///< Trionic 8 CPU1 (Main)
#define DPTMCR_Base 0xF680
#define FASRAM_Base 0xF6C0
#define TPU_Base    0xF800














#endif /* BDM_H_ */
