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
 *                CP    avril   2006    LAL Orsay     *
 *                CG    mai 2006        CERN          *
 *                      evolutions des fonctions JTAG *
 *                                                    *
 *	Changement de version SDK :                  *
 *                      version 4.40                  *
 *                CP    septembre 2006  LAL Orsay     *
 *                      remplacement des fonctions    *
 *                      d'interruption                */
// Specs.cpp : Defines the entry point for the DLL application.
//

#ifdef _WINDOWS
#define Sleep_ms                Sleep
#else
#ifdef PLX_LINUX
#include <unistd.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#define Sleep_ms(arg)       usleep((arg) * 1000)
#endif
#endif
#include "Specs.h"




#define DMA  0x8

#define I2Cmap_head  0x76543210
#define Tdimap_head  0x76543210
#define Tmsmap_head  0x76543210
#define JTAGmap_head  0x76543210
#define Parallelmap_head  0x76543210

#define TrameI2cLengthMax  0x1B

#define TDI1	0x91
#define TMS1	0xA2
#define TDI0	0x80
#define TMS0	0x80

#define BitLongueTrame 0x80

#ifdef _WINDOWS
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
		       )
{
  switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
      break;
    }
  return TRUE;
}
#endif

void SpecsmasterStartWrite(SPECSMASTER * pSpecsmaster)
{
  U32 Data;
  return ;
  Data= SpecsmasterCtrlRead( pSpecsmaster);
  Data=( Data | 0x4000 );
  
  SpecsmasterCtrlWrite(pSpecsmaster,Data);
  
  Data=( Data & 0xFFFFBFFF);
  
  SpecsmasterCtrlWrite(pSpecsmaster,Data);
  return;

}
void SpecsmasterEnableInt (SPECSMASTER * pSpecsmaster, U32 numIt)
{	static PLX_INTR		IntSources;
  IntSources.PIO_Line=numIt;
 
    
  if (numIt == IT_STATUS ) PlxIntrEnable(
				pSpecsmaster->hdle,
				&IntSources,
				pSpecsmaster->pSpecsmasterStatus
				);
  return;
}
void SpecsmasterInit(SPECSMASTER * pSpecsmaster,
                     UINT MasterID,
                     HANDLE hdle)
{
 
  pSpecsmaster->hdle = hdle;
  pSpecsmaster->pSpecsmasterSx      = RESET  ;
  pSpecsmaster->pSpecsmasterStatus  = PIO_STATUS_REG + (( MasterID%4) * MASTERS_OFFSET ) +(OFFSET_CROSSING_BRIDGE* (MasterID/4 +1));
  pSpecsmaster->pSpecsmasterCtrl    =  PIO_CTRL_REG+ ( ( MasterID%4) * MASTERS_OFFSET )+ (OFFSET_CROSSING_BRIDGE* (MasterID/4 +1));
  pSpecsmaster->pSpecsmasterTestRegister =   0x0;
  pSpecsmaster->pSpecsmasterReceiverFifo =   FIFO_RECEPTION_IN + ( (MasterID%4) * MASTERS_OFFSET )+ (OFFSET_CROSSING_BRIDGE* (MasterID/4 +1)) ;
  pSpecsmaster->pSpecsmasterEmitterFifo   =  FIFO_EMISSION_OUT + ( (MasterID%4) * MASTERS_OFFSET )+ (OFFSET_CROSSING_BRIDGE* (MasterID/4+1));
  pSpecsmaster->masterID=MasterID;
  PlxPciFIFOReset(pSpecsmaster->hdle,pSpecsmaster->masterID);

  //  SpecsmasterEnableInt (pSpecsmaster,2);	
  return;
}

void SpecsmasterReset(SPECSMASTER * pSpecsmaster)
{
 
  if ( PlxPciBoardReset(pSpecsmaster->hdle,pSpecsmaster->masterID) == ApiSuccess)
    PlxPciFIFOReset(pSpecsmaster->hdle,pSpecsmaster->masterID);
 
  return;
}

RETURN_CODE SpecsmasterEnd(SPECSMASTER * pSpecsmaster)
{
  //	RETURN_CODE rc;

  //rc = PlxPciBarUnmap(pSpecsmaster->hdle,&pSpecsmaster->pSpecsmasterReset);
  //if (rc != ApiSuccess) return ApiInvalidAddress;
  //mtq	free(pSpecsmaster->pSpecsmasterSx);
  pSpecsmaster->hdle = 0;
  pSpecsmaster->pSpecsmasterSx = 0;
  pSpecsmaster->pSpecsmasterStatus = 0;
  pSpecsmaster->pSpecsmasterCtrl = 0;
  pSpecsmaster->pSpecsmasterTestRegister = 0;
  pSpecsmaster->pSpecsmasterReceiverFifo = 0;
  pSpecsmaster->pSpecsmasterEmitterFifo = 0;
  return ApiSuccess;
}

void SpecsmasterStatusRead(SPECSMASTER * pSpecsmaster,
			   U32 *pData)
{
  *pData = SpecsPlxStatusRead(pSpecsmaster);
  return;
}

U32 SpecsPlxStatusRead(SPECSMASTER * pSpecsmaster)
{
  U32 Data;

  PlxBusIopRead(
		pSpecsmaster->hdle,
		pSpecsmaster->masterID,
		pSpecsmaster->pSpecsmasterStatus,
		FALSE,
		&Data,
		1,
		BitSize32
		);
  return Data;
}

U32 SpecsPlxTestRead(SPECSMASTER * pSpecsmaster)
{
  U32 Data;

  PlxBusIopRead(
		pSpecsmaster->hdle,
		(IOP_SPACE)pSpecsmaster->masterID,
		pSpecsmaster->pSpecsmasterTestRegister,
		FALSE,
		&Data,
		1,
		BitSize32
		);
  return Data;
}

void SpecsPlxTestWrite(SPECSMASTER * pSpecsmaster,
		       U32 Data)
{
  PlxBusIopWrite(
		 pSpecsmaster->hdle,
		 pSpecsmaster->masterID,
		 pSpecsmaster->pSpecsmasterTestRegister,
		 FALSE,
		 &Data,
		 1,
		 BitSize32
		 );
  return;
}

void SpecsmasterCtrlWrite(SPECSMASTER * pSpecsmaster,
			  U32 Data)
{
 
  //  printf (" Ctrl Write %x\n",Data);
  PlxBusIopWrite(
		 pSpecsmaster->hdle,
		 pSpecsmaster->masterID,
		 pSpecsmaster->pSpecsmasterCtrl,
		 FALSE,
		 &Data,
		 1,
		 BitSize32
		 );
  return;
}

U32 SpecsmasterCtrlRead(SPECSMASTER * pSpecsmaster)
{
  U32 Data;
 
  PlxBusIopRead(
		pSpecsmaster->hdle,
		(IOP_SPACE)pSpecsmaster->masterID,
		pSpecsmaster->pSpecsmasterCtrl,
		FALSE,
		&Data,
		1,
		BitSize32
		);

  return Data;
}

RETURN_CODE SpecsmasterEmitterFIFOWrite(
					SPECSMASTER * pSpecsmaster,
					U32 *pData,
					U32 Length)
{
  //U32	*BufferDest, ProcessPriorite;
  U32		ProcessPriorite;
  //	HANDLE hProcess;
  //	BOOLEAN bBoule;
  RETURN_CODE rc;
  U32 j=0x0,n = (SpecsPlxStatusRead(pSpecsmaster) >> 8) & 0xFF;
  if (Length > FifoLengthMax) 
    {
      printf("SpecmasterEmiterFIFIWrite :  message trop long %d\n",Length);
      return ApiInvalidSize;
    }
 
  rc=PlxBusIopWrite(
		 pSpecsmaster->hdle,
		 pSpecsmaster->masterID,
		 pSpecsmaster->pSpecsmasterEmitterFifo,
		 FALSE,
		 pData,
		 Length,
		 BitSize32
		 );
  //MTQ comme le write declenche des IRQ j'ai devalide les irq avant le write start
  if (rc == ApiInvalidSize) { 
    // printf( "In FIFO write %d\n" , Length ) ;
    return rc;
  }
  return ApiSuccess;
}

RETURN_CODE SpecsmasterReceiverFIFORead(
					SPECSMASTER * pSpecsmaster,
					U32 *pData,
					U32 Length)
{
  U32	 i=0, status, k=0;
  static BOOLEAN bPass=FALSE;
  PLX_NOTIFY_OBJECT * pPCIInterrupt=NULL;
  //static PLX_NOTIFY_OBJECT PCIInterrupt;
  static PLX_INTR		IntSources;

  RETURN_CODE		EventStatus;

  IntSources.PIO_Line=IT_FIFO;
  if (Length > FifoLengthMax) return ApiInvalidSize;

	
  EventStatus = PlxNotificationWait(
				    pSpecsmaster->hdle,
				    // MTQ pPCIInterrupt,
				    pSpecsmaster->masterID,
 
				    //4*1000
				    //2*1000
				    500
				    );

	
  if     (EventStatus !=  ApiSuccess )
    {
   
      PlxIntrDisable(
		     pSpecsmaster->hdle,
		     &IntSources,
		     pSpecsmaster->pSpecsmasterReceiverFifo
		     );
      return ApiWaitTimeout;
   
    }
 
  EventStatus= PlxBusIopRead(
		pSpecsmaster->hdle,
		pSpecsmaster->masterID,
		pSpecsmaster->pSpecsmasterReceiverFifo,
		FALSE,
		pData,
		Length,
		BitSize32
		) ;
			
  
  PlxIntrDisable(
		 pSpecsmaster->hdle,
		 &IntSources,
		 pSpecsmaster->pSpecsmasterReceiverFifo
		 );

    if ( EventStatus== ApiInvalidSize ) return ApiInvalidSize;
    if ( EventStatus== ApiInvalidData ) return ApiInvalidData; 
  return ApiSuccess;
  //Après une erreur Temps dépassé, l'application doit m.à.z. le maître concerné
  //	afin de dévalider enable read. Une fonction MasterReset doit être créée
  //	dans la bibliothèque pour permettre cette action.

}

RETURN_CODE I2cBufferRead(
			  SPECSSLAVE * pSpecsslave,
			  U8 OutSelect,
			  U8 I2cAdd,
			  U32 *pI2cData,
			  U32 Length)
{
  unsigned char BufI2c[0x100];
  //	U32	BufSpecs[0x100];
  U32 i=0,j=0;
  U32 Taille;
  U32 Control=0;
  RETURN_CODE     rc=ApiFailed;
  U32 BufSpecslocal[BufSpecsLengthMax];

  if (Length > BufI2cLengthMax-1) return ApiInvalidSize;
  while (i < BufSpecsLengthMax)
    {
      BufSpecslocal[i] = 0;
      i++;
    }

  i = 0;
  BufI2c[0] = I2cAdd << 1 | 1;
  i++;
  while (i<=Length)
    {
      BufI2c[i]=0xFF;
      i++;
    }
  Control=(OutSelect & 0xF)|pSpecsslave->SpecsslaveAdd <<18;
  Taille = I2ctoSpecs(BufI2c,BufSpecslocal,i,Control);

  /*	BufSpecs[1] = BufSpecs[1] | (i-1)<<8;
	BufSpecs[1]=BufSpecs[1] | modI2C;
	BufSpecs[1]=BufSpecs[1] | (OutSelect & 0xF);
	// adresse Specsslave a placer
	BufSpecs[1]=BufSpecs[1] | pSpecsslave->SpecsslaveAdd << 18;*/
  i = 0;
  while (i < Taille)
    {
      pI2cData[i] = BufSpecslocal[i];
      i++;
    }

  i=0;
 
   while (i<Taille)
    {
      i++;
      while ((i<Taille) & (BufSpecslocal[i] != I2Cmap_head))
	i++;
 
      rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,&BufSpecslocal[j],i-j);
      //  SpecsmasterEnableInt ( pSpecsslave-> pSpecsmaster,1);//MTQ
      SpecsmasterStartWrite ( pSpecsslave->pSpecsmaster);
      j=i;
    }
   //    SpecsmasterEnableInt ( pSpecsslave-> pSpecsmaster,1);//MTQ
  /*	rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,
	BufSpecs,Taille);*/

  return rc;
}

RETURN_CODE I2cBufferReadwithSsAdd(
				   SPECSSLAVE * pSpecsslave,
				   U8 OutSelect,
				   U8 I2cAdd,
				   U8 I2cSsAdd,
				   U32 *pI2cData,
				   U32 Length)
{
  unsigned char BufI2c[0x100];
  //	U32	BufSpecs[0x100];
  U32 i=0,j=0;
  U32 Taille;
  U32 Control=0;
  RETURN_CODE     rc=ApiFailed;
  U32 BufSpecslocal[BufSpecsLengthMax];

  if (Length > BufI2cLengthMax-1) return ApiInvalidSize;
  while (i < BufSpecsLengthMax)
    {
      BufSpecslocal[i] = 0;
      i++;
    }

  i = 0;
  BufI2c[0] = I2cAdd << 1 | 1;
  i++;
  BufI2c[1] = I2cSsAdd;
  i++;
  while (i<=Length+1)
    {
      BufI2c[i]=0xFF;
      i++;
    }

  Control=(OutSelect & 0xF)|pSpecsslave->SpecsslaveAdd <<18;
  Taille = I2ctoSpecs(BufI2c,BufSpecslocal,i,Control);

  BufSpecs[6]=BufSpecslocal[6] & 0xFFFF00FF;
  BufSpecs[6]=BufSpecslocal[6] | 0x2800;
  i = 0;
  while (i < Taille)
    {
      pI2cData[i] = BufSpecslocal[i];
      i++;
    }
  i=0;
  while (i<Taille)
    {
      i++;
      while ((i<Taille) & (BufSpecslocal[i] != I2Cmap_head))
	i++;
      rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,&BufSpecslocal[j],i-j);
      //  SpecsmasterEnableInt ( pSpecsslave-> pSpecsmaster,1);//MTQ
      SpecsmasterStartWrite ( pSpecsslave->pSpecsmaster);
      j=i;
    }
  /*	rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,
	BufSpecs,Taille);*/

  return rc;
}

RETURN_CODE I2cBufferWrite(
			   SPECSSLAVE * pSpecsslave,
			   U8 OutSelect,
			   U8 I2cAdd,
			   U8 *pI2cData,
			   U32 Length)
{
  unsigned char BufI2c[0x100];
  //	U32	BufSpecs[0x100];
  U32 i=0,j=0;
  U32 Taille;
  U32 Control=0;
  RETURN_CODE     rc=ApiFailed;
  U32 BufSpecslocal[BufSpecsLengthMax];

  if (Length > BufI2cLengthMax-1) return ApiInvalidSize;
  while (i < BufSpecsLengthMax)
    {
      BufSpecslocal[i] = 0;
      i++;
    }

  i = 0;
  BufI2c[0] = I2cAdd << 1 & 0xFE;
  i++;
  while (i<=Length)
    {
      BufI2c[i] = pI2cData[i-1];
      i++;
    }

  Control=(OutSelect & 0xF)|pSpecsslave->SpecsslaveAdd <<18;
  Taille = I2ctoSpecs(BufI2c,BufSpecslocal,i,Control);
  i=0;
  
  /*// adresse Specsslave a placer
    BufSpecs[1]=BufSpecs[1] | pSpecsslave->SpecsslaveAdd << 18;
    //	BufSpecs[1]=BufSpecs[1] | (i-1) << 8;
    BufSpecs[1]=BufSpecs[1] | modI2C;
    BufSpecs[1]=BufSpecs[1] | (OutSelect & 0xF);*/

  while (i<Taille)
    {
      i++;
      while ((i<Taille) & (BufSpecslocal[i] != I2Cmap_head))
        i++;
      rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,&BufSpecslocal[j],i-j);
      //      if ( rc == ApiInvalidSize ) 
      //	printf( "ICI %d %d\n" , i , Taille ) ;
      SpecsmasterStartWrite ( pSpecsslave->pSpecsmaster);
      j=i;
    }
  /*	rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,
	BufSpecs,Taille);*/
  return rc;
}

RETURN_CODE I2cEEPROMWrite(
			   SPECSSLAVE * pSpecsslave,
			   U8 OutSelect,
			   U8 I2cAdd,
			   U8 *pI2cData,
			   U32 Length)
{
  unsigned char BufI2c[0x100];
  U32 i=0,j=0;
  U32 Taille;
  U32 Control=0;
  RETURN_CODE     rc=ApiFailed;
  U32 BufSpecslocal[BufSpecsLengthMax];
  U32  Cntoct;
  U32 Cntent, Cntrest, Msk;

  //if (Length > BufI2cLengthMax-1) return ApiInvalidSize;
  if (Length > 27) return ApiInvalidSize;
  while (i < BufSpecsLengthMax)
    {
      BufSpecslocal[i] = 0;
      i++;
    }

  i = 0;
  BufI2c[0] = I2cAdd << 1 & 0xFE;
  i++;
  while (i<=Length)
    {
      BufI2c[i] = pI2cData[i-1];
      i++;
    }

  Control=(OutSelect & 0xF)|pSpecsslave->SpecsslaveAdd <<18;
  Taille = I2ctoSpecs(BufI2c,BufSpecslocal,i,Control);
  Cntoct = (BufSpecslocal[1] & 0xFF000000) >> 24;
  Cntent = Cntoct/4;
  Cntrest = Cntoct%4;
  // prépare le vrai compte d'octets specs
  Cntoct--;
  BufSpecslocal[1] &= 0x00FFFFFF;
  BufSpecslocal[1] |= Cntoct << 24;
  if (Cntrest)
    {
      Msk = 0xFF << (4-Cntrest)*8;
      Msk = ~Msk;
      BufSpecslocal[Cntent+2] &= Msk;
      if (Cntrest == 1)
	Taille --;
    }
  else
    {
      Msk = 0xFF;
      Msk = ~Msk;
      BufSpecslocal[Cntent+1] &= Msk;
    }

  i=0;
  while (i<Taille)
    {
      i++;
      while ((i<Taille) & (BufSpecslocal[i] != I2Cmap_head))
	i++;
      rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,&BufSpecslocal[j],i-j);
      SpecsmasterStartWrite ( pSpecsslave->pSpecsmaster);
      j=i;
    }

  return rc;
}

RETURN_CODE JtagBufTDIWrite(
			    SPECSSLAVE * pSpecsslave,
			    U8 OutSelect,
			    U8 JtagAdd,
			    U8 *pTdiData,
			    U32 Length)
{
  unsigned char BufTdi[50];
  //	U32	BufSpecs[0x100];
  U32 i=0, Taille;
  RETURN_CODE     rc;
  U32 BufSpecslocal[BufSpecsLengthMax];

  //	BufTdi[0] = JtagAdd << 1 & 0xFE;
  //	i++;
  if (Length > BufJtagMax) return ApiInvalidSize;
  while (i < BufSpecsLengthMax)
    {
      BufSpecslocal[i] = 0;
      i++;
    }

  i = 0;
  while (i<=Length)
    {
      BufTdi[i] = pTdiData[i];
      i++;
    }
  Taille = TditoSpecs(BufTdi,BufSpecslocal,i-1);

  BufSpecslocal[1]=BufSpecslocal[1] | pSpecsslave->SpecsslaveAdd << 18;
  BufSpecslocal[1]=BufSpecslocal[1] | (i-1) << 8;
  BufSpecslocal[1]=BufSpecslocal[1] | modJTAG;
  BufSpecslocal[1]=BufSpecslocal[1] | (OutSelect & 0xF);
  rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,
				   BufSpecslocal,Taille);

  return rc;
}

RETURN_CODE JtagBufTMSWrite(
			    SPECSSLAVE * pSpecsslave,
			    U8 OutSelect,
			    U8 JtagAdd,
			    U8 *pTmsData,
			    U32 Length)
{
  unsigned char BufTms[50];
  //	U32	BufSpecs[0x100];
  U32 i=0, Taille;
  RETURN_CODE     rc;
  U32 BufSpecslocal[BufSpecsLengthMax];

  //	BufTms[0] = JtagAdd << 1 & 0xFE;
  //	i++;
  if (Length > BufJtagMax) return ApiInvalidSize;
  while (i < BufSpecsLengthMax)
    {
      BufSpecslocal[i] = 0;
      i++;
    }

  i = 0;
  while (i<=Length)
    {
      BufTms[i] = pTmsData[i];
      i++;
    }
  Taille = TmstoSpecs(BufTms,BufSpecslocal,i-1);

  BufSpecslocal[1]=BufSpecslocal[1] | pSpecsslave->SpecsslaveAdd << 18;
  BufSpecslocal[1]=BufSpecslocal[1] | (i-1) << 8;
  BufSpecslocal[1]=BufSpecslocal[1] | modJTAG;
  BufSpecslocal[1]=BufSpecslocal[1] | (OutSelect & 0xF);
  rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,
				   BufSpecslocal,Taille);
  SpecsmasterStartWrite ( pSpecsslave->pSpecsmaster);
 
  return rc;
}

RETURN_CODE ParallelWrite(
			  SPECSSLAVE * pSpecsslave,
			  U8 LocAdd,
			  U16 Data)
{
  U32	BufSpecslocal[0x3] = {0,0,0};
  U32 i=0;
  RETURN_CODE     rc;

  /*	while (i < FifoLengthMax)
	{
	BufSpecs[i] = 0;
	i++;
	}*/

  BufSpecslocal[0] = Parallelmap_head;
  BufSpecslocal[1] = 0x2000000 | modParallelBus;
  BufSpecslocal[1] = BufSpecslocal[1] | pSpecsslave->SpecsslaveAdd << 18;
  BufSpecslocal[1] = BufSpecslocal[1] | LocAdd << 8;
  //	BufSpecs[1] = BufSpecs[1]; //CS,OS,R/W
  BufSpecslocal[2] = Data << 16;
  rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,
				   BufSpecslocal,3);
  SpecsmasterStartWrite ( pSpecsslave->pSpecsmaster);
  return rc;
}

RETURN_CODE ParallelRead(
			 SPECSSLAVE * pSpecsslave,
			 U8 LocAdd,
			 U16 * pData)
{
  U32	BufSpecslocal[0x3] = {0,0,0};
  U32 i=0;
  RETURN_CODE     rc;

  /*	while (i < FifoLengthMax)
	{
	BufSpecs[i] = 0;
	i++;
	}*/

  BufSpecslocal[0] = Parallelmap_head;
  BufSpecslocal[1] = 0x2000000 | modParallelBus;
  BufSpecslocal[1] = BufSpecslocal[1] | pSpecsslave->SpecsslaveAdd << 18;
  BufSpecslocal[1] = BufSpecslocal[1] | LocAdd << 8;
  BufSpecslocal[1] = BufSpecslocal[1] | 1; //CS,OS,R/W
  BufSpecslocal[2] = 0;
  rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,
				   BufSpecslocal,3);
  SpecsmasterEnableInt ( pSpecsslave-> pSpecsmaster,1);//MTQ
  SpecsmasterStartWrite ( pSpecsslave->pSpecsmaster);
  return rc;
}

RETURN_CODE ParallelDMAWrite(
			     SPECSSLAVE * pSpecsslave,
			     U8 LocAdd,
			     U8 * pData,
			     U32 Length)
{
  //	U32 BufSpecs[0x100];
  U32 i=2, k=4, j=0, fh2=0, Taille;
  RETURN_CODE     rc;
  U32 BufSpecslocal[BufSpecsLengthMax];


  while (fh2 < BufSpecsLengthMax)
    {
      BufSpecslocal[fh2] = 0;
      fh2++ ;
    }

  BufSpecslocal[0] = Parallelmap_head;
  BufSpecslocal[1] = Length << 24 | modParallelBus;
  BufSpecslocal[1] = BufSpecslocal[1] | pSpecsslave->SpecsslaveAdd << 18;
  BufSpecslocal[1] = BufSpecslocal[1] | LocAdd << 8;
  //	BufSpecs[1] = BufSpecs[1]; //CS,OS,R/W
  BufSpecslocal[1] = BufSpecslocal[1] | DMA;

  //	i=2;
  //	k=4;
  for (j=0; j < Length; j++)
    {
      k--;
      BufSpecslocal[i] = BufSpecslocal[i] | pData[j] << 8*k;
      if (k == 0)
	{
	  k=4;
	  i++;
	}
    }
  if (k != 4) i++;
  Taille = i;

  rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,
				   BufSpecslocal,Taille);
  SpecsmasterStartWrite ( pSpecsslave->pSpecsmaster);

  return rc;
}

RETURN_CODE ParallelDMARead(
			    SPECSSLAVE * pSpecsslave,
			    U8 LocAdd,
			    U8 * pData,
			    U32 Length)
{
  U32	BufSpecslocal[0x3] = {0,0,0};
  U32 i=0;
  RETURN_CODE     rc;


  /*	while (fh2 < FifoLengthMax)
	{
	BufSpecslocal[fh2] = 0;
	fh2++ ;
	}*/

  BufSpecslocal[0] = Parallelmap_head;
  BufSpecslocal[1] = 2 << 24 | modParallelBus;
  BufSpecslocal[1] = BufSpecslocal[1] | pSpecsslave->SpecsslaveAdd << 18;
  BufSpecslocal[1] = BufSpecslocal[1] | LocAdd << 8;
  BufSpecslocal[1] = BufSpecslocal[1] | 1; //CS,OS,R/W
  BufSpecslocal[1] = BufSpecslocal[1] | DMA;
  BufSpecslocal[2] = Length << 24;

  //	i=2;
  //	k=4;
  /*	for (j=0; j < Length; j++)
	{
	k--;
	BufSpecs[i] = BufSpecs[i] | pData[j] << 8*k;
	if (k == 0)
	{
	k=4;
	i++;
	}
	}
	if (k != 4) i++;*/

  rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,
				   BufSpecslocal,3);
  SpecsmasterEnableInt ( pSpecsslave-> pSpecsmaster,1);//MTQ
  SpecsmasterStartWrite ( pSpecsslave->pSpecsmaster);

  return rc;
}

RETURN_CODE RegisterWrite(
			  SPECSSLAVE * pSpecsslave,
			  U8 LocAdd,
			  U16 Data)
{
  U32	BufSpecslocal[0x3] = {0,0,0};
  U32 i=0;
  RETURN_CODE     rc;

  /*	while (i < FifoLengthMax)
	{
	BufSpecs[i] = 0;
	i++;
	}*/
  BufSpecslocal[0] = Parallelmap_head;
  BufSpecslocal[1] = 0x2000000 | modRegistre;
  BufSpecslocal[1] = BufSpecslocal[1] | pSpecsslave->SpecsslaveAdd << 18;
  BufSpecslocal[1] = BufSpecslocal[1] | LocAdd << 8;
  //	BufSpecs[1] = BufSpecs[1]; //CS,OS,R/W
  BufSpecslocal[2] = Data << 16;
  rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,
				   BufSpecslocal,3);
  SpecsmasterStartWrite ( pSpecsslave->pSpecsmaster);

  return rc;
}

RETURN_CODE RegisterRead(
			 SPECSSLAVE * pSpecsslave,
			 U8 LocAdd,
			 U16 * pData)
{
  U32	BufSpecslocal[0x3] = {0,0,0};
  U32 i=0;
  RETURN_CODE     rc;

  /*	while (i < FifoLengthMax)
	{
	BufSpecs[i] = 0;
	i++;
	}*/

  BufSpecslocal[0] = Parallelmap_head;
  BufSpecslocal[1] = 0x2000000 | modRegistre;
  BufSpecslocal[1] = BufSpecslocal[1] | pSpecsslave->SpecsslaveAdd << 18;
  BufSpecslocal[1] = BufSpecslocal[1] | LocAdd << 8;
  BufSpecslocal[1] = BufSpecslocal[1] | 1; //CS,OS,R/W
  BufSpecslocal[2] = 0;
  rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,
				   BufSpecslocal,3);
  SpecsmasterEnableInt ( pSpecsslave-> pSpecsmaster,1);//MTQ
  SpecsmasterStartWrite ( pSpecsslave->pSpecsmaster);
 
  return rc;
}

RETURN_CODE SpecsCardIdRead(DEVICE_LOCATION DevLoc,
			    int * pIdentificateur)
{
  U32 rc;
#define off 0x2C

  *pIdentificateur = PlxPciConfigRegisterRead(
					      DevLoc.BusNumber,
					      DevLoc.SlotNumber,
					      off,
					      &rc
					      );

  return (RETURN_CODE)rc;
}

int I2ctoSpecs(unsigned char *BufI2c,
	       U32 *BufDestSpec,
	       U32 tailleBufI2c,
	       U32 ctrl)
{
  int const I2Cmap_0 = 0x28;
  int const I2Cmap_1 = 0x7D;
  const int I2Cmap_start = 0x0B;
  const int I2Cmap_stop = 0xF8;
  const int I2Cmap_ack_ecr = 0x7D;
  const int I2Cmap_ack_lec = 0x28;

  //	const int I2Cmap_head = 0x76543210;
  int  j, k, t, z, tailleBufDest, tailleBufDestSpec,I2Cmap_ack=I2Cmap_ack_ecr;
  unsigned int i;
  unsigned char BufI2cTemp, BufDest[BufSpecsLengthMax*9];
  unsigned long BufDestTemp;
  BOOLEAN nTramentier=FALSE;
  int T;

  //test de tailleBufI2c < BufI2cLengthMax avec refus éventuel
  if (tailleBufI2c > BufI2cLengthMax) return BufSpecsLengthMax+1;

  if (BufI2c[0] & 1)
    I2Cmap_ack = I2Cmap_ack_lec;	// lecture ou écriture

  for (i=0; i <= tailleBufI2c-1; i++)	// ex : tailleBufI2c=4
    {
      BufI2cTemp = BufI2c[i];
      for (j=0; j<=7; j++)
	{
	  if (BufI2cTemp & 0x80)
	    BufDest[9*i + j] = I2Cmap_1;
	  else								// ligne par ligne de BufI2c :
	    BufDest[9*i + j] = I2Cmap_0;	// conversion de chaque bit en octet
	  BufI2cTemp = BufI2cTemp << 1;		// specs dans BufDest
	}
      if (i == 0 || i == tailleBufI2c-1)
	BufDest[9*i+j] = I2Cmap_ack_ecr;	// dernier Ack toujours à 1
      else
	BufDest[9*i + j] = I2Cmap_ack;

    }
  tailleBufDest = 9*i;						// ex : 36


  // modifications pour permettre les trames longues I2C
  // C.P. 7 avril 2005

  i = 2;
  z = 9;
  t = 0;
  k = 3;
  BufDestSpec[i] = I2Cmap_start << 8*k;
  /*	for (j=0; j<=8; j++)
	{
	k--;
	BufDestTemp = BufDest[j] << 8*k;
	BufDestSpec[i] |= BufDestTemp;

	if (k==0)
	{
	k = 4;
	i++;
	}
	}*/

  //	z=j;
  for (j=0; j<=tailleBufDest -1; j++)
    {
      k--;
      BufDestTemp = BufDest[j] << 8*k;
      BufDestSpec[i] |= BufDestTemp;

      if (k==0)
	{
	  k = 4;
	  i++;
	}
      if (j == tailleBufDest - 1)
	{
	  k--;								// si fin de BufDest, donc fin de BufI2c
	  BufDestTemp = I2Cmap_stop << 8*k;	// on place F8 (stop)
	  BufDestSpec[i] |= BufDestTemp;
	}
      if (z==9)
	T = TrameI2cLengthMax;				// 1ere trame partielle
      else
	T = TrameI2cLengthMax+1;			// les suivantes
      if (j-z == T*9-1)
	{										// trame partielle pleine
	  BufDestSpec[t] = I2Cmap_head;		// on complète ses 2 en-têtes
	  if (j == T*9+8)						// c.a.d. z=9 (1ere trame partielle)
	    if (j==tailleBufDest-1)
	      BufDestSpec[t+1] = (j+3) << 8*3;	// fin d'envoi donc + 0B + F8
	    else
	      BufDestSpec[t+1] = (j+2) << 8*3;
	  else										// trames partielles suivantes
	    if (j==tailleBufDest-1)
	      BufDestSpec[t+1] = (j-z+2) << 8*3;	// j-z+1 +1 (F8) mais pas 0B
	    else
	      BufDestSpec[t+1] = (j-z+1) << 8*3;	// j-z+1 ni 0B ni F8
	  BufDestSpec[t+1] |= ctrl;
	  BufDestSpec[t+1] |= modI2C;
	  BufDestSpec[t+1] |= (T) << 8;
	  if (j != tailleBufDest-1)
	    BufDestSpec[t+1] |= BitLongueTrame << 8;	// fin de trame partielle, mais
	  // ça n'est pas fini
	  if (j==tailleBufDest-1)
	    {
	      /*				BufDestSpec[t+1] |= ((TrameI2cLengthMax-1) & 0xFF) << 8;
						BufDestTemp = ~(0xFF << 8*k);
						BufDestSpec[i++] &= BufDestTemp;
						//				i++;
						BufDestSpec[i++] = I2Cmap_head;
						//				i++;
						BufDestSpec[i] = 0x01000100;
						BufDestSpec[i] |= ctrl;
						BufDestSpec[i++] |= modI2C;
						//				i++;
						BufDestSpec[i] = 0xF8000000;*/
	      nTramentier = TRUE;					// fin d'envoi avec trame partielle
	      break;								// complète .... on arrêtera là
	    }
	  if (k !=4 )
	    i++;
	  k = 4;
	  t = i;
	  i++;
	  i++;
	  z = j+1;
	}
    }



  //	k--;
  //	BufDestTemp = I2Cmap_stop << 8*k;
  //	BufDestSpec[i] = BufDestSpec[i] | BufDestTemp;
  //	if (k != 4) inutile ?
  i++;
  tailleBufDestSpec = i;

  //   Ancienne version
  //	BufDestSpec[0] = I2Cmap_head;
  //	BufDestSpec[1] = (tailleBufDest+2) << 8*3;
  if (!nTramentier)				// la dernière trame n'est pas complète
    {								// il faut remplir les en-têtes
      BufDestSpec[t] = I2Cmap_head;
      //			BufDestSpec[t+1] = (j-z+1) << 8*3;
      if (j<=TrameI2cLengthMax*9+8)               // T ????
	BufDestSpec[t+1] = (j+2) << 8*3;
      else
	BufDestSpec[t+1] = (j-z+1) << 8*3;
      BufDestSpec[t+1] |= ctrl;
      BufDestSpec[t+1] |= modI2C;
      //			BufDestSpec[t+1] |= (j-z)/9 << 8;
      BufDestSpec[t+1] |= ((j-z)/9) << 8;
    }

  return tailleBufDestSpec;
}

int TditoSpecs(unsigned char *BufTdi,
	       U32 *BufDestSpec,
	       int tailleBufTdi)
{
  int const Tdimap_0 = 0x80;
  int const Tdimap_1 = 0x91;
  //	const int Tdimap_head = 0x76543210;
  int i, k, j, tailleBufDest, tailleBufDestSpec;
  unsigned char BufTdiTemp, BufDest[BufJtagMax*8+1];
  unsigned long BufDestTemp;
  //	if (BufI2c[0] & 1)
  //		I2Cmap_ack = I2Cmap_ack_lec;

  if (tailleBufTdi > BufJtagMax) return FifoLengthMax+1;

  for (i=0; i <= BufJtagMax*2; i++)
    BufDestSpec[i] = 0;

  for (i=0; i <= tailleBufTdi-1; i++)
    {
      BufTdiTemp = BufTdi[i];
      for (j=0; j<=7; j++)
	{
	  if (BufTdiTemp & 0x80)
	    BufDest[8*i + j] = Tdimap_1;
	  else
	    BufDest[8*i + j] = Tdimap_0;
	  BufTdiTemp = BufTdiTemp << 1;
	}
      //		if (i == 0 || i == tailleBufTdi-1)
      //			BufDest[9*i+j] = I2Cmap_ack_ecr;
      //		else
      //			BufDest[9*i + j] = I2Cmap_ack;

    }
  tailleBufDest = 8*i + 1;
  BufDest[tailleBufDest-1] = 0;
		


  i = 2;
  k = 4;
  //	BufDestSpec[i] = I2Cmap_start << 8*k;
  for (j=0; j<=tailleBufDest -1; j++)
    {
      k--;
      BufDestTemp = BufDest[j] << 8*k;
      BufDestSpec[i] = BufDestSpec[i] | BufDestTemp;
      if (k==0)
	{
	  k = 4;
	  i++;
	}
    }
  //	k--;
  //	BufDestTemp = I2Cmap_stop << 8*k;
  //	BufDestSpec[i] = BufDestSpec[i] | BufDestTemp;
  if (k != 4)
    i++;
  tailleBufDestSpec = i;
  BufDestSpec[0] = Tdimap_head;
  BufDestSpec[1] = (tailleBufDest) << 8*3;


  return tailleBufDestSpec;
}

int TmstoSpecs(unsigned char *BufTms,
	       U32*BufDestSpec,
	       int tailleBufTms)
{
  int const Tmsmap_0 = 0x80;
  int const Tmsmap_1 = 0xA2;


  //	const int Tmsmap_head = 0x76543210;
  int i, k, j, tailleBufDest, tailleBufDestSpec;
  unsigned char BufTmsTemp, BufDest[BufJtagMax*8+1];
  unsigned long BufDestTemp;

  if (tailleBufTms > BufJtagMax) return FifoLengthMax+1;

  for (i=0; i <= BufJtagMax*2; i++)
    BufDestSpec[i] = 0;

  for (i=0; i <= tailleBufTms-1; i++)
    {
      BufTmsTemp = BufTms[i];
      for (j=0; j<=7; j++)
	{
	  if (BufTmsTemp & 0x80)
	    BufDest[8*i + j] = Tmsmap_1;
	  else
	    BufDest[8*i + j] = Tmsmap_0;
	  BufTmsTemp = BufTmsTemp << 1;
	}
    }
  tailleBufDest = 8*i + 1;
  BufDest[tailleBufDest-1] = 0;


  i = 2;
  k = 4;
  //	BufDestSpec[i] = I2Cmap_start << 8*k;
  for (j=0; j<=tailleBufDest -1; j++)
    {
      k--;
      BufDestTemp = BufDest[j] << 8*k;
      BufDestSpec[i] = BufDestSpec[i] | BufDestTemp;
      if (k==0)
	{
	  k = 4;
	  i++;
	}
    }
  if (k != 4)
    i++;
  tailleBufDestSpec = i;
  BufDestSpec[0] = Tmsmap_head;
  BufDestSpec[1] = (tailleBufDest) << 8*3;


  return tailleBufDestSpec;
}

RETURN_CODE JtagWriteRead(
                          SPECSSLAVE * pSpecsslave,
                          U8 outSelect,
                          U8 *pDataIn,
                          U8 *pDataOut,
                          U32 nBits,
                          JTAG_OPER oper, int header, int trailler)
{
  RETURN_CODE rc;
  int sizeBufSpecs;
  int nhead, ntrail, extra;

  U32 bufSpecsOut[0x40];

  nhead = 0;
  extra= 0;

  if(header)
    {
      if(oper == IRSCAN)
        nhead = 4;
      else if(oper == DRSCAN)
        nhead = 3;
    }
  else
    {
      if(oper == MOVE)
        nhead = -1;
    }
  ntrail = 0;
  if(trailler)
  {
    ntrail = 2;
    extra = 1;
    }
  
  sizeBufSpecs = JtagtoSpecs(pDataIn, BufSpecs, nBits, nhead, ntrail);
  
  if(nhead == -1)
    nhead = 0;

  if (sizeBufSpecs == 0) return ApiInvalidSize;

  BufSpecs[1] |= pSpecsslave->SpecsslaveAdd << 18;
  BufSpecs[1] &= 0x00FFFFFF;
  BufSpecs[1] |= (nBits+1+nhead+ntrail+extra) << 24;
  // BufSpecs[1] |= (nBits) << 24 ;
  BufSpecs[1] |= JTAG_calcNBytes(nBits + nhead + ntrail) << 8;
  BufSpecs[1] |= modJTAG;
  BufSpecs[1] |= (outSelect & 0xF);
  
  int expectedSize = 1+((nBits+nhead+ntrail+extra)/4) ;

  if ( expectedSize != (sizeBufSpecs - 2 ) ) 
    sizeBufSpecs = sizeBufSpecs - 1 ;

  rc = SpecsmasterEmitterFIFOWrite(pSpecsslave->pSpecsmaster,
                                   BufSpecs, sizeBufSpecs);
  SpecsmasterEnableInt ( pSpecsslave-> pSpecsmaster,1);//MTQ
  SpecsmasterStartWrite ( pSpecsslave->pSpecsmaster);
  if ( rc != ApiSuccess )
    return rc ;

  if(oper == MOVE)
    {
      return ApiSuccess;
    }

  sizeBufSpecs = 3 + ( nBits - 1 ) / 32 ;

  rc = SpecsmasterReceiverFIFORead(pSpecsslave->pSpecsmaster,
                                   bufSpecsOut, sizeBufSpecs);
    
  if ( rc != ApiSuccess )
    return rc ;

  SpecstoJtag(bufSpecsOut, pDataOut, nBits, nhead, ntrail);

  return ApiSuccess;
}

int JtagtoSpecs(U8 *bufJtag, U32 *bufSpecs, int nBits, int nHead, int nTrail)
{
  U8 *bufPtr, tmp, bit, tmp_bit;
  int i, j, n, index, nBytes, nBitsLeft, sizeBufSpecs;

  if(nHead == -1)
    {
      bit = TMS1;
      nHead = 0;
    }
  else
    bit = TDI1;
  nBytes = JTAG_calcNBytes(nBits+1);
 
  if (nBytes > BufJtagMax) return 0;

  bufSpecs[0] = JTAGmap_head;
  bufSpecs[1] = 0;

  bufPtr = (U8 *)&bufSpecs[2];
  index = 0;
  if(nHead)
    {
      tmp_bit = TMS1 & 0x33;
      bufPtr[JTAG_byteIndex(index)] = tmp_bit;
      index++;
      if(nHead == 4)
      {
        bufPtr[JTAG_byteIndex(index)] = TMS1;
        index++;
      }
      bufPtr[JTAG_byteIndex(index)] = TMS1;
      index++;
      bufPtr[JTAG_byteIndex(index)] = TMS0;
      index++;
      bufPtr[JTAG_byteIndex(index)] = TMS0;
      index++;
    }
  else
    {
      tmp = bufJtag[nBytes-1];
      tmp_bit = (tmp & 0x1) ? bit : 0x00;
      tmp_bit &= 0x33;
      bufPtr[JTAG_byteIndex(index)] = tmp_bit;
      index++;
    }
  nBytes = JTAG_calcNBytes(nBits);
  
  for(i = nBytes; i > 0; i--)
  {
    tmp = bufJtag[i-1];
    j = 0;
    n = 8;
    if( i == nBytes)
    {
      nBitsLeft = JTAG_bitsLeft(nBits);
      j = 8 - nBitsLeft;
      n = nBitsLeft + j;
    }
    for(; j < n; j++)
    {
      bufPtr[JTAG_byteIndex(index)] =
        tmp & (0x1 << j) ? bit : 0x80;
      index++;
    }
  }
  if(nTrail)
  {
    bufPtr[JTAG_byteIndex(index - 1)] |= 0x22;
    bufPtr[JTAG_byteIndex(index)] = TMS1;
    index++;
    bufPtr[JTAG_byteIndex(index)] = TMS0;
    index++;
  }
  bufPtr[JTAG_byteIndex(index)] = 0;
  index++;
  
  sizeBufSpecs = ((index-1)/4)+3;
  
  return(sizeBufSpecs);
}

int SpecstoJtag(U32 *bufSpecs, U8 *bufJtag, int nBits, int nHead, int nTrail)
{
  U8 *bufPtr, tmp;
  int i, j, n, index, done, intrail, nBytes, nBitsLeft;
		
  bufPtr = (U8 *)bufSpecs;

  for(i = 0; i < JTAG_calcNBytes(nBits); i++)
    {
      bufJtag[i] = 0;
    }
  index = 0;
  done = 0;
  intrail = nTrail;
  nBytes = JTAG_calcNBytes(nBits+nHead+nTrail);
  for(i = nBytes; i > 0; i--)
    {
      tmp = bufPtr[JTAG_byteIndex(i + 7 - 1)];
      n = 8;
      j = 0;
      if( i == 1)
        n = 8 - nHead;
      if( i == nBytes )
      {
        nBitsLeft = JTAG_bitsLeft(nBits+nHead+nTrail);
        j = 8 - nBitsLeft;
        n = nBitsLeft +j;
        tmp <<= 8 - nBitsLeft;
      }
      for(; j < n; j++)
      {
        if(intrail)
        {
          intrail--;
          continue;
        }
        bufJtag[index] |=
          (tmp & (0x1 << j)) ? (0x1 << (7-done)) : 0; 
        done++;
        if(done == 8)
        {
          done = 0;
          index++;
        }
      }
    }
  return 1;
}

void SpecsmasterNotifyClear(HANDLE Phdle,
                            U32 Index)
{
  RETURN_CODE     rc;

  //MTQ	rc = PlxNotificationCancel(Phdle,&InterTab[Index]);
  rc = PlxNotificationCancel(Phdle,Index);

  memset(&InterTab[Index],0,sizeof(PLX_NOTIFY_OBJECT));
  return;
}
