#ifndef __PLX_H
#define __PLX_H

/*******************************************************************************
 * Copyright (c) 2005 PLX Technology, Inc.
 * 
 * PLX Technology Inc. licenses this software under specific terms and
 * conditions.  Use of any of the software or derviatives thereof in any
 * product without a PLX Technology chip is strictly prohibited. 
 * 
 * PLX Technology, Inc. provides this software AS IS, WITHOUT ANY WARRANTY,
 * EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  PLX makes no guarantee
 * or representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 * 
 * IN NO EVENT SHALL PLX BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL PLX'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO PLX FOR THE PRODUCT LICENSED HEREUNDER.
 * 
 ******************************************************************************/

/******************************************************************************
 *
 * File Name:
 *
 *      Plx.h
 *
 * Description:
 *
 *      This file contains definitions that are common to all PCI SDK code.
 *
 * Revision:
 *
 *      12-01-05 : PCI SDK v4.40
 *
 ******************************************************************************/
#define PLX_STATUS RETURN_CODE

#ifdef __cplusplus
extern "C" {
#endif
// Access Size Type
#include "AltSpecsV2.h"

#if !defined(TRUE)
    #define TRUE              1
#endif

#if !defined(FALSE)
    #define FALSE             0
#endif

  typedef int                     HANDLE;
  typedef unsigned long long      U64;
  typedef unsigned int           U32;
  typedef unsigned char           U8;
  typedef unsigned short          U16;
  typedef unsigned int UINT;
  typedef int                    S32;

// Local Space Types
typedef enum _IOP_SPACE
{
    IopSpace0,
    IopSpace1,
    IopSpace2,
    IopSpace3
} IOP_SPACE;
#if !defined(BOOLEAN)
typedef U8            BOOLEAN;
#endif

typedef void VOID;
typedef struct _PLX_INTR
{
  U32 PIO_Line;
} PLX_INTR;
// PLX Notification Object
typedef struct _PLX_NOTIFY_OBJECT
{
    U32 IsValidTag;                  // Magic number to determine validity
    U64 pWaitObject;                 // -- INTERNAL -- Wait object used by the driver
    U64 hEvent;                      // User event handle (HANDLE can be 32 or 64 bit)
} PLX_NOTIFY_OBJECT;
// Bus Index Types
typedef enum _BUS_INDEX
{
    PrimaryPciBus
} BUS_INDEX;
// EEPROM Type Definitions
typedef enum _EEPROM_TYPE
{
    Eeprom93CS46,
    Eeprom93CS56,
    Eeprom93CS66,
    EepromX24012,
    EepromX24022,
    EepromX24042,
    EepromX24162,
    EEPROM_UNSUPPORTED
} EEPROM_TYPE;

#define API_RETURN_CODE_STARTS              0x200   /* Starting return code */
#include "Plx.h"

/* API Return Code Values */
typedef enum _RETURN_CODE
{
    ApiSuccess = API_RETURN_CODE_STARTS,
    ApiFailed,
    ApiAccessDenied,
    ApiDmaChannelUnavailable,
    ApiDmaChannelInvalid,
    ApiDmaChannelTypeError,
    ApiDmaInProgress,
    ApiDmaDone,
    ApiDmaPaused,
    ApiDmaNotPaused,
    ApiDmaCommandInvalid,
    ApiDmaManReady,
    ApiDmaManNotReady,
    ApiDmaInvalidChannelPriority,
    ApiDmaManCorrupted,
    ApiDmaInvalidElementIndex,
    ApiDmaNoMoreElements,
    ApiDmaSglInvalid,
    ApiDmaSglQueueFull,
    ApiNullParam,
    ApiInvalidBusIndex,
    ApiUnsupportedFunction,
    ApiInvalidPciSpace,
    ApiInvalidIopSpace,
    ApiInvalidSize,
    ApiInvalidAddress,
    ApiInvalidAccessType,
    ApiInvalidIndex,
    ApiMuNotReady,
    ApiMuFifoEmpty,
    ApiMuFifoFull,
    ApiInvalidRegister,
    ApiDoorbellClearFailed,
    ApiInvalidUserPin,
    ApiInvalidUserState,
    ApiEepromNotPresent,
    ApiEepromTypeNotSupported,
    ApiEepromBlank,
    ApiConfigAccessFailed,
    ApiInvalidDeviceInfo,
    ApiNoActiveDriver,
    ApiInsufficientResources,
    ApiObjectAlreadyAllocated,
    ApiAlreadyInitialized,
    ApiNotInitialized,
    ApiBadConfigRegEndianMode,
    ApiInvalidPowerState,
    ApiPowerDown,
    ApiFlybyNotSupported,
    ApiNotSupportThisChannel,
    ApiNoAction,
    ApiHSNotSupported,
    ApiVPDNotSupported,
    ApiVpdNotEnabled,
    ApiNoMoreCap,
    ApiInvalidOffset,
    ApiBadPinDirection,
    ApiPciTimeout,
    ApiDmaChannelClosed,
    ApiDmaChannelError,
    ApiInvalidHandle,
    ApiBufferNotReady,
    ApiInvalidData,
    ApiDoNothing,
    ApiDmaSglBuildFailed,
    ApiPMNotSupported,
    ApiInvalidDriverVersion,
    ApiWaitTimeout,
    ApiWaitCanceled,
    ApiLastError               // Do not add API errors below this line
} RETURN_CODE;






#ifdef __cplusplus
}
#endif
#include "Plx.ph"
#endif
