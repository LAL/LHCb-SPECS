#ifndef __Plx_ph__
#define __Plx_ph__

#ifdef __cplusplus
extern "C" {
#endif

double DiffTime (struct timeval *heure_depart,struct  timeval *heure_fin);
RETURN_CODE PlxPciDeviceClose (  HANDLE Handle ,U8 masterId   );
RETURN_CODE PlxPciDeviceFind ( DEVICE_LOCATION *pDevice,			      U32             *pRequestLimit    );
RETURN_CODE  PlxPciDeviceOpen (  DEVICE_LOCATION *pDevice, 				HANDLE          *pHandle,				U8               masterId);
RETURN_CODE PlxBusIopRead (			  HANDLE       Handle,			  int           masterId,			  U32          address,			  BOOLEAN      bRemap,			  VOID        *pBuffer,			  U32          ByteCount,			  ACCESS_TYPE  AccessType			  );
RETURN_CODE PlxBusIopWrite ( HANDLE       Handle,			    int          masterId,			    U32          address,			    BOOLEAN      bRemap,			    VOID        *pBuffer,			    U32          ByteCount,			    ACCESS_TYPE  AccessType			    			    );
RETURN_CODE PlxIntrEnable (			  HANDLE       Handle,			  PLX_INTR     *pInter,			  U32 Register			  );
RETURN_CODE PlxIntrDisable (			   HANDLE       Handle,			   PLX_INTR     *pInter,			   U32 statRegister			   );
RETURN_CODE PlxNotificationRegisterFor (				       HANDLE handle,				       PLX_INTR     *pPlxIntr,				       unsigned short masterId				       );
RETURN_CODE PlxNotificationWait (				HANDLE  handle,				unsigned short masterId,				U64            Timeout_ms				);
RETURN_CODE PlxNotificationCancel (				  HANDLE handle,				  U32 card				  );
RETURN_CODE PlxPciConfigRegisterRead (				     U32 bus,				     U32 slot,				     U32 registerNumber,				     U32* pData );
RETURN_CODE PlxSerialEepromReadByOffset  (  HANDLE busIndex,					   int eepromType,					   U32* buffer,					   U32 size);
U64 PlxRegisterRead  ( HANDLE    busIndex,		      U32 registerOffset,		      RETURN_CODE *error );
int  PlxPciBoardReset  (	HANDLE  handle , U8 masterId );
VOID PlxPciFIFOReset  (	HANDLE  handle , U8 masterId );
U64 PlxRegisterWrite  ( HANDLE busIndex,		       U32 registerOffset,		       U64 datas);
U32 PlxCheckSlaveIt ( HANDLE handle, U8 masterId );

#ifdef __cplusplus
}
#endif

#endif

