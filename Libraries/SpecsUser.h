// $Id: SpecsUser.h,v 1.1.1.1 2014/01/21 13:04:57 taurigna Exp $
//
// This Software uses the 
// PLX API libraries, Copyright (c) 2003 PLX Technology, Inc.
//

/** @file SpecsUser.h
 *  SpecsUser library interface. The purpose of this library is
 *  to provide simple access functions to use the Specs system.
 *  
 *  @author Marie-Helene Schune  schunem@lal.in2p3.fr
 *  @author Clara Gaspar         Clara.Gaspar@cern.ch
 *  @author Claude Pailler       pailler@lal.in2p3.fr
 *  @author Frederic Machefert   frederic.machefert@in2p3.fr
 *  @author Serge Du             du@lal.in2p3.fr
 *  @author Bruno Mansoux        mansoux@lal.in2p3.fr
 *  @author Patrick Robbe        robbe@lal.in2p3.fr
 *
 *  @date 25 Feb 2005
 */

#ifndef _SPECSUSER_H 
#define _SPECSUSER_H 1

// Automatic declaration of compilation symbols
#ifndef PCI_CODE
#define PCI_CODE
#endif // PCI_CODE

#ifndef PLX_LITTLE_ENDIAN
#define PLX_LITTLE_ENDIAN
#endif // PLX_LITTLE_ENDIAN

#ifndef WIN32
#ifndef PLX_LINUX
#define PLX_LINUX
#endif  // PLX_LINUX
#endif  // WIN32

#include "Specs.h"
#include <errno.h>
#include <stdio.h>
#ifdef PLX_LINUX
#ifdef SPECSUSER_EXPORTS
#define SPECSUSER_API
#else
#define SPECSUSER_API extern
#endif // SPECSUSER_EXPORTS
#else
#ifdef SPECSUSER_EXPORTS
#define SPECSUSER_API __declspec(dllexport)
#else
#define SPECSUSER_API __declspec(dllimport)
#endif // SPECSUSER_EXPORTS
#endif // PLX_LINUX

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**  Structure to contain the number of Specs opened for each card.
 *   The StDevLoc field can be used to access information about the
 *   Specs Master PCI card as shown in this example:
 *   @code
 *   DEVICE_INVENT device ;
 *   // Vendor Id of the Plx9030 chip (0x10b5 for PLX)
 *   printf(" Vendor Id: %X\n" , device.StDevLoc.VendorId ) ;
 *   // Device Id of the Plx9030 chip (0x9030)
 *   printf(" Device Id: %X\n" , device.StDevLoc.DeviceId ) ;
 *   // Bus number where the card is connectec
 *   printf(" Bus Number: %d\n" , device.StDevLoc.BusNumber ) ;
 *   // Slot number on the bus
 *   printf(" Slot Number: %d\n" , device.StDevLoc.SlotNumber ) ;
 *   @endcode
 */
typedef struct _DEVICE_INVENT
{
	DEVICE_LOCATION	StDevLoc;
	U32				Index;
} DEVICE_INVENT;

/** @defgroup errcodes Error Codes
 *  If the error code is 0, it means no error.
 *  If the error code is not 0, one or more errors have occured.
 * 
 *  Each bit of the SpecsError code corresponds to a predefined error.
 *  The resulting SpecsError code is a OR of all possible codes defined here:
 */
/*@{*/
/// Type for error code 16 bit integer.
typedef U16 SpecsError ;
/// No Error
static const SpecsError SpecsSuccess             = 0x0000 ;
/// One or more parameters of the function are not correct
static const SpecsError InvalidParameter         = 0x0001 ;

/** Specs Master does not allow write access.
 *  It corresponds to bit ::EnableWriteBit of Master Status Register.
 */
static const SpecsError WriteAccessDenied        = 0x0002 ;

/** Specs Master does not allow read access.
 *  It corresponds to bit ::EnableReadBit of Master Status Register.
 */
static const SpecsError ReadAccessDenied         = 0x0004 ;

/// Invalid size of Buffer
static const SpecsError InvalidBufferSize        = 0x0008 ;

/// No Frame separator in Specs Frame
static const SpecsError NoFrameSeparator         = 0x0010 ;

/// Frame separator Repetition in Specs Frame
static const SpecsError FrameSeparatorRepetition = 0x0020 ;

/// Header Repetition in Specs Frame
static const SpecsError HeaderRepetition         = 0x0040 ;

/// Wrong device information
static const SpecsError WrongDeviceInfo          = 0x0080 ;

/// No PLX Driver available
static const SpecsError NoPlxDriver              = 0x0100 ;

/// DCU Access Denied
static const SpecsError DCUAccessDenied          = 0x0200 ;

/// Specs Master used by another process (timeout)
static const SpecsError SpecsMasterLocked        = 0x0400 ;

/// Checksum error during the transaction
static const SpecsError ChecksumError            = 0x0800 ;
  
/// No I2C Acknowledge
static const SpecsError NoI2cAck                 = 0x1000 ;
  
/// Interrupt Timeout
static const SpecsError SpecsTimeout             = 0x2000 ;
/*@}*/

/** @defgroup i2cfunc I2c Functions
 *  Functions to read and write with I2c.
 */
/*@{*/
/** Write data to a i2c Slave.
 *  @ingroup i2c
 *  @param[in] theSlave       Specs Slave object to use to write data
 *  @param[in] outputSelect   Output select
 *  @param[in] i2cAddress     Address of the i2c Slave where to write 
 *                            data to
 *  @param[in] nValues        Number of 8 bit values to write 
 *  @param[in] data           Array containing the 8 bit values to send
 *  @return               ::InvalidParameter
 *  @return               ::WriteAccessDenied
 *  @return               ::InvalidBufferSize
 *  @return               ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_i2c_write(  SPECSSLAVE * theSlave , 
                                       U8 outputSelect       ,
                                       U8 i2cAddress         ,
                                       U8 nValues            ,
                                       U8 * data              ) ;

/** Write data to a i2c Slave using a sub-address.
 *  This function performs a write to a i2c Slave, the first byte transmitted
 *  after the i2c slave address (@a i2cAddress) is the sub-address 
 *  (@a i2cSubAdd).
 *  @param[in] theSlave     Specs Slave object to use to write data
 *  @param[in] outputSelect Output select
 *  @param[in] i2cAddress   Address of the i2c Slave where to write 
 *                          data to
 *  @param[in] i2cSubAdd    Sub-address 
 *  @param[in] nValues      Number of 8 bit values to write 
 *  @param[in] data         Array containing the 8 bit values to send
 *  @return                 ::InvalidParameter
 *  @return                 ::WriteAccessDenied
 *  @return                 ::InvalidBufferSize
 *  @return                 ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_i2c_write_sub(  SPECSSLAVE * theSlave , 
                                               U8 outputSelect       ,
                                               U8 i2cAddress         ,
                                               U8 i2cSubAdd          ,
                                               U8 nValues            ,
                                               U8 * data              ) ;

/** Write data to i2c Slaves using a hardware general call.
 *  In this mode, the first byte transmitted is 0, the second byte is the
 *  7-bit address of the hardware master (@a masterAddress), with a 1 as LSB.
 *  @param[in] theSlave      Specs Slave object to use to write data
 *  @param[in] outputSelect  Output select
 *  @param[in] masterAddress Address of the hardware master
 *  @param[in] nValues       Number of 8 bit values to write 
 *  @param[in] data          Array containing the 8 bit values to send
 *  @return                  ::InvalidParameter
 *  @return                  ::WriteAccessDenied
 *  @return                  ::InvalidBufferSize
 *  @return                  ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_i2c_general_write(  SPECSSLAVE * theSlave , 
                                                   U8 outputSelect       ,
                                                   U8 masterAddress      ,
                                                   U8 nValues            ,
                                                   U8 * data              ) ;

/** Write data to i2c Slaves using a hardware general call and a sub-address.
 *  In this mode, the first byte transmitted is 0, the second byte is the
 *  7-bit address of the hardware master (@a masterAddress), with a 1 as LSB, 
 *  and the third byte transmitted is the sub-address (@a i2cSubAdd).
 *  @param[in] theSlave      Specs Slave object to use to write data
 *  @param[in] outputSelect  Output select
 *  @param[in] masterAddress Address of the hardware master
 *  @param[in] i2cSubAdd     Sub-address
 *  @param[in] nValues       Number of 8 bit values to write 
 *  @param[in] data          Array containing the 8 bit values to send
 *  @return                  ::InvalidParameter
 *  @return                  ::WriteAccessDenied
 *  @return                  ::InvalidBufferSize
 *  @return                  ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_i2c_general_write_sub(  SPECSSLAVE * theSlave , 
                                                       U8 outputSelect       ,
                                                       U8 masterAddress      ,
                                                       U8 i2cSubAdd          ,
                                                       U8 nValues            ,
                                                       U8 * data            ) ;

/** Read data from a i2c Slave.
 *  @param[in]  theSlave     Specs Slave object to use to read data
 *  @param[in]  outputSelect Output select
 *  @param[in]  i2cAddress   Address of the i2c Slave where to read 
 *                           data from
 *  @param[in]  nValues      Number of 8 bit values to read 
 *  @param[out] data         Array containing the 8 bit values received
 *  @return                  ::InvalidParameter
 *  @return                  ::ReadAccessDenied
 *  @return                  ::InvalidBufferSize
 *  @return                  ::NoFrameSeparator
 *  @return                  ::FrameSeparatorRepetition
 *  @return                  ::HeaderRepetition
 *  @return                  ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_i2c_read(  SPECSSLAVE * theSlave , 
                                          U8 outputSelect       ,
                                          U8 i2cAddress         ,
                                          U8 nValues            ,
                                          U8 * data              ) ;

/** Read data from a i2c Slave with a sub-address.
 *  The first byte transmitted after the i2c slave address (@a i2cAddress)
 *  in the i2c frame is the sub-address (@a i2cSubAdd).
 *  @param[in]  theSlave     Specs Slave object to use to read data
 *  @param[in]  outputSelect Output select
 *  @param[in]  i2cAddress   Address of the i2c Slave where to read 
 *                           data from
 *  @param[in]  i2cSubAdd    Sub-address of the i2c Slave where to read 
 *                           data from
 *  @param[in]  nValues      Number of 8 bit values to read 
 *  @param[out] data         Array containing the 32 bit values 
 *                           received
 *  @return                  ::InvalidParameter
 *  @return                  ::ReadAccessDenied
 *  @return                  ::InvalidBufferSize
 *  @return                  ::NoFrameSeparator
 *  @return                  ::FrameSeparatorRepetition
 *  @return                  ::HeaderRepetition
 *  @return                  ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_i2c_read_sub(  SPECSSLAVE * theSlave , 
                                              U8 outputSelect       ,
                                              U8 i2cAddress         ,
                                              U8 i2cSubAdd          ,
                                              U8 nValues            ,
                                              U8 * data               ) ;

/** Read data from a i2c Slave with a combined format.
 *  The first part of the i2c frame is a write frame to the i2c slave address
 *  (@a i2cAddress)
 *  where the bytes of data transferred are the sub-addresses (@a i2cSubAdd). 
 *  This is followed by a @b repeated @b START @b condition and a read frame 
 *  to the i2c slave address (@a i2cAddress).
 *  @param[in]  theSlave     Specs Slave object to use to read data
 *  @param[in]  outputSelect Output select
 *  @param[in]  i2cAddress   Address of the i2c Slave where to read 
 *                           data from
 *  @param[in]  i2cSubAdd    Sub-addresses of the i2c Slave where to read 
 *                           data from (can be several bytes)
 *  @param[in]  subAddLength Number of butes to write as a sub-address
 *  @param[in]  nValues      Number of 8 bit values to read 
 *  @param[out] data         Array containing the 32 bit values 
 *                           received
 *  @return                  ::InvalidParameter
 *  @return                  ::ReadAccessDenied
 *  @return                  ::InvalidBufferSize
 *  @return                  ::NoFrameSeparator
 *  @return                  ::FrameSeparatorRepetition
 *  @return                  ::HeaderRepetition
 *  @return                  ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_i2c_combinedread(  SPECSSLAVE * theSlave , 
                                                  U8 outputSelect       ,
                                                  U8 i2cAddress         ,
                                                  U8 * i2cSubAdd        ,
                                                  U8 subAddLength       ,
                                                  U8 nValues            ,
                                                  U8 * data            ) ;

/*@}*/


/** @defgroup parfunc Parallel Bus Functions
 *  Functions to read and write via the parallel bus or DMA bus
 */
/*@{*/
/** Write on parallel bus. The function decides which mode to use: parallel
 *  mode when nValues is 1 and DMA mode when writing more than 1 value.
 *  @param[in] theSlave Specs Slave object to use to write data
 *  @param[in] address  Value of Address Bus
 *  @param[in] nValues  Number of 16 bit values to write (up to 127 included)
 *  @param[in] data     Array of 16 bit values of Data Bus
 *  @return             ::InvalidParameter
 *  @return             ::WriteAccessDenied
 *  @return             ::InvalidBufferSize
 *  @return             ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_parallel_write(  SPECSSLAVE * theSlave ,
                                                U8 address            ,
                                                U8 nValues            ,
                                                U16 * data             ) ;

/** Read on parallel bus. The function decides which mode to use: parallel
 *  mode when nValues is 1 and DMA mode when reading more than 1 value.
 *  @param[in]  theSlave Specs Slave object to use to read data
 *  @param[in]  address  Value of Address Bus
 *  @param[in]  nValues  Number of values to read 
 *  @param[out] data     Array of 16 bit values of Data Bus
 *  @return              ::InvalidParameter
 *  @return              ::ReadAccessDenied
 *  @return              ::InvalidBufferSize
 *  @return              ::NoFrameSeparator
 *  @return              ::FrameSeparatorRepetition
 *  @return              ::HeaderRepetition
 *  @return              ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_parallel_read( SPECSSLAVE * theSlave ,
                                              U8           address  ,
                                              U8           nValues  ,
                                              U16        * data       ) ;
/*@}*/

/** @defgroup jtagfunc JTAG Functions
 *  Functions to perform JTAG commands.
 */
/*@{*/

/** Set component in reset state.
 *  @param[in] theSlave     Specs Slave object to use
 *  @param[in] outputSelect Output Select
 *  @return                ::InvalidParameter
 *  @return                ::InvalidBufferSize
 *  @return                ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_jtag_reset( SPECSSLAVE * theSlave     ,
                                           U8           outputSelect   ) ;

/** Set component in idle state.
 *  @param[in] theSlave     Specs Slave object to use
 *  @param[in] outputSelect Output Select
 *  @return                ::InvalidParameter
 *  @return                ::InvalidBufferSize
 *  @return                ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_jtag_idle( SPECSSLAVE * theSlave     ,
                                          U8           outputSelect   ) ;


/** Perform an Instruction Register scan operation.
 *  @param[in]  theSlave     Specs Slave object to use
 *  @param[in]  outputSelect Output Select
 *  @param[out] dataIn       Input data
 *  @param[in]  dataOut      Output data
 *  @param[in]  nBits        Number of bits
 *  @return                ::InvalidParameter
 *  @return                ::InvalidBufferSize
 *  @return                ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_jtag_irscan( SPECSSLAVE * theSlave     ,
                                            U8           outputSelect ,
                                            U8         * dataIn       ,
                                            U8         * dataOut      ,
                                            U32          nBits          ) ;

/** Perform a Data Register scan operation.
 *  @param[in]  theSlave     Specs Slave object to use
 *  @param[in]  outputSelect Output Select
 *  @param[out] dataIn       Input data
 *  @param[in]  dataOut      Output data
 *  @param[in]  nBits        Number of bits
 *  @return                ::InvalidParameter
 *  @return                ::InvalidBufferSize
 *  @return                ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_jtag_drscan( SPECSSLAVE * theSlave     ,
                                            U8           outputSelect ,
                                            U8         * dataIn       ,
                                            U8         * dataOut      ,
                                            U32          nBits          ) ;

/*@}*/

/** @defgroup regfunc Register Functions
 *  Functions to read and write registers.
 */
/*@{*/

/** Write in register
 *  @param[in] theSlave    Specs Slave object to use to write in 
 *                         register
 *  @param[in] theRegister Register number where to write
 *  @param[in] data        16 bit value to write in register theRegister
 *  @return                ::InvalidParameter
 *  @return                ::WriteAccessDenied
 *  @return                ::InvalidBufferSize
 *  @return                ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_register_write(  SPECSSLAVE * theSlave ,
                                                U8 theRegister        ,
                                                U16 data               ) ;

/** Read register
 *  @param[in]  theSlave    Specs Slave object to use to read in register
 *  @param[in]  theRegister Register number to read
 *  @param[out] data        16 bit value content of theRegister 
 *  @return                ::InvalidParameter
 *  @return                ::ReadAccessDenied
 *  @return                ::InvalidBufferSize
 *  @return                ::NoFrameSeparator
 *  @return                ::FrameSeparatorRepetition
 *  @return                ::HeaderRepetition
 *  @return                ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_register_read(  SPECSSLAVE * theSlave ,
                                               U8 theRegister        ,
                                               U16 * data              ) ;
/*@}*/


/** @defgroup manfunc Management Functions
 *  Functions to manage SPECSSLAVE and SPECSMASTER objects.
 */
/*@{*/

/** Internal (soft) Reset of the Specs Slave
 *  @param[in] theSlave    Specs Slave to reset 
 *  @return                ::InvalidParameter
 *  @return                ::WriteAccessDenied
 *  @return                ::InvalidBufferSize
 *  @return                ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_slave_internal_reset(  SPECSSLAVE * theSlave ) ;

/** External Reset of the Specs Slave (equivalent to press the reset button
 *  of the mezzanine), with duration more than 1 ms.
 *  @param[in] theSlave    Specs Slave to reset 
 *  @return                ::InvalidParameter
 *  @return                ::WriteAccessDenied
 *  @return                ::InvalidBufferSize
 *  @return                ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_slave_external_reset( SPECSSLAVE * theSlave ) ;

/** External Short Reset of the Specs Slave (equivalent to press the reset button
 *  of the mezzanine), with short duration.
 *  @param[in] theSlave    Specs Slave to reset 
 *  @return                ::InvalidParameter
 *  @return                ::WriteAccessDenied
 *  @return                ::InvalidBufferSize
 *  @return                ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_slave_external_shortreset( SPECSSLAVE * theSlave ) ;
  
/** Reset Specs Master Boards (all specs masters on the same board).
 *  Use this function only in case of emergy ! 
 *  Always use ::specs_master_softreset to reset individually one master.
 *  After calling this function, the speed of the master is 0.
 *  @param[in] theMaster   Specs Master to reset
 *  @return                ::InvalidParameter
 *  @return                ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_master_reset(  SPECSMASTER * theMaster ) ;

/** Reset a single Specs Master Boards. 
 *  After calling this function, the speed of the master is set 
 *  to the last speed defined for this master.
 *  @param[in] theMaster   Specs Master to reset
 *  @return                ::InvalidParameter
 *  @return                ::SpecsMasterLocked
 *  @return                ::WriteAccessDenied
 */
SPECSUSER_API SpecsError specs_master_softreset(  SPECSMASTER * theMaster ) ;

/** Set speed of the Master: use this function to benefit from 
 *  protections.
 *  @param[in] theMaster  Specs Master to change speed
 *  @param[in] speed      Speed of the Master. 0, 1, 2, 3 or 4, will set the 
 *                        JTAG or I2C clock frequency to 1024 kHz, 512 kHz, 
 *                        256 kHz, 128 kHz or 64 kHz.
 *  @return               ::InvalidParameter
 *  @return               ::WriteAccessDenied
 *  @return               ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_master_setspeed( SPECSMASTER * theMaster , 
                                                U8 speed ) ;

/** Disable checksum verification in master
 *  @param[in] theMaster  Specs Master to mask
 *  @return               ::InvalidParameter
 *  @return               ::WriteAccessDenied
 *  @return               ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_master_maskchecksum( SPECSMASTER * theMaster ) ;

/** Enable checksum verification in master
 *  @param[in] theMaster  Specs Master to enable
 *  @return               ::InvalidParameter
 *  @return               ::WriteAccessDenied
 *  @return               ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_master_unmaskchecksum( SPECSMASTER * theMaster ) ;  

/** Reset Specs Master enable read status bit
 *  @param[in] theMaster   Specs Master to reset
 *  @return                ::InvalidParameter
 */

SPECSUSER_API SpecsError specs_master_enableread_reset( SPECSMASTER * 
                                                        theMaster ) ;

/** Reset Specs Master receiver FIFO
 *  @param[in] theMaster   Specs Master to reset
 *  @return                ::InvalidParameter
 */

SPECSUSER_API SpecsError specs_master_recFIFO_reset(  SPECSMASTER * 
                                                      theMaster ) ;

/** Connect a Specs Slave to a Specs Master. 
 *  @param[in]  theMaster    Specs Master to use to communicate with the Slave
 *  @param[in]  slaveAddress Slave address
 *  @param[out] theSlave     Specs Slave to connect. The structure 
 *                           SPECSSLAVE must be created (empty) 
 *                           before calling the function.
 *  @return                  ::InvalidParameter
 */

SPECSUSER_API SpecsError specs_slave_open(  SPECSMASTER * theMaster ,
                                            U8 slaveAddress         ,
                                            SPECSSLAVE * theSlave     ) ;

/** Obtain a list of Specs Master cards connected to the PCI bus
 *  @param[out] deviceList   Array of Specs master cards connected to 
 *                           the PCI bus in ::DEVICE_INVENT format, 
 *                           containing information about Slot number 
 *                           and Bus number
 *  @return     Number of SpecsMaster cards found
 *
 *  Here is an example how to use this function:
 *  @code
 *  DEVICE_INVENT DeviceList[MAX_CARD];
 *  U8 DevIndex;
 *  // List all cards connected to the PC
 *  DevIndex = specs_master_card_select(DeviceList);
 *  @endcode
 */
SPECSUSER_API U8 specs_master_card_select( DEVICE_INVENT * deviceList ) ;

/** Open the selected Specs Master for further use in the program.
 *  To release system resources properly, each ::specs_master_open 
 *  must be matched with one ::specs_master_close.
 *  It is safe to call this function on the same master card and with
 *  different MasterId, but this function should not be called twice using 
 *  the same master card AND the same index, unless the master is closed 
 *  inbetween. It is also good to call the functions ::specs_master_setspeed 
 *  after and ::specs_master_unmaskchecksum
 *
 *  @param[in]  theDevice      Device to open 
 *  @param[in]  MasterId       Id (1 to 4) of the master to use on the card 
 *  @param[out] theMasterCard  Empty SPECSMASTER object describing the 
 *                             device and which will be initialized by 
 *                             the function. 
 *                             The resulting SPECSMASTER object can be 
 *                             used to associate a SPECSSLAVE to it. 
 *  @return                       ::InvalidParameter
 *  @return                       ::WrongDeviceInfo
 *  @return                       ::NoPlxDriver
 *
 *  Here is an example how to use this function 
 *  (see also ::specs_master_card_select):
 *  @code
 *  SPECSMASTER      pSpm1[4], pSpm2[4], pSpm3[4], pSpm4[4];
 *  // Open 2 first masters on each card
 *  for (unsigned int i=0; i<DevIndex; i++)
 *  {
 *        if (specs_master_open(
 *                              DeviceList[i],
 *                              1,
 *                              &pSpm1[i] ) == SpecsSuccess)
 *        {
 *                 specs_master_open(
 *                                   DeviceList[i],
 *                                   2,
 *                                   &pSpm2[i]);
 *        }
 *   }
 *  @endcode
 */
SPECSUSER_API SpecsError specs_master_open(DEVICE_INVENT theDevice,
                                           U8 MasterId,
                                           SPECSMASTER * theMasterCard);
  
/** Close the Specs Master at the end of the program
 *  To release system resources properly, each ::specs_master_open 
 *  must be matched with one ::specs_master_close.
 *  It is safe to call this function on different specs master located 
 *  on the same card, but the function should not be called twice on
 *  the same master (same index, same card) unless the master is
 *  reopen inbetween.
 *
 *  @param[in] theMasterCard  Specs Master object to close. 
 *  @return                         ::WrongDeviceInfo
 *
 *  Here is an example how to use this function (see also ::specs_master_open):
 *  @code
 *  SpecsError rc;
 *
 *  specs_master_close(&pSpm2[Index]);
 *  rc = specs_master_close(&pSpm1[Index]);
 *  @endcode
 */
SPECSUSER_API SpecsError specs_master_close(SPECSMASTER * theMasterCard);

/// Serial number of the Specs board
SPECSUSER_API unsigned int specs_master_serialNumber( SPECSMASTER * 
                                                      theMasterCard ) ;

/// Major version of the library 
SPECSUSER_API unsigned short specs_major_version( ) ;

/// Minor version of the library
SPECSUSER_API unsigned short specs_minor_version( ) ;

/*@}*/

/** @defgroup dcu DCU Access and Control Functions
 */
/*@{*/
/** @defgroup dcureg DCU Registers 
 *  All the registers are read via internal I2C (output select = 0xF )
 *  The i2c address of the registers is ( DCUAddress << 3 ) | 
 * (RegisterAddress) 
 */
/*@{*/
/// Control Register (Read/Write)
static const U8 DcuCREG   = 0x00 ;

/// Status and data high register (Read only)
static const U8 DcuSHREG  = 0x01 ;

/// Auxiliary Register (Read/Write)
static const U8 DcuAREG   = 0x02 ;

/// Data Low Register (Read only)
static const U8 DcuLREG   = 0x03 ;

/// Test Register (Read/Write)
static const U8 DcuTREG   = 0x04 ; 
/*@}*/

/** @defgroup dcufunc DCU Functions
 *  Functions to control DCU chip on Slave Mezzanine.
 */
/*@{*/

/** Write a value to a DCU register.
 *  @param[in] theSlave        Specs Slave containing the DCU
 *  @param[in] registerAddress Address of the DCU register to write
 *  @param[in] data            Value to write 
 *  @return                    ::InvalidParameter
 *  @return                    ::WriteAccessDenied
 *  @return                    ::InvalidBufferSize
 *  @return                    ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_dcu_register_write(  SPECSSLAVE * theSlave ,
                                                    U8 registerAddress    ,
                                                    U8 data                ) ;

/** Read a value from a DCU register.
 *  @param[in]  theSlave        Specs Slave containing the DCU
 *  @param[in]  registerAddress Address of the DCU register to write
 *  @param[out] data            Read value 
 *  @return                     ::InvalidParameter
 *  @return                     ::ReadAccessDenied
 *  @return                     ::InvalidBufferSize
 *  @return                     ::NoFrameSeparator
 *  @return                     ::FrameSeparatorRepetition
 *  @return                     ::HeaderRepetition
 *  @return                     ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_dcu_register_read(  SPECSSLAVE * theSlave ,
                                                   U8 registerAddress    ,
                                                   U8 * data              ) ;

/** Perform an acquisition sequence over a DCU channel.
 *  @param[in] theSlave        Specs Slave containing the DCU
 *  @param[in] inputChannel    DCU channel to read ( 0 - 6 : normal 
 *                             channels, 7 : temperature channel )
 *  @param[out] data           Acquisition ADC value (12 bits)
 *  @param[out] convertedValue Acquisition ADC value (in Volt)
 *  @return                  ::InvalidParameter
 *  @return                  ::ReadAccessDenied
 *  @return                  ::WriteAccessDenied
 *  @return                  ::InvalidBufferSize
 *  @return                  ::NoFrameSeparator
 *  @return                  ::FrameSeparatorRepetition
 *  @return                  ::HeaderRepetition
 *  @return                  ::DCUAccessDenied 
 *  @return                  ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_dcu_acquire(  SPECSSLAVE * theSlave  ,
                                             U8 inputChannel        ,
                                             U16 * data             ,
                                             double * convertedValue ) ;

/** Software reset of the DCU chip.
 *  @param[in] theSlave   Specs Slave containing the DCU
 *  @return               ::InvalidParameter
 *  @return               ::ReadAccessDenied
 *  @return               ::WriteAccessDenied
 *  @return               ::InvalidBufferSize
 *  @return               ::NoFrameSeparator
 *  @return               ::FrameSeparatorRepetition
 *  @return               ::HeaderRepetition
 *  @return               ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_dcu_reset(  SPECSSLAVE * theSlave ) ;

/** Initialization sequence of the DCU chip.
 *  @param[in] theSlave   Specs Slave containing the DCU
 *  @return               ::InvalidParameter
 *  @return               ::ReadAccessDenied
 *  @return               ::WriteAccessDenied
 *  @return               ::InvalidBufferSize
 *  @return               ::NoFrameSeparator
 *  @return               ::FrameSeparatorRepetition
 *  @return               ::HeaderRepetition
 *  @return               ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_dcu_initialize(  SPECSSLAVE * theSlave ) ;  

/** Set LIR Mode for the DCU.
 *  @param[in] theSlave     Specs Slave containing the DCU
 *  @return                  ::InvalidParameter
 *  @return                  ::ReadAccessDenied
 *  @return                  ::WriteAccessDenied
 *  @return                  ::InvalidBufferSize
 *  @return                  ::NoFrameSeparator
 *  @return                  ::FrameSeparatorRepetition
 *  @return                  ::HeaderRepetition
 *  @return                  ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_dcu_set_LIR(  SPECSSLAVE * theSlave ) ;

/** Set HIR Mode for the DCU.
 *  @param[in] theSlave      Specs Slave containing the DCU
 *  @return                  ::InvalidParameter
 *  @return                  ::ReadAccessDenied
 *  @return                  ::WriteAccessDenied
 *  @return                  ::InvalidBufferSize
 *  @return                  ::NoFrameSeparator
 *  @return                  ::FrameSeparatorRepetition
 *  @return                  ::HeaderRepetition
 *  @return                  ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_dcu_set_HIR(  SPECSSLAVE * theSlave ) ;
  
/** Read DCU polarity mode (HIR or LIR mode).
 *  @param[in]  theSlave     Specs slave containing the DCU
 *  @param[out] mode         0 for HIR mode, 1 for LIR mode
 *  @return                ::InvalidParameter
 *  @return                ::ReadAccessDenied
 *  @return                ::WriteAccessDenied
 *  @return                ::InvalidBufferSize
 *  @return                ::NoFrameSeparator
 *  @return                ::FrameSeparatorRepetition
 *  @return                ::HeaderRepetition
 *  @return                ::SpecsMasterLocked
 */

SPECSUSER_API SpecsError specs_dcu_read_mode(  SPECSSLAVE * theSlave ,
                                               U8         * mode        ) ;
/*@}*/
/*@}*/

/** @defgroup master Specs Master Registers and Functions
 *  Functions and Registers for the Specs Master
 */
/*@{*/

/** @defgroup masterfunc Specs Master Register Functions
 *  Functions to access Master registers.
 */
/*@{*/

/** Write to specs master test register
 *  @param[in] theMaster    Specs master to use
 *  @param[in] value        16 bit value to write
 *  @retval              ::InvalidParameter
 */
SPECSUSER_API SpecsError specs_master_testreg_write( SPECSMASTER * theMaster ,
                                                     U16 value             ) ;

/** Read the specs master test register
 *  @param[in]  theMaster   Specs master to use
 *  @param[out] value       Read value
 *  @retval               ::InvalidParameter
 */
SPECSUSER_API SpecsError specs_master_testreg_read( SPECSMASTER * theMaster ,
                                                    U16 * value           ) ;

/** Read the date of the creation of the specs master prom
 *  @param[in] theMaster    Specs master
 *  @return                 Date of creation of master PROM
 */
SPECSUSER_API U32 specs_master_date_prom(  SPECSMASTER * theMaster ) ;
/*@}*/

/** @defgroup masterstatbits Master Status Register Bits
 *  They can be read with the SpecsmasterStatusRead function.
 *  Masks to access status register bits.
 */
/*@{*/
/// Interrupt Frame 
static const U32 ItFrame          = 0xC00000 ;

static const U32 ItIsUserInt      = 0x000000 ;
static const U32 ItIsChecksum     = 0x400000 ;
static const U32 ItIsI2cAck       = 0x800000 ;
static const U32 ItIsIO           = 0xC00000 ;
  
/// Pointer of the Master emitter FIFO
static const U16 EmiFIFOPointer   = 0xFF00 ;

/// If 1, error on the checksum (Master side)
static const U8 ChecksumErrorBit  = 0x80 ; 

/// If 1, read emitter FIFO is empty
static const U8 RdemptyEmiBit     = 0x40 ;

/// If 1, write emitter FIFO is full
static const U8 WrfullEmiBit      = 0x20 ; 

/// If 1, writing is allowed
static const U8 EnableWriteBit    = 0x10 ;

/// If 1, interrupt is detected (Slave side)
static const U8 ItDetected        = 0x08 ;

/// If 1, read receiver FIFO is empty
static const U8 RdemptyRecBit     = 0x04 ; 

/// If 1, write receiver FIFO is full
static const U8 WrfullRecBit      = 0x02 ; 

/// If 1, reading is allowed
static const U8 EnableReadBit     = 0x01 ;
/*@}*/

/** @defgroup masterctrlbits Master Control Register Bits
 *  They can be written with the SpecsmasterCtrlWrite function.
 */ 
/*@{*/
/** If set to 1 mask Checksum errors in the Master. @sa ::specs_master_unmaskchecksum
 *  and @sa ::specs_mastermaskchecksum
 */
static const U8 MaskChecksumBit = 0x80 ;

/// If set to 1, then to 0, reset Master receiver FIFO
static const U8 ResetRecFIFO    = 0x20 ;

/// If set to 1 then to 0, reset ::EnableReadBit
static const U8 ResetRdBit      = 0x10 ; 

/// If set to 1, software reset of the master
static const U8 ResetProgBit      = 0x08 ;

/** If set to 0, 1, 2, 3 or 4, set the JTAG or I2C clock frequency to 
 * 1024 kHz, 512 kHz, 256 kHz, 128 kHz or 64 kHz. @sa ::specs_master_setspeed
 */
static const U8 ClockDivisionBits = 0x07 ;
/*@}*/
/*@}*/

/** @defgroup slavereg Slave Registers
 *  Addresses and bit allocations of the slave registers.
 */
/*@{*/
/** @defgroup islavereg Internal Slave Registers Addresses
 *  They can be accessed with the ::specs_register_read and 
 *  ::specs_register_write functions at the address
 *  ::internalSlaveAddress
 */
/*@{*/
static const U8 internalSlaveAddress = 0x1F ; ///< Internal Slave address
static const U8 broadcastAddress     = 0x3F ; ///< Broadcast Address
static const U8 internalTestMSBReg   = 0x0 ; ///< MSB of test register 
static const U8 internalTestLSBReg   = 0x1 ; ///< LSB of test register
/*@}*/

/** @defgroup eslavereg Slave Registers Addresses 
 *  They can be accessed with the ::specs_register_read and
 *  ::specs_register_write functions.
 */
/*@{*/

/** Output pins Registers. 
 * When the pin is configured in output, the register controls the output
 * pin (using bit number i to control pin number i)
 * When the pin is configured in input, the register allows to read
 * the status of the output pin. (using bit number i to read pin number i)
 */
static const U8 RegOutMSBReg = 0x0 ; ///< Access to pins 16 to 31
static const U8 RegOutLSBReg = 0x1 ; ///< Access to pins 0 to 15

/** Configuration registers of the 32 output pins. 
 *  A 0 at the i^th position configures the pin number i 
 *  in input, a 1 in output.
 */
static const U8 confRegOutMSBReg = 0x2 ; ///< Configuration of pins 16 to 31 (for backward compatibility)
static const U8 ConfRegOutMSBReg = 0x2 ; ///< Configuration of pins 16 to 31
static const U8 ConfRegOutLSBReg = 0x3 ; ///< Configuration of pins 0 to 15

/** Control Register of the mezzanine (R/W). 
 * See also the @ref ctrlbits.
 */
static const U8 MezzaCtrlReg = 0x4 ;

/** @defgroup ctrlbits Bit Allocation of Control Register
 * Masks to access bits of the control register ::MezzaCtrlReg
 */
/*@{*/
/// Disable SCL Spy Control
static const U16 SCLSpyControl = 0x400 ;

/// Disable decoding of Calibration Channel B command
static const U16 CalibChannelBDisable = 0x200 ;

/// Disable decoding of L0 Reset Channel B command
static const U16 L0ChannelBDisable = 0x100 ;

/// Disable use of broadcast for Channel B decoding
static const U8 BckChannelBDisable = 0x80 ;

/// EEPROM OE (Internal Use)
static const U8 EEPROMOE = 0x40 ;

/// EEPROM SerEn and CE (Internal Use)
static const U8 EEPROMSerEn = 0x20 ;

/// Divide the mezzanine clock by 2
static const U8 ClkDivFact = 0x10 ;  

/** Mode of control of I2C_DE and JTAG_DE.
 *  If 1, static mode, if 0 dynamic mode.
 */
static const U8 StadynModeBit = 0x08 ;

/** Source of the mezzanine clock. 
 * If 0, local oscillator, if 1, use external clock
 */
static const U8 CmdOscillatorBit = 0x04 ; 

/// Control of the reset pin of the mezzanine 
static const U8 OutputExternalResetBit = 0x02 ;
 
/// Global reset of the mezzanine
static const U8 GlobalSoftwareResetBit = 0x01 ;
/*@}*/

/** Status Register of the mezzanine (R).
 *  See also the @ref statbits.
 */
static const U8 MezzaStatReg = 0x5 ; 

/** @defgroup statbits Bit Allocation of Status Register
 * Masks to access bits of the status register ::MezzaStatReg
 */
/*@{*/
/// Masks to access bits of the status register

/// Master/Slave resistor status: if 0, resistor, if 1, no resistor.
static const U16 MSResistorBit = 0x100 ;
  
/// Master/Slave bit: if 0, Mezzanine in Master mode, if 1, in slave mode.
static const U16 MastslaveBit = 0x200 ;
/*@}*/

/** Serial Bus Static Output Selection Register (R/W).
 *  When the mode is static, this register controls the DE/RE lines 
 *  directly (16 bit register). Set bit i to 1 to drive bus 
 *  with output select i staticly.
 */
static const U8 BusSelReg = 0x6 ;

/** Interrupt vector, contains the meaning of the last interrupt generated.
 *  Reading the register clears it. See also @ref intbits.
 */
static const U8 IntVect = 0x7 ;

/// Date of the Mezzanine PROM in MMDD format (R)
static const U8 DatePROMReg = 0x8 ;

/** Interrupt vector definition, declares (write 1) or masks (write 0) 
 *  interrupts to generate.
 *  See also the @ref intbits.
 */
static const U8 IntDefineVect = 0x9 ;

/** @defgroup intbits Bit Allocation of Interrupt Vector Registers
 * Masks to access bits of ::IntVect or ::IntDefineVect 
 */
/*@{*/ 
/// Mask or active User Interrupt
static const U8 UserInterruptBit = 0x01 ;

/// Mask or active Checksum Error
static const U8 ChecksumBit = 0x02 ;
  
/// Mask or active I2C acknowledge error
static const U8 I2CAckInterruptBit = 0x04 ;
  
/// Mask or active I/O 0 interrupt
static const U8 IO0InterruptBit = 0x08 ;

/// Mask or active I/O 1 interrupt
static const U8 IO1InterruptBit = 0x10 ;

/// Mask or active I/O 2 interrupt
static const U8 IO2InterruptBit = 0x20 ;

/// Mask or active I/O 3 interrupt
static const U8 IO3InterruptBit = 0x40 ;

/// Mask or active I/O 4 interrupt
static const U8 IO4InterruptBit = 0x80 ;

/// Mask or active I/O 5 interrupt
static const U16 IO5InterruptBit = 0x100 ;

/// Mask or active I/O 6 interrupt
static const U16 IO6InterruptBit = 0x200 ;

/// Mask or active I/O 7 interrupt
static const U16 IO7InterruptBit = 0x400 ;  
/*@}*/
  
/** 8 bit register to program the delay of the Calib 0 bit of the 
 *  Channel B with 25 ns steps.
 */
static const U8 ChanBDelayReg = 0xA ;
  /*@}*/
  /*@}*/

/** @defgroup eepromfunc On board EEPROM Functions
 *  Functions to communicate with the on board EEPROM.
 */
/*@{*/

/** Write data to the EEPROM.
 *  Data are written pages by pages into the EEPROM.
 *  A page contains 64 bytes and the EEPROM contains 128 pages.
 *  The data can be written at any location in a given page, at the end
 *  of the page, writting continues from the beginning of the same page.
 *  All data to be written in a given page must be sent in one function
 *  call. If the number of bytes written is less than 64, the unspecified
 *  data values will be erased from the EEPROM and will be replaced by 
 *  0xFF.
 *  Page 0 is reserved to store, starting at address 0:
 *     - the Mezzanine Serial Number (16 bits) 
 *     - the DCU calibration constant: 
 *         100000000 * A(InCh0) [LIR], 1000 * B(InCh0) [LIR],
 *         100000000 * -A(InCh0) [HIR], 1000 * B(InCh0) [HIR]
 *          ......
 *
 *  @param[ in ] theSlave     Specs Slave object to use to write data
 *  @param[ in ] pageNumber   Page number where to write  
 *  @param[ in ] startAddress Address where to start writing data
 *  @param[ in ] nValues      Number of values to write
 *  @param[ in ] data         Values to write
 *  @return                   ::InvalidParameter
 *  @return                   ::WriteAccessDenied
 *  @return                   ::InvalidBufferSize
 *  @return                   ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_slave_write_eeprom( SPECSSLAVE * theSlave ,
                                                   U8 pageNumber         ,
                                                   U8 startAddress       ,
                                                   U8 nValues            ,
                                                   U8 * data               ) ;

/** Read data from the on board EEPROM.
 *  See ::specs_write_eeprom for address definition, the difference for
 *  read access is that at the end of one page, the reading continues
 *  on the next page, at first byte.
 *  @param[in]  theSlave         Specs Slave object to use to read data
 *  @param[in]  pageNumber       Page number where to read
 *  @param[in]  startAddress     Address where to start reading data
 *  @param[in]  nValues          Number of values to read (not equal to 25 !)
 *  @param[out] data             Read values
 *  @return                      ::InvalidParameter
 *  @return                      ::ReadAccessDenied
 *  @return                      ::InvalidBufferSize
 *  @return                      ::NoFrameSeparator
 *  @return                      ::FrameSeparatorRepetition
 *  @return                      ::HeaderRepetition
 *  @return                      ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_slave_read_eeprom( SPECSSLAVE * theSlave ,
                                                  U8 pageNumber         ,
                                                  U8 startAddress       ,
                                                  U8 nValues            ,
                                                  U8 * data               ) ;

/** Read serial number of Specs Mezzanine in EEPROM
 *  @param[in]  theSlave    Specs Mezzanine
 *  @param[out] SN          Value of serial number (16 bits)
 *  @return                 ::InvalidParameter
 *  @return                 ::ReadAccessDenied
 *  @return                 ::InvalidBufferSize
 *  @return                 ::NoFrameSeparator
 *  @return                 ::FrameSeparatorRepetition
 *  @return                 ::HeaderRepetition
 *  @return                 ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_slave_serialNumber( SPECSSLAVE * theSlave ,
                                                   U16 * SN ) ;

/** Read DCU calibration constants in EEPROM
 *  @param[in]  theSlave    Specs Mezzanine
 *  @return                 ::InvalidParameter
 *  @return                 ::ReadAccessDenied
 *  @return                 ::InvalidBufferSize
 *  @return                 ::NoFrameSeparator
 *  @return                 ::FrameSeparatorRepetition
 *  @return                 ::HeaderRepetition
 *  @return                 ::SpecsMasterLocked
 */
SPECSUSER_API SpecsError specs_slave_dcuconstant( SPECSSLAVE * theSlave ,
                                                  U8 DCUChannel , 
                                                  U8 DCUMode , 
                                                  double * a , 
                                                  double * b ) ;
  

/*@}*/


/** @defgroup intfunc Interrupt functions
 *  <b>Functions to use SPECS interrupts</b>
 *
 *  To activate SPECS interrupts, an interrupt vector defining the 
 *  types of interrupts to activate must be written in the ::IntDefineVect
 *  register. The value to write in the register is described in 
 *  @ref intbits.
 * 
 *  Two interrupts will generate an error when they are activated during
 *  a SPECS access: ::ChecksumBit (error during transmission from master
 *  to slave) and ::I2CAckInterruptBit (no acknowledge during I2C accesses).
 *  The error code returned by the function during which the interrupt
 *  occured will indicate the error corresponding to it, that is to say
 *  ::ChecksumError and ::NoI2cAck, respectively.
 *
 *  The other interrupts (::UserInterruptBit, ::IO0InterruptBit, 
 *  ::IO1InterruptBit, ::IO2InterruptBit, ::IO3InterruptBit, 
 *  ::IO4InterruptBit, ::IO5InterruptBit, ::IO6InterruptBit and
 *  ::IO7InterruptBit) are user interrupts and are triggered by 
 *  an input pin of the Specs Slave. Checking these user interrupts
 *  must be done with ::specs_slave_waitForInterrupt, which returns
 *  when one of the specified interrupts has been triggered.
 *  
 *  Interrupts must be acknowledged before a new interrupt can be
 *  generated, reading the ::IntVect slave register. But this is 
 *  done automatically by the library, for all types of interrupts.
 * 
 *  Note that by default and after a slave reset, all interrupts
 *  are desactivated. They must be activated by the user before being used,
 *  writing to the ::IntDefineVect register.
 *
 *  Here is an example how to wait for user interrupts:
 *  @code
 *  // Activate UserInterrupt and IO5 as user interrupts.
 *  SpecsError theError = specs_register_write( theSlave , 
 *                            IntDefineVect , UserInterruptBit | IO5InterruptBit ) ;
 *
 *  // Clear IntVect 
 *  U16 intVect ;
 *  theError = specs_register_read( theSlave , IntVect , &intVect ) ;
 * 
 *  // Now wait for one of the interrupt to occur before a delay
 *  // of 100s
 *  U16 itOut ;
 *  theError = specs_slave_waitForInterrupt( theSlave , 
 *                 UserInterruptBit | IO5InterruptBit , 100 * 1000 ,
 *                 &itOut ) ;
 *
 *  // Check the interrupt received
 *  if ( SpecsSuccess == theError ) {
 *     if ( 0 != ( UserInterruptBit & itOut ) ) 
 *         printf( "User interrupt received !\n" ) ;
 *     if ( 0 != ( IO5InterruptBit & itOut ) ) 
 *         printf( "IO5 interrupt received !\n" ) ;
 *  } else if ( SpecsTimeout == theError ) 
 *     printf( "No interrupt received.\n" ) ;
 *  @endcode
 */
/*@{*/

/** Wait for a SPECS interrupt to occur
 *  @param[in]  theSlave      Specs Mezzanine attached to the interrupt source
 *  @param[in]  InterruptType Type of the interrupts to wait for: use constants
 *                            defined in @ref intbits
 *  @param[in]  timeOut       Desired time to wait, in second
 *  @param[out] ItOut         Type of interrupt received (See constants defined
 *                            in @ref intbits) 
 *  @return                   ::SpecsSuccess if the interrupt has occured
 *  @return                   ::SpecsTimeout if the interrupt has not occured
 *  @return                   ::InvalidParameter
 */
SPECSUSER_API SpecsError specs_slave_waitForInterrupt( SPECSSLAVE * theSlave ,
                                                       U16 InterruptType , 
                                                       U32 timeOut , 
                                                       U16 * ItOut ) ;

/*@}*/

#ifdef __cplusplus
}
#endif // __cplusplus
                                               
#endif // _SPECSAPI_H
