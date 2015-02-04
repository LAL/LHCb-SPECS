#ifndef _SPECS_H
#define _SPECS_H
/*                bibliotheque Specs                 *
*    création : CP 1er juillet 2003    LAL Orsay     *
*    évolution : CP 28 janvier 2004    LAL Orsay     *
*                CP    juin    2004    LAL Orsay     *
*                CG    mars    2005    CERN          *
*                      JtagWriteRead                 *
*                      JtagtoSpecs                   *
*                      SpecstoJtag                   *
*                CP    avril   2005    LAL Orsay     *
*                CP    fevrier 2006    LAL Orsay     *
*                      SpecsCardIdRead               *
*                CG    fevrier 2006    CERN          *
*                      evolutions des fonctions JTAG *
*                CP    mars 2006       LAL Orsay     *
*                      lecture FIFO réception        *
*                      autorisée par interruption    *
*                CP    avril   2006    LAL Orsay     */

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SPECS_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SPECS_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.


#ifdef PLX_LINUX
#ifdef SPECS_EXPORTS
#define SPECS_API
#else
#define SPECS_API extern
#endif
#else
#ifdef SPECS_EXPORTS
#define SPECS_API __declspec(dllexport)
#else
#define SPECS_API __declspec(dllimport)
#endif
#endif
#include "Plx.h"
#ifdef __cplusplus
extern "C" {
#endif


#define modI2C  0x00000
#define modJTAG  0x10000
#define modParallelBus  0x20000
#define modRegistre  0x30000

#define MAX_CARD 6

#define FifoLengthMax  0xFF
//#define BufI2cLengthMax  0x100
#define BufI2cLengthMax  0x70  //  compatible avec BufSpecsLengthMax
//#define TrameI2cLengthMax  0x1C

#define BufJtagMax  0x1F
#define BufSpecsLengthMax  300

SPECS_API U32 BufSpecs[BufSpecsLengthMax];
SPECS_API PLX_NOTIFY_OBJECT InterTab[MAX_CARD];

#ifdef SPECS_EXPORTS
  /*
SPECS_API BOOLEAN Spdemo=FALSE;
SPECS_API HANDLE EtatDev[MAX_CARD]={(HANDLE)-1,(HANDLE)-1,(HANDLE)-1,(HANDLE)-1,
                            (HANDLE)-1,(HANDLE)-1};
SPECS_API U32	 EtatDevCount[MAX_CARD]={0,0,0,0,0,0};
  */
SPECS_API BOOLEAN Spdemo=FALSE;
SPECS_API HANDLE EtatDev[NB_MASTER]={(HANDLE)-1,(HANDLE)-1,(HANDLE)-1,(HANDLE)-1,
				     (HANDLE)-1,(HANDLE)-1,(HANDLE)-1,(HANDLE)-1,(HANDLE)-1,(HANDLE)-1,
				     (HANDLE)-1,(HANDLE)-1,(HANDLE)-1,(HANDLE)-1,(HANDLE)-1,(HANDLE)-1,
				     };
  SPECS_API U32	 EtatDevCount[NB_MASTER]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#else
  /*MTQ
SPECS_API BOOLEAN Spdemo;
SPECS_API HANDLE EtatDev[MAX_CARD];
SPECS_API U32	 EtatDevCount[MAX_CARD];
  */
SPECS_API BOOLEAN Spdemo;
SPECS_API HANDLE EtatDev[NB_MASTER];
SPECS_API U32	 EtatDevCount[NB_MASTER];
#endif

#define JTAG_byteIndex(n)		((n)+3-2*((n)%4))
#define JTAG_calcNBytes(nBits)	((nBits - 1)/8 + 1)
#define JTAG_bitsLeft(nBits)		((nBits - 1)%8 + 1)

#define pSpecsmasterReset pSpecsmasterSx

typedef enum _oper {
	MOVE, IRSCAN, DRSCAN
} JTAG_OPER;

  /*typedef struct _SPECSMASTER
{
	HANDLE hdle;
	U64 * pSpecsmasterSx;
	U64 * pSpecsmasterStatus;
	U64 * pSpecsmasterCtrl;
	U64 * pSpecsmasterTestRegister;
	U64 * pSpecsmasterReceiverFifo;
	U64 * pSpecsmasterEmitterFifo;
} SPECSMASTER;
  */
typedef struct _SPECSMASTER
{
  HANDLE hdle;
  U32  pSpecsmasterSx;
  U32  pSpecsmasterStatus;
  U32  pSpecsmasterCtrl;
  U32  pSpecsmasterTestRegister;
  U32  pSpecsmasterReceiverFifo;
  U32  pSpecsmasterEmitterFifo;
  U32  masterID;

} SPECSMASTER;

typedef struct _SPECSSLAVE
{
	SPECSMASTER *	pSpecsmaster;
	U8				SpecsslaveAdd;
} SPECSSLAVE;


SPECS_API void SpecsmasterInit(SPECSMASTER * pSpecsmaster,
							   UINT MasterID,
							   HANDLE hdle);

  SPECS_API void SpecsmasterReset(SPECSMASTER * pSpecsmaster);

SPECS_API RETURN_CODE SpecsmasterEnd(SPECSMASTER * pSpecsmaster);

SPECS_API void SpecsmasterStatusRead(SPECSMASTER * pSpecsmaster,
									 U32 *pData);

SPECS_API U32 SpecsPlxStatusRead(SPECSMASTER * pSpecsmaster);

SPECS_API U32 SpecsPlxTestRead(SPECSMASTER * pSpecsmaster);

SPECS_API void SpecsPlxTestWrite(SPECSMASTER * pSpecsmaster,
									 U32 Data);

SPECS_API void SpecsmasterCtrlWrite(SPECSMASTER * pSpecsmaster,
									 U32 Data);
SPECS_API U32 SpecsmasterCtrlRead(SPECSMASTER * pSpecsmaster);
								

SPECS_API RETURN_CODE SpecsmasterEmitterFIFOWrite(
				SPECSMASTER * pSpecsmaster,
				U32 *pData,
				U32 Length);

SPECS_API RETURN_CODE SpecsmasterReceiverFIFORead(
				SPECSMASTER * pSpecsmaster,
				U32 *pData,
				U32 Length);

SPECS_API RETURN_CODE I2cBufferRead(
				SPECSSLAVE * pSpecsslave,
				U8 OutSelect,
				U8 I2cAdd,
				U32 *pI2cData,
				U32 Length);

SPECS_API RETURN_CODE I2cBufferReadwithSsAdd(
				SPECSSLAVE * pSpecsslave,
				U8 OutSelect,
				U8 I2cAdd,
				U8 I2cSsAdd,
				U32 *pI2cData,
				U32 Length);

SPECS_API RETURN_CODE I2cBufferWrite(
				SPECSSLAVE * pSpecsslave,
				U8 OutSelect,
				U8 I2cAdd,
				U8 *pI2cData,
				U32 Length);

SPECS_API RETURN_CODE I2cEEPROMWrite(
				SPECSSLAVE * pSpecsslave,
				U8 OutSelect,
				U8 I2cAdd,
				U8 *pI2cData,
				U32 Length);

SPECS_API RETURN_CODE JtagBufTDIWrite(
				SPECSSLAVE * pSpecsslave,
				U8 OutSelect,
				U8 JtagAdd,
				U8 *pTdiData,
				U32 Length);

SPECS_API RETURN_CODE JtagBufTMSWrite(
				SPECSSLAVE * pSpecsslave,
				U8 OutSelect,
				U8 JtagAdd,
				U8 *pTmsData,
				U32 Length);

SPECS_API RETURN_CODE JtagWriteRead(
				SPECSSLAVE * pSpecsslave,
				U8 outSelect,
				U8 *pTdiData,
				U8 *pTdoData,
				U32 nBits,
				JTAG_OPER oper, int header, int trailler);

SPECS_API RETURN_CODE ParallelWrite(
				SPECSSLAVE * pSpecsslave,
				U8 LocAdd,
				U16 Data);

SPECS_API RETURN_CODE ParallelRead(
				SPECSSLAVE * pSpecsslave,
				U8 LocAdd,
				U16 * pData);

SPECS_API RETURN_CODE ParallelDMAWrite(
				SPECSSLAVE * pSpecsslave,
				U8 LocAdd,
				U8 * pData,
				U32 Length);

SPECS_API RETURN_CODE ParallelDMARead(
				SPECSSLAVE * pSpecsslave,
				U8 LocAdd,
				U8 * pData,
				U32 Length);

SPECS_API RETURN_CODE RegisterWrite(
				SPECSSLAVE * pSpecsslave,
				U8 LocAdd,
				U16 Data);

SPECS_API RETURN_CODE RegisterRead(
				SPECSSLAVE * pSpecsslave,
				U8 LocAdd,
				U16 * pData);

SPECS_API RETURN_CODE SpecsCardIdRead(
				DEVICE_LOCATION DevLoc,
				int * pIdentificateur);

SPECS_API int I2ctoSpecs(unsigned char *BufI2c,
						 U32 *BufDestSpec,
						 U32 tailleBufI2c,
						 U32 ctrl);

SPECS_API int TditoSpecs(unsigned char *BufTdi,
						 U32 *BufDestSpec,
						 int tailleBufTdi);

SPECS_API int TmstoSpecs(unsigned char *BufTms,
						 U32 *BufDestSpec,
						 int tailleBufTms);

SPECS_API int JtagtoSpecs(U8 *bufJtag,
						  U32 *bufSpecs,
						  int nBits,
						  int nHead,
						  int nTrail);

SPECS_API int SpecstoJtag(U32 *bufSpecs,
						  U8 *bufJtag,
						  int nBits,
						  int nHead,
						  int nTrail);

SPECS_API void SpecsmasterNotifyClear(
						  HANDLE Phdle,
						  U32 Index);

#ifdef __cplusplus
}
#endif

#endif

