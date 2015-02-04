// $Id: SpecsUser.cpp,v 1.9 2014/07/10 13:48:57 taurigna Exp $
//
// This Software uses the 
// PLX API libraries, Copyright (c) 2003 PLX Technology, Inc.
//

/** @file SpecsUser.cpp
 *  Implementation file for SpecsUser library
 *  
 *  @author Monique Taurigna    taurigna@lal.in2p3.fr
 *  @author Marie-Helene Schune schunem@lal.in2p3.fr
 *  @author Clara Gaspar        Clara.Gaspar@cern.ch
 *  @author Claude Pailler      pailler@lal.in2p3.fr
 *  @author Frederic Machefert  frederic.machefert@in2p3.fr
 *  @author Patrick Robbe       robbe@lal.in2p3.fr
 *  @author Bruno Mansoux       mansoux@lal.in2p3.fr
 *  @author Serge Du            du@lal.in2p3.fr
 *  @date 25 Feb 2005
 */

#include "SpecsUser.h"
#include <cstdlib>
#include <vector>
#include <map>
#include <sstream> 
#include <iostream>
#include <cstring>
using namespace std ;
#include <cmath>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
extern int errno ;
#include <signal.h>

#include "AltSpecsV2.h"
#define GLOBAL_REG PIO_STATUS_REG
// Version of the library
#define SpecsUserMajorVersion 9
#define SpecsUserMinorVersion 0

static struct timespec LockDelay = { 1 , 0 } ;

// Delay to wait for Slave interrupt
static unsigned int ItDelay = 100 ;

// DCU output select
static const U8 DcuOutputSelect = 0xE ;  

// EEPROM output select
static const U8 EEPROMOutputSelect = 0xC ;

// DCU Address
static const U8 DcuAddress = 0x0 ;

// EEPROM Address
static const U8 EEPROMAddress = 0x53 ;

// DCU Speed
static const U8 DCUSpeed = 3 ;

// Utility functions...
// Check Validity of parameters at the begining of each function
SpecsError  specsUserCheckParameters(  SPECSSLAVE * theSlave );
// READ Receiver FIFO of the Specs
SpecsError  specsUserFIFORead( SPECSMASTER * pSpecsmaster, U32 *pData,
                               U32 Length);
// JTAG intermediate function
SpecsError  JtagWriteReadMany( SPECSSLAVE * pSpecsslave, U8 outSelect,
                               U8 *pDataIn, U8 *pDataOut, U32 nBits,
                               JTAG_OPER oper, int header, int trailler);
// Read speed of the Master
U8 getSpeed( SPECSMASTER * theMaster ) ;

// Check if checksum is maked
U8 isChecksumMasked( SPECSMASTER * theMaster ) ;

// Check interrupt from Slave
SpecsError checkSlaveIt( SPECSSLAVE * pSpecsslave ) ;

// Read without IT
SpecsError specs_register_read_withoutIt( SPECSSLAVE * theSlave ,
                                          U8 theRegister ,
                                          U16 * data , 
                                          bool checkLock ) ;

// Create Semaphore (Linux)
bool specsCreateSemaphore( bool * semExists , int * semId ,
                           std::string inputKey ,
                           int secondaryKey , int nSem , 
                           std::map< SPECSMASTER * , int > * theMap , 
                           SPECSMASTER * theMasterCard ) ;

struct compPair {
  bool operator()( const std::pair< SPECSMASTER * , unsigned short > &p1 , 
                   const std::pair< SPECSMASTER * , unsigned short > &p2 ) 
    const {
    if ( p1.first != p2.first ) return p1.first < p2.first ;
    else return p1.second < p2.second ;
  }
} ;

// Map to contain Master Id for each specs master
std::map< SPECSMASTER * , unsigned short > idMap ;
typedef std::map< SPECSMASTER * , unsigned short >::iterator idMapIterator ;

// Map for recursive locks
std::map< SPECSMASTER * , bool > recursiveLock ;

// Map for semaphores
std::map< SPECSMASTER * , int > mutexMap ;
typedef std::map< SPECSMASTER * , int >::iterator mutexMapIterator ;

// Map to contain IT2 events for each master
std::map< SPECSMASTER * , PLX_NOTIFY_OBJECT > intEventMap ;
typedef std::map< SPECSMASTER * , PLX_NOTIFY_OBJECT >::iterator 
intEventMapIterator ;

PLX_INTR IntSources ;

std::map< SPECSMASTER * , std::string > serialIdMap ;
  
static void startSpecsUser(void) __attribute__ ((constructor));
static void stopSpecsUser(void) __attribute__ ((destructor));
void stopSpecsUser_signal( int s ) ;
void startSpecsUser(void) {

  idMap.clear() ;
  intEventMap.clear() ;


  memset( &IntSources , 0 , sizeof( PLX_INTR ) ) ;
  // MTQ N'est plus utilise    IntSources.IopToPciInt_2 = 1 ;
  //  IntSources.PIO_Line =0x01000008;
  IntSources.PIO_Line =0x01;
  signal( SIGINT , stopSpecsUser_signal ) ;

}
void stopSpecsUser_signal( int s ) 
{
  stopSpecsUser( ) ;
  exit( 1 ) ;
}

void stopSpecsUser( void ) {
  int MasterId;
 
  for (MasterId =0; MasterId<NB_MASTER;MasterId ++)
    if (EtatDev[MasterId] != -1) 
      //specs_master_close( idMap[MasterId]);
      { printf(" Close Master %d  \n",MasterId+1);
	PlxPciDeviceClose(EtatDev[MasterId],MasterId);
	EtatDev[MasterId]= -1;
      }
}

//========================================================================
// Function to reserve the Master to protect against concurrent accesses
//========================================================================
SpecsError reserveMaster_checkRecursive( SPECSMASTER * theMaster ) {
  struct sembuf operation ;
  int res ;

  if ( ! recursiveLock[ theMaster ] ) {
    operation.sem_num = 0 ;
    operation.sem_op  = -1 ;
    operation.sem_flg = SEM_UNDO ;
    
    bool retry = true ;
    while ( retry ) {
      retry = false ;
      res = semtimedop( mutexMap[ theMaster ] , &operation , 1 , 
			&LockDelay ) ;
      if ( res < 0 ) { // is interrupted system call, retry 
	if ( EINTR == errno ) {
	  retry = true ;
	} else {
	  perror("") ; return SpecsMasterLocked ; 
	}
      } 
    }
  }
  return SpecsSuccess ;
}

// Without checking recursivity

SpecsError reserveMaster( SPECSMASTER * theMaster ) {
  struct sembuf operation ;
  int res ;

  operation.sem_num = 0 ;
  operation.sem_op  = -1 ;
  operation.sem_flg = SEM_UNDO ;
  
  bool retry = true ;
  while ( retry ) {
    retry = false ;
    res = semtimedop( mutexMap[ theMaster ] , &operation , 1 , 
		      &LockDelay ) ;
    if ( res < 0 ) { // is interrupted system call, retry 
      if ( EINTR == errno ) {
	retry = true ;
      } else {
	perror("") ; return SpecsMasterLocked ; 
      }
    } 
  }
  return SpecsSuccess ;
}

//========================================================================
// Function to reserve the Master to protect against concurrent accesses
//========================================================================
SpecsError releaseMaster_checkRecursive( SPECSMASTER * theMaster ) {
  struct sembuf operation ;
  int res ;

  if ( ! recursiveLock[ theMaster ] ) {
    operation.sem_num = 0 ;
    operation.sem_op  = 1 ;
    operation.sem_flg = SEM_UNDO ;
    res = semop( mutexMap[ theMaster ] , &operation , 1 ) ;
  }

  return SpecsSuccess ;
}
// without recursivity check
SpecsError releaseMaster( SPECSMASTER * theMaster ) {
  struct sembuf operation ;
  int res ;

  operation.sem_num = 0 ;
  operation.sem_op  = 1 ;
  operation.sem_flg = SEM_UNDO ;
  
  res = semop( mutexMap[ theMaster ] , &operation , 1 ) ;

  return SpecsSuccess ;
}

//========================================================================
//
//========================================================================
unsigned short specs_major_version( ) {
  return SpecsUserMajorVersion ;
}

//========================================================================
//
//========================================================================
unsigned short specs_minor_version( ) {
  return SpecsUserMinorVersion ;
}

//========================================================================
//
//========================================================================
unsigned int specs_master_serialNumber( SPECSMASTER * theMasterCard ) { 
  U32 EepromData[ 4 ] ;
  for ( int ii = 0 ; ii < 4 ; ++ii ) EepromData[ ii ] = 0 ;
  unsigned int serId ;
  serId = EepromData[ 3 ] ;
  serId=0;
  return serId ;  
}

//========================================================================
//
//========================================================================
unsigned int board_serialNumber( HANDLE * hdle ) {
  U32 EepromData[ 4 ] ;
  for ( int ii = 0 ; ii < 4 ; ++ii ) EepromData[ ii ] = 0 ;
  unsigned int serId ;
  serId = EepromData[ 3 ] ;
  serId=0;//MTQ
  return serId ;  
}

//========================================================================
//
//========================================================================
SpecsError specs_i2c_write( SPECSSLAVE * theSlave , 
                            U8 outputSelect       ,
                            U8 i2cAddress         ,
                            U8 nValues            ,
                            U8 * data              ) {
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( theError != SpecsSuccess ) return theError ;

  RETURN_CODE errorCodeFromLowLevelSpecsLib ;

  // Wait for master to be free and block it
  theError = reserveMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  errorCodeFromLowLevelSpecsLib = 
    I2cBufferWrite(theSlave,outputSelect,i2cAddress,data,nValues);
  theError |= checkSlaveIt( theSlave ) ;

  // Release the master
  theError |= releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  
  switch ( errorCodeFromLowLevelSpecsLib ) {
  case ApiSuccess:
    theError |= SpecsSuccess ; break ;
  case ApiAccessDenied:
    theError |= WriteAccessDenied ; break ;
  case ApiInvalidSize:
    printf( "Invalid buffer size\n" );
    theError |= InvalidBufferSize ; break ;
  default: 
    break ; 
  }

  return theError ; 
}

//========================================================================
//
//========================================================================
SpecsError specs_i2c_write_sub( SPECSSLAVE * theSlave , 
                                U8 outputSelect       ,
                                U8 i2cAddress         ,
                                U8 i2cSubAdd          ,
                                U8 nValues            ,
                                U8 * data              ) {
  SpecsError theError = specsUserCheckParameters( theSlave );
  if ( theError != SpecsSuccess ) return theError ;

  U8 newData[256] ; 
  newData[0] = i2cSubAdd ; 
  for (unsigned int i=0 ; i< nValues ; ++i){
    newData[i+1] = data[i];
  }

  RETURN_CODE errorCodeFromLowLevelSpecsLib ;

  theError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;
  
  errorCodeFromLowLevelSpecsLib = 
    I2cBufferWrite(theSlave,outputSelect,i2cAddress,newData,nValues+1);
  theError |= checkSlaveIt( theSlave ) ;

  theError |= releaseMaster( theSlave -> pSpecsmaster ) ;
  return theError ; 
}

//========================================================================
//
//========================================================================
SpecsError specs_i2c_general_write( SPECSSLAVE * theSlave , 
                                    U8 outputSelect       ,
                                    U8 masterAddress      ,
                                    U8 nValues            ,
                                    U8 * data              ) {
  SpecsError theError = specsUserCheckParameters( theSlave );
  if ( theError != SpecsSuccess ) return theError ;

  U8 newData[256] ; 
  newData[0] = ( masterAddress & 0x7F << 1 ) ; 
  for (unsigned int i=0 ; i< nValues ; ++i){
    newData[i+1] = data[i];
  }
  RETURN_CODE errorCodeFromLowLevelSpecsLib ;

  theError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  errorCodeFromLowLevelSpecsLib = 
    I2cBufferWrite(theSlave,outputSelect,0x0,newData,nValues+1);
  theError |= checkSlaveIt( theSlave ) ;

  theError |= releaseMaster( theSlave -> pSpecsmaster ) ;

  switch ( errorCodeFromLowLevelSpecsLib ) {
  case ApiSuccess:
    theError |= SpecsSuccess ; break ;
  case ApiAccessDenied:
    theError |= WriteAccessDenied  ; break ;
  case ApiInvalidSize:
    theError |= InvalidBufferSize ; break ;
  default:
    break ;
  }
  
  return theError ; 
}
//========================================================================
//
//========================================================================
SpecsError specs_i2c_general_write_sub( SPECSSLAVE * theSlave , 
                                        U8 outputSelect       ,
                                        U8 masterAddress      ,
                                        U8 i2cSubAdd          ,
                                        U8 nValues            ,
                                        U8 * data              ) {
  SpecsError theError = specsUserCheckParameters( theSlave );
  if ( theError != SpecsSuccess ) return theError ;

  U8 newData[256] ; 
  newData[0] = ( masterAddress & 0x7F << 1 ) ;
  newData[1] = i2cSubAdd ;
  for (unsigned int i=0 ; i< nValues ; ++i){
    newData[i+2] = data[i];
  }
  RETURN_CODE errorCodeFromLowLevelSpecsLib ;

  theError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  errorCodeFromLowLevelSpecsLib = 
    I2cBufferWrite(theSlave,outputSelect,0x0,newData,nValues+2);
  theError |= checkSlaveIt( theSlave ) ;

  theError |= releaseMaster( theSlave -> pSpecsmaster ) ;
  
  switch ( errorCodeFromLowLevelSpecsLib ) { 
  case ApiSuccess:
    theError |= SpecsSuccess ; break ;
  case ApiAccessDenied:
    theError |= WriteAccessDenied  ; break ;
  case ApiInvalidSize:
    theError |= InvalidBufferSize ; break ;
  default:
    break ;
  }
  
  return theError ; 
}

//=======================================================================
//
//=======================================================================
SpecsError specs_i2c_read( SPECSSLAVE * theSlave , 
                           U8 outputSelect       ,
                           U8 i2cAddress         ,
                           U8 nValues            ,
                           U8 * data              ) {
  SpecsError theError = specsUserCheckParameters( theSlave );
  if ( theError != SpecsSuccess ) return theError ;

  SPECSMASTER* specsMtr = theSlave->pSpecsmaster ;
  U32 pI2cData[256];
  U32 specsData[256];

  for ( int ii = 0 ; ii < 256 ; ++ii ) {
    pI2cData[ ii ] = 0 ;
    specsData[ ii ] = 0 ;
  }

  U32 nWordsSpecs ; 
  double nValuesCorrDouble ;
  int idata ;
  int nFrame ;
  int nMaxValues ;
  int nToRead ;
  int idata2 ;

  RETURN_CODE errorCodeFromLowLevelSpecsLib ;

  theError = reserveMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  errorCodeFromLowLevelSpecsLib = 
    I2cBufferRead(theSlave,outputSelect,i2cAddress,pI2cData,nValues); 
  
  switch ( errorCodeFromLowLevelSpecsLib ) {
  case ApiSuccess:
    theError = SpecsSuccess ; break ;
  case ApiAccessDenied:
    theError = WriteAccessDenied ; break ;
  case ApiInvalidSize:
    theError = InvalidBufferSize ; break ;
  default:
    break ;
  }
  
  if ( SpecsSuccess != theError ) {
    releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
    return theError ;
  }    

  // Check slave IT
  theError |= checkSlaveIt( theSlave ) ;

  idata = 0 ;
  nFrame = 0 ;
  /* mis plus bas comme a l'origine 
     nValuesCorrDouble = ( nValues  ) + 4 + 3 ;
     nWordsSpecs = (U32) std::ceil(nValuesCorrDouble/4.0 )+((nValues/(9*4))*2);
     // on recoit un header de 8 bytes tous les 9 mots
     theError |= specsUserFIFORead( specsMtr, specsData,nWordsSpecs ) ;
  */
  while ( idata < nValues ) {
  
    if ( 0 == nFrame ) nMaxValues = 0x1B ;
    else nMaxValues = 0x1C ;
   
    if ( ( nValues - idata ) > nMaxValues ) { 
      nWordsSpecs = 9 ;
      nToRead = nMaxValues ;
    } else {
      // nbre de mots de 8 bits que l on veut lire. 
      // comme le specs donne des mots de 32 bits et que le 1er est
      // dummy on a donc a lire [nValues + 4 (pour le 1er mot dummy) + 3 
      // (pour les 24 bits dummy du 2eme mot)]/4 

      nValuesCorrDouble = ( nValues - idata ) + 4 + 3 ;
     
      // number of
      // 4-octet words (Specs) prend le 1er entier au dessus
      //
      // nWordsSpecs est le nombre de mots de 32 bits
      // nValues est le nbre de mots de 8 bits 
      //
      //     nWordsSpecs = (U32) std::ceil(nValuesCorrDouble/4.0 );
      nWordsSpecs = (U32) std::ceil(nValuesCorrDouble/4.0 );//MTQ  verifie
      nToRead = ( nValues - idata ) ;
 
    }
    // 
    // le bazar sur la FIFO ..
    //enleve read
      
    IntSources.PIO_Line =0x01;
    PlxIntrEnable(
		  specsMtr->hdle,
		  &IntSources,
		  specsMtr->pSpecsmasterReceiverFifo
		  );
   
    // SpecsmasterEnableInt ( specsMtr,1);
  
    theError |= specsUserFIFORead( specsMtr, specsData,nWordsSpecs ) ;
    nFrame++ ;
    
    idata2 = 0 ;
    
    if ( theError == SpecsSuccess ) {
      data[ idata ] = static_cast< U8 > ( specsData[1] & 0xFF ) ;
      for (unsigned int iwordSpecs = 2 ;  iwordSpecs <= nWordsSpecs ; 
	   iwordSpecs++){
	idata++ ; idata2++ ; if ( idata2 >= nToRead ) break ;
	data[idata] = static_cast< U8 > 
	  ( ( specsData[iwordSpecs] & 0xFF000000 ) >> 24 ) ; 
	idata++ ; idata2++ ; if ( idata2 >= nToRead ) break ;
	data[idata] = static_cast< U8 > ( ( specsData[iwordSpecs] & 
					    0xFF0000 ) >> 16 ) ; 
	idata++ ; idata2++ ; if ( idata2 >= nToRead ) break ;
	data[idata] = static_cast< U8 > ( ( specsData[iwordSpecs] & 
					    0xFF00 ) >> 8 ) ; 
	idata++ ; idata2++ ; if ( idata2 >= nToRead ) break ;
	data[idata] = static_cast< U8 > ( specsData[iwordSpecs] & 0xFF ) ; 
      }
    } else break ;
  }

  theError |= releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;

  return theError ; 
}
//========================================================================
//
//========================================================================
SpecsError specs_i2c_read_sub(  SPECSSLAVE * theSlave , 
                                U8 outputSelect       ,
                                U8 i2cAddress         ,
                                U8 i2cSubAdd          ,
                                U8 nValues            ,
                                U8 * data                    ) {
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( theError != SpecsSuccess ) return theError ;
  SPECSMASTER* specsMtr = theSlave->pSpecsmaster ;
  
  U32 pI2cData[256];
  U32 specsData[256];

  for ( int ii = 0 ; ii < 256 ; ++ii ) {
    pI2cData[ ii ] = 0 ;
    specsData[ ii ] = 0 ;
  }

  int idata ;
  int nFrame ;
  int nMaxValues , nToRead ;
  double nValuesCorrDouble ;
  U32 nWordsSpecs ;
  int idata2 ;

  RETURN_CODE errorCodeFromLowLevelSpecsLib ;

  theError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  errorCodeFromLowLevelSpecsLib = 
    I2cBufferReadwithSsAdd(theSlave,outputSelect,i2cAddress,i2cSubAdd,
			   pI2cData,nValues);
  switch ( errorCodeFromLowLevelSpecsLib ) {
  case ApiSuccess:
    theError = SpecsSuccess ; break ;
  case ApiAccessDenied:
    theError = WriteAccessDenied ; break ;
  case ApiInvalidSize:
    theError = InvalidBufferSize ; break ;
  default:
    break ;
  }
  
  if ( SpecsSuccess != theError ) {
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return theError ;
  }    
  
  theError |= checkSlaveIt( theSlave ) ;
  
  idata = 0 ;
  nFrame = 0 ;
  
  nValuesCorrDouble = ( nValues  ) + 4 + 3 ;
  nWordsSpecs = (U32) std::ceil(nValuesCorrDouble/4.0 )+((nValues/(9*4))*2);
  // on recoit un header de 8 bytes tous les 9 mots
  theError |= specsUserFIFORead( specsMtr, specsData,nWordsSpecs ) ;
  
  while ( idata < nValues ) {
    if ( 0 == nFrame ) nMaxValues = 0x1A ;
    else nMaxValues = 0x1C ;
    
    if ( ( nValues - idata ) > nMaxValues ) {
      nWordsSpecs = 9 ;
      nToRead = nMaxValues ;
    } else {
      // nbre de mots de 8 bits que l on veut lire. 
      // comme le specs donne des mots de 32 bits et que le 1er est dummy
      // on a donc a lire [nValues + 4 (pour le 1er mot dummy) + 3 
      // (pour les 24 bits dummy du 2eme mot)]/4 
      nValuesCorrDouble = ( nValues - idata ) + 4 + 3 + 1 ;
      nWordsSpecs=(U32)std::ceil(nValuesCorrDouble/4.); 
      // number of 4-octet 
      // words (Specs) prend le 1er entier au dessus
      //
      // nWordsSpecs est le nombre de mots de 32 bits
      // nValues est le nbre de mots de 8 bits 
      //
      nToRead = ( nValues - idata ) ;
    }
    // 
    // le bazar sur la FIFO ..
    // theError |= specsUserFIFORead(specsMtr,specsData,nWordsSpecs); MTQ
    // read avant laboucle
    nFrame++ ;
    
    idata2 = 0 ;
    if ( SpecsSuccess == theError ) {
      if ( 1 == nFrame ) { 
	for (unsigned int iwordSpecs = 2 ;  iwordSpecs< nWordsSpecs ; 
	     iwordSpecs++) {
	  data[ idata ] = static_cast< U8 > 
	    ( ( specsData[iwordSpecs] & 0xFF000000 ) >> 24 ) ; 
	  idata++ ; idata2++ ; if ( idata2 >= nToRead ) break ;
	  data[idata] = 
	    static_cast< U8 > ( ( specsData[iwordSpecs] & 0xFF0000 ) 
				>> 16 ) ; 
	  idata++ ; idata2++ ; if ( idata2 >= nToRead ) break ;
	  data[idata] = static_cast< U8 > ( ( specsData[iwordSpecs] & 
					      0xFF00 ) >> 8 ) ; 
	  idata++ ; idata2++ ; if ( idata2 >= nToRead ) break ;
	  data[idata] = static_cast< U8 > ( specsData[iwordSpecs] & 
					    0xFF ) ; 
	  idata++ ; idata2++ ; if ( idata2 >= nToRead ) break ;
	}
      } else {
	data[ idata ] = static_cast< U8 > ( specsData[1] & 0xFF ) ;
	for (unsigned int iwordSpecs = 2 ;  iwordSpecs <= nWordsSpecs ; 
	     iwordSpecs++){
	  idata++ ; idata2++ ; if ( idata2 >= nToRead ) break ;
	  data[idata] = static_cast< U8 > 
	    ( ( specsData[iwordSpecs] & 0xFF000000 ) >> 24 ) ; 
	  idata++ ; idata2++ ; if ( idata2 >= nToRead ) break ;
	  data[idata] = 
	    static_cast< U8 > ( ( specsData[iwordSpecs] & 0xFF0000 ) 
				>> 16 ) ; 
	  idata++ ; idata2++ ; if ( idata2 >= nToRead ) break ;
	  data[idata] = static_cast< U8 > ( ( specsData[iwordSpecs] & 
					      0xFF00 ) >> 8 ) ; 
	  idata++ ; idata2++ ; if ( idata2 >= nToRead ) break ;
	  data[idata] = static_cast< U8 > ( specsData[iwordSpecs] & 
					    0xFF ) ; 
	}
      }
    } else break ;  
  }

  releaseMaster( theSlave -> pSpecsmaster ) ;
  return theError ; 
}
//========================================================================
//
//========================================================================
SpecsError specs_i2c_combinedread(  SPECSSLAVE * theSlave , 
                                    U8 outputSelect       ,
                                    U8 i2cAddress         ,
                                    U8 * i2cSubAdd        ,
                                    U8 subAddLength       ,
                                    U8 nValues            ,
                                    U8 * data                    ) {
  bool isFromRecursiveLock = true ;
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( theError != SpecsSuccess ) return theError ;
  
  RETURN_CODE errorCodeFromLowLevelSpecsLib = ApiSuccess ;

  if ( ! recursiveLock[ theSlave -> pSpecsmaster ] )
    isFromRecursiveLock = false ;
  
  theError = reserveMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  errorCodeFromLowLevelSpecsLib = 
    I2cEEPROMWrite(theSlave,outputSelect,i2cAddress,i2cSubAdd,
		   subAddLength);
  
  switch ( errorCodeFromLowLevelSpecsLib ) {
  case ApiSuccess:
    theError = SpecsSuccess ; break ;
  case ApiAccessDenied:
    theError = WriteAccessDenied ; break ;
  case ApiInvalidSize:
    theError = InvalidBufferSize ; break ;
  default:
    break ;
  }    
  
  if ( SpecsSuccess != theError ) {
    releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
    return theError ;
  }
  
  theError |= checkSlaveIt( theSlave ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  // Now do the normal READ
  theError = specs_i2c_read( theSlave , outputSelect , i2cAddress , 
			     nValues , data ) ;

  if ( ! isFromRecursiveLock )   recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  
  theError |= releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  
  return theError ; 
}

//========================================================================
// 
//========================================================================
SpecsError specs_parallel_write(  SPECSSLAVE * theSlave ,
                                  U8 address            ,
                                  U8 nValues            ,
                                  U16 * data              ) 
{
  /* Test of all the arguments before calling the relevant functions . */
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( theError != SpecsSuccess ) return theError ;

  if ( nValues > 127 ) return InvalidParameter ;

  RETURN_CODE theSpecsLibError ;
  if (data == NULL)
    {
      theError = InvalidParameter ;
    }
  else if (nValues <= 0) 
    {
      theError = InvalidParameter ;
    }
  else if (nValues == 1) 
    /* Select Parallel or DMA following the value of nValues */
    {
      theError = reserveMaster( theSlave -> pSpecsmaster ) ;
      if ( theError != SpecsSuccess ) return theError ;

      theSpecsLibError = ParallelWrite (theSlave, address, data[0] );
      theError |= checkSlaveIt( theSlave ) ;

      releaseMaster( theSlave -> pSpecsmaster ) ;
 
      switch ( theSpecsLibError ) {
      case ApiAccessDenied: 
	theError |= WriteAccessDenied ; break ;
      case ApiInvalidSize:
	theError |= InvalidBufferSize ; break ;
      default:
	break ;
      }

    }
  else
    {
      U8* pData;
      pData = (U8*)malloc (2*nValues*sizeof(U8));
      for (U8 i = 0 ; i < nValues; i++) {
        pData[2*i] = (U8) ( ( data[i] & 0xFF00 ) >> 8 ) ;
        pData[2*i+1] = (U8) ( data[i] & 0xFF ) ;
      }
      theError = reserveMaster( theSlave -> pSpecsmaster ) ;
      if ( theError != SpecsSuccess ) return theError ;

      theSpecsLibError = 
	ParallelDMAWrite (theSlave, address, pData, 2*nValues);  
      theError |= checkSlaveIt( theSlave ) ;

      releaseMaster( theSlave -> pSpecsmaster ) ;
      
      free(pData);
      
      switch ( theSpecsLibError ) {
      case ApiAccessDenied:
	theError |= WriteAccessDenied ; break ;
      case ApiInvalidSize:
	theError |= InvalidBufferSize ; break ;
      default:
	break ;
      }
    }
  
  return theError ;
}

//========================================================================
// 
//========================================================================
SpecsError specs_parallel_read ( SPECSSLAVE * theSlave ,
                                 U8 address            ,
                                 U8 nValues            ,
                                 U16 * data             ) 
{
  /* Test of all the arguments before calling the relevant functions ... */
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( theError != SpecsSuccess ) return theError ;
  
  if ( nValues > 128 ) return InvalidParameter ;
  
  RETURN_CODE theSpecsLibError ; 
  SPECSMASTER* specsMtr = theSlave->pSpecsmaster ; 

  theError = reserveMaster( specsMtr ) ;
  if ( theError != SpecsSuccess ) return theError ;

  if (nValues <= 0)
    {
      theError = InvalidParameter ;
    }
  else if (nValues == 1) 
    /* Select Parallel or DMA following the value of nValues */
    {
      theSpecsLibError = ParallelRead (theSlave, address, data);
      switch ( theSpecsLibError ) {
      case ApiAccessDenied:
	theError |= WriteAccessDenied ; break ;
      case ApiInvalidSize:
	theError |= InvalidBufferSize ; break ;
      default:
	break ;
      }        
    }
  else
    {
      U8 * pData = new U8[ 256 ] ;
      
      theSpecsLibError = 
	ParallelDMARead (theSlave, address, pData, 2*nValues);  
      switch ( theSpecsLibError ) {
      case ApiAccessDenied:
	theError |= WriteAccessDenied ; break ;
      case ApiInvalidSize:
	theError |= InvalidBufferSize ; break ;
      default:
	break ;
      }
      delete [] pData ;
    }
  
  if ( SpecsSuccess == theError)
    { 
      theError |= checkSlaveIt( theSlave ) ;
      /* Use of FIFO for reading the values */
      /* be careful : 32 bits word in FIFO, we want 16 bits word */
      U32 specsData[256];
      U32 nSpecsWords = ( nValues * 2 + 3 + 4 ) / 4 + 1 ;
      theError |= specsUserFIFORead(specsMtr,specsData,nSpecsWords); 
      
      if (!theError)
	{  
	  /* We have passed a correct header */ 
	  /* We get the values */
	  unsigned int j = 2 ;
	  for (unsigned int i = 0 ; i < nValues ; i++)
	    {
	      if ( ( i % 2 ) == 0 ) {
		data [i] = 
		  static_cast< U16 > ( ( ( specsData[j-1] & 0xFF ) << 8 ) |
				       ( ( specsData[j] & 0xFF000000 ) 
					 >> 24 ) ) ;
	      } else {
		data [i] = 
		  static_cast< U16 > ( ( specsData[j] & 0xFFFF00 ) >> 8 ) ;
		j++ ;
	      } 
	    }
	}     
    }

  releaseMaster( theSlave -> pSpecsmaster ) ;
  
  return theError ;
}

//========================================================================
//
//========================================================================
SpecsError specs_jtag_reset(  SPECSSLAVE * theSlave , 
                              U8 outputSelect)
{
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( SpecsSuccess != theError ) return theError ;
  U8 tms;

  tms = 0xF8;
  RETURN_CODE specsLibError ;

  theError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  specsLibError =
    JtagWriteRead(theSlave, outputSelect, &tms, 0, 5, MOVE, 0, 0);
  
  if ( ApiInvalidSize == specsLibError ) theError |= InvalidBufferSize ;
  else if ( ApiAccessDenied == specsLibError ) 
    theError |= WriteAccessDenied ;
  else if ( ApiWaitTimeout == specsLibError ) 
    theError |= ReadAccessDenied ;
  else if ( ApiInvalidData == specsLibError ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_master_softreset( theSlave -> pSpecsmaster ) ;
    theError |= ChecksumError ;
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  }
  else {
    theError |= checkSlaveIt( theSlave ) ;
    SPECSMASTER * specsMtr = theSlave -> pSpecsmaster ;
    U32 specsData[ 256 ] ;
    theError = specsUserFIFORead( specsMtr , specsData , 2 ) ;
  }

  releaseMaster( theSlave -> pSpecsmaster ) ;
  
  return theError ;
}

//========================================================================
//
//========================================================================
SpecsError specs_jtag_idle(  SPECSSLAVE * theSlave , 
                             U8 outputSelect)
{
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( SpecsSuccess != theError ) return theError ;
  U8 tms;
  
  tms = 0x00;
  RETURN_CODE specsLibError ;

  theError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  specsLibError =
    JtagWriteRead(theSlave, outputSelect, &tms, 0, 5, MOVE, 0, 0); 
  
  if ( ApiInvalidSize == specsLibError ) theError |= InvalidBufferSize ;
  else if ( ApiAccessDenied == specsLibError ) 
    theError |= WriteAccessDenied ;
  else if ( ApiWaitTimeout == specsLibError ) 
    theError |= ReadAccessDenied ;
  else if ( ApiInvalidData == specsLibError ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_master_softreset( theSlave -> pSpecsmaster ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    theError |= ChecksumError ;
  }
  else {
    theError |= checkSlaveIt( theSlave ) ;
    SPECSMASTER * specsMtr = theSlave -> pSpecsmaster ;
    U32 specsData[ 256 ] ;
    theError |= specsUserFIFORead( specsMtr , specsData , 2 ) ;
  }

  releaseMaster( theSlave -> pSpecsmaster ) ;
  
  return theError ;
}

//========================================================================
//
//========================================================================
SpecsError specs_jtag_irscan(  SPECSSLAVE * theSlave    , 
                               U8 outputSelect	,
                               U8 *dataIn	,
                               U8 *dataOut	,
                               U32 nBits	)
{
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( SpecsSuccess != theError ) return theError ;
  return JtagWriteReadMany(theSlave, outputSelect, dataIn, dataOut, nBits, 
                           IRSCAN, 1, 1); 
}

//========================================================================
//
//========================================================================
SpecsError specs_jtag_drscan(  SPECSSLAVE * theSlave , 
                               U8 outputSelect ,
                               U8 *dataIn      ,
                               U8 *dataOut     ,
                               U32 nBits       )
{
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( SpecsSuccess != theError ) return theError ;
  return JtagWriteReadMany(theSlave, outputSelect, dataIn, dataOut, 
                           nBits, DRSCAN, 1, 1); 
}

//========================================================================
//
//========================================================================
SpecsError specs_register_write( SPECSSLAVE * theSlave ,
                                 U8 theRegister        ,
                                 U16 data               ) {
  
  /* Test of all the arguments before calling the relevant functions... */
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( theError != SpecsSuccess ) return theError ;
 
  RETURN_CODE errorCodeFromLowLevelSpecsLib ;

  theError = reserveMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;
 
  errorCodeFromLowLevelSpecsLib = RegisterWrite(theSlave, theRegister,
						data);					 
  theError |= checkSlaveIt( theSlave ) ;
  
  
  switch ( errorCodeFromLowLevelSpecsLib ) {
  case ApiAccessDenied:
    theError |= WriteAccessDenied ; break ;
  case ApiInvalidSize:
    theError |= InvalidBufferSize ; break ;
  default:
    break ;
  }

  theError |= releaseMaster_checkRecursive( theSlave -> pSpecsmaster );
  
  return theError ; 
}

//=============================================================================
//
//=============================================================================
SpecsError specs_register_read( SPECSSLAVE * theSlave ,
                                U8 theRegister        ,
                                U16 * data                   ) {
  /* Test of all the arguments before calling the relevant functions ... */
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( theError != SpecsSuccess ) return theError ;

  SPECSMASTER* specsMtr = theSlave->pSpecsmaster ;
  U32 specsData[5] ;
  
  for ( int ii = 0 ; ii < 5 ; ++ii ) specsData[ ii ] = 0 ;

  U32 size = 3;

  RETURN_CODE errorCodeFromLowLevelSpecsLib ;

  theError = reserveMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  errorCodeFromLowLevelSpecsLib = RegisterRead(theSlave,theRegister,data);	
  theError |= checkSlaveIt( theSlave ) ;
  switch ( errorCodeFromLowLevelSpecsLib ) {
  case ApiSuccess:
    theError |= SpecsSuccess ; break ;
  case ApiAccessDenied:
    theError |= WriteAccessDenied ; break ;
  case ApiInvalidSize:
    theError |= InvalidBufferSize ; break ;
  default:
    break ;
  }
  
  // le bazar sur la FIFO ..
  theError |= specsUserFIFORead(specsMtr,specsData,size);

  theError |= releaseMaster_checkRecursive( specsMtr ) ;

  if ( SpecsSuccess == theError ) {
    U8 data8Bits[2] ;
    data8Bits[0] = static_cast< U8 > ( specsData[1] & 0xFF ) ; 
    data8Bits[1] = static_cast< U8 > ( ( specsData[2] & 0xFF000000 ) 
				       >> 24 ) ; 
    // on colle les 2 morceaux de 8 bits 
    // (apres avoir decale la partie haute de 8 bits
    U16 highPart = data8Bits[0]<<8 ; 
    
    *data = highPart |  data8Bits[1] ;
  }
  
  return theError ; 
}

//=============================================================================
//
//=============================================================================
SpecsError specs_slave_internal_reset(  SPECSSLAVE * theSlave ) {
  U16 resetOn = GlobalSoftwareResetBit ;
  U16 resetOff = 0x0 ;
  if ( 0 == theSlave ) return InvalidParameter ;

  SpecsError theError ;  
  theError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = 
    specs_register_write( theSlave , MezzaCtrlReg , resetOn ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( theError != SpecsSuccess ) {
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return theError ;
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = 
    specs_register_write( theSlave , MezzaCtrlReg , resetOff ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  theError |= releaseMaster( theSlave -> pSpecsmaster ) ;
  
  return theError ;
}

//=============================================================================
//
//=============================================================================
SpecsError specs_slave_external_reset(  SPECSSLAVE * theSlave ) {
  U16 resetOn = OutputExternalResetBit ;
  U16 resetOff = 0x0 ;
  if ( 0 == theSlave ) return InvalidParameter ;
  SpecsError theError ;

  theError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = 
    specs_register_write( theSlave , MezzaCtrlReg , resetOn ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( theError != SpecsSuccess ) {
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return theError ;
  }
  
  usleep( 1000 ) ;

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError =  
    specs_register_write( theSlave , MezzaCtrlReg , resetOff ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  theError |= releaseMaster( theSlave -> pSpecsmaster ) ;
  
  return theError ;
}

//=============================================================================
//
//=============================================================================
SpecsError specs_slave_external_shortreset(  SPECSSLAVE * theSlave ) {
  U16 resetOn = OutputExternalResetBit ;
  U16 resetOff = 0x0 ;
  if ( 0 == theSlave ) return InvalidParameter ;
  SpecsError theError ;

  theError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = 
    specs_register_write( theSlave , MezzaCtrlReg , resetOn ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( theError != SpecsSuccess ) {
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return theError ;
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError =  
    specs_register_write( theSlave , MezzaCtrlReg , resetOff ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  theError |= releaseMaster( theSlave -> pSpecsmaster ) ;
  
  return theError ;
}

//=============================================================================
//
//=============================================================================
SpecsError specs_master_reset(  SPECSMASTER * theMaster ) {
  if ( 0 == theMaster ) return InvalidParameter ;

  bool semExists1(true) , semExists2(true) , semExists3(true) , 
    semExists4(true) ;
  bool semExists5(true) , semExists6(true) , semExists7(true) , 
    semExists8(true) ;
  bool semExists9(true) , semExists10(true) , semExists11(true) , 
    semExists12(true) ;
  bool semExists13(true) , semExists14(true) , semExists15(true) , 
    semExists16(true) ;
  int semId1(0), semId2(0), semId3(0), semId4(0) ;
  int semId5(0), semId6(0), semId7(0), semId8(0) ;
  int semId12(0), semId11(0), semId10(0), semId9(0) ;
  int semId13(0), semId14(0), semId15(0), semId16(0) ;
  std::map< SPECSMASTER * , int > dummyMap ;
  // Open or create all semaphores:
  specsCreateSemaphore( &semExists1 , &semId1 , 
                        serialIdMap[ theMaster ] , 0 ,
                        3 , &dummyMap , theMaster ) ;
  specsCreateSemaphore( &semExists2 , &semId2 , 
                        serialIdMap[ theMaster ] , 1 ,
                        3 , &dummyMap , theMaster ) ;
  specsCreateSemaphore( &semExists3 , &semId3 , 
                        serialIdMap[ theMaster ] , 2 ,
                        3 , &dummyMap , theMaster ) ;
  specsCreateSemaphore( &semExists4 , &semId4 , 
                        serialIdMap[ theMaster ] , 3 ,
                        3 , &dummyMap , theMaster ) ;  
  specsCreateSemaphore( &semExists5 , &semId5 , 
                        serialIdMap[ theMaster ] , 4 ,
                        3 , &dummyMap , theMaster ) ;
  specsCreateSemaphore( &semExists6 , &semId6 , 
                        serialIdMap[ theMaster ] , 5 ,
                        3 , &dummyMap , theMaster ) ;
  specsCreateSemaphore( &semExists7 , &semId7 , 
                        serialIdMap[ theMaster ] , 6 ,
                        3 , &dummyMap , theMaster ) ;
  specsCreateSemaphore( &semExists8 , &semId8 , 
                        serialIdMap[ theMaster ] , 7 ,
                        3 , &dummyMap , theMaster ) ;  
  specsCreateSemaphore( &semExists9 , &semId9 , 
                        serialIdMap[ theMaster ] , 8 ,
                        3 , &dummyMap , theMaster ) ;
  specsCreateSemaphore( &semExists10 , &semId10 , 
                        serialIdMap[ theMaster ] , 9 ,
                        3 , &dummyMap , theMaster ) ;
  specsCreateSemaphore( &semExists11 , &semId11 , 
                        serialIdMap[ theMaster ] , 10 ,
                        3 , &dummyMap , theMaster ) ;
  specsCreateSemaphore( &semExists12 , &semId12 , 
                        serialIdMap[ theMaster ] , 11 ,
                        3 , &dummyMap , theMaster ) ;  
  specsCreateSemaphore( &semExists13 , &semId13 , 
                        serialIdMap[ theMaster ] , 12 ,
                        3 , &dummyMap , theMaster ) ;
  specsCreateSemaphore( &semExists14 , &semId14 , 
                        serialIdMap[ theMaster ] , 13 ,
                        3 , &dummyMap , theMaster ) ;
  specsCreateSemaphore( &semExists15 , &semId15 , 
                        serialIdMap[ theMaster ] , 14 ,
                        3 , &dummyMap , theMaster ) ;
  specsCreateSemaphore( &semExists16 , &semId16 , 
                        serialIdMap[ theMaster ] , 15 ,
                        3 , &dummyMap , theMaster ) ;  

  union semun {
    int val ;
    struct semid_ds *buf ;
    unsigned short *array ;
  } arg ;
  arg.val = 1 ;

  if ( ! semExists1 ) 
    semctl( semId1 , 0 , SETVAL , arg ) ;
  if ( ! semExists2 ) 
    semctl( semId2 , 0 , SETVAL , arg ) ;
  if ( ! semExists3 ) 
    semctl( semId3 , 0 , SETVAL , arg ) ;
  if ( ! semExists4 ) 
    semctl( semId4 , 0 , SETVAL , arg ) ;
  if ( ! semExists5 ) 
    semctl( semId5 , 0 , SETVAL , arg ) ;
  if ( ! semExists6 ) 
    semctl( semId6 , 0 , SETVAL , arg ) ;
  if ( ! semExists7 ) 
    semctl( semId7 , 0 , SETVAL , arg ) ;
  if ( ! semExists8 ) 
    semctl( semId8 , 0 , SETVAL , arg ) ;
  if ( ! semExists9 ) 
    semctl( semId9 , 0 , SETVAL , arg ) ;
  if ( ! semExists10 ) 
    semctl( semId10 , 0 , SETVAL , arg ) ;
  if ( ! semExists11 ) 
    semctl( semId11 , 0 , SETVAL , arg ) ;
  if ( ! semExists12 ) 
    semctl( semId12 , 0 , SETVAL , arg ) ;
  if ( ! semExists13 ) 
    semctl( semId13 , 0 , SETVAL , arg ) ;
  if ( ! semExists14 ) 
    semctl( semId14 , 0 , SETVAL , arg ) ;
  if ( ! semExists15 ) 
    semctl( semId15 , 0 , SETVAL , arg ) ;
  if ( ! semExists16 ) 
    semctl( semId16 , 0 , SETVAL , arg ) ;

  // now lock all masters
  int res ;
  struct sembuf operation ;
  operation.sem_num = 0 ;
  operation.sem_op  = -1 ;
  operation.sem_flg = SEM_UNDO ;
  res = semtimedop( semId1 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 1") ; return SpecsMasterLocked ; }
  res = semtimedop( semId2 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 2") ; return SpecsMasterLocked ; }
  res = semtimedop( semId3 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 3") ; return SpecsMasterLocked ; }
  res = semtimedop( semId4 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 4") ; return SpecsMasterLocked ; }
  res = semtimedop( semId5 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 5") ; return SpecsMasterLocked ; }
  res = semtimedop( semId6 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 6") ; return SpecsMasterLocked ; }
  res = semtimedop( semId7 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 7") ; return SpecsMasterLocked ; }
  res = semtimedop( semId8 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 8") ; return SpecsMasterLocked ; }
  res = semtimedop( semId9 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 9") ; return SpecsMasterLocked ; }
  res = semtimedop( semId10 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 10") ; return SpecsMasterLocked ; }
  res = semtimedop( semId11 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 11") ; return SpecsMasterLocked ; }
  res = semtimedop( semId12 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 12") ; return SpecsMasterLocked ; }
  res = semtimedop( semId13 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 13") ; return SpecsMasterLocked ; }
  res = semtimedop( semId14 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 14") ; return SpecsMasterLocked ; }
  res = semtimedop( semId15 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 15") ; return SpecsMasterLocked ; }
  res = semtimedop( semId16 , &operation , 1 , &LockDelay ) ;
  if ( res < 0 ) { perror("master reset 16") ; return SpecsMasterLocked ; }
  
  SpecsmasterReset( theMaster ) ;

  operation.sem_op = 1 ;
  semop( semId1 , &operation , 1 ) ;
  semop( semId2 , &operation , 1 ) ;
  semop( semId3 , &operation , 1 ) ;
  semop( semId4 , &operation , 1 ) ;
  semop( semId5 , &operation , 1 ) ;
  semop( semId6 , &operation , 1 ) ;
  semop( semId7 , &operation , 1 ) ;
  semop( semId8 , &operation , 1 ) ;
  semop( semId9 , &operation , 1 ) ;
  semop( semId10 , &operation , 1 ) ;
  semop( semId11 , &operation , 1 ) ;
  semop( semId12 , &operation , 1 ) ;
  semop( semId13 , &operation , 1 ) ;
  semop( semId14 , &operation , 1 ) ;
  semop( semId15 , &operation , 1 ) ;
  semop( semId16 , &operation , 1 ) ;

  if ( ! semExists1 ) semctl( semId1 , 0 , IPC_RMID ) ;
  if ( ! semExists2 ) semctl( semId2 , 0 , IPC_RMID ) ;
  if ( ! semExists3 ) semctl( semId3 , 0 , IPC_RMID ) ;
  if ( ! semExists4 ) semctl( semId4 , 0 , IPC_RMID ) ;  
  if ( ! semExists5 ) semctl( semId5 , 0 , IPC_RMID ) ;
  if ( ! semExists6 ) semctl( semId6 , 0 , IPC_RMID ) ;
  if ( ! semExists7 ) semctl( semId7 , 0 , IPC_RMID ) ;
  if ( ! semExists8 ) semctl( semId8 , 0 , IPC_RMID ) ;  
  if ( ! semExists9 ) semctl( semId9 , 0 , IPC_RMID ) ;
  if ( ! semExists10 ) semctl( semId10 , 0 , IPC_RMID ) ;
  if ( ! semExists11 ) semctl( semId11 , 0 , IPC_RMID ) ;
  if ( ! semExists12 ) semctl( semId12 , 0 , IPC_RMID ) ;  
  if ( ! semExists13 ) semctl( semId13 , 0 , IPC_RMID ) ;
  if ( ! semExists14 ) semctl( semId14 , 0 , IPC_RMID ) ;
  if ( ! semExists15 ) semctl( semId15 , 0 , IPC_RMID ) ;
  if ( ! semExists16 ) semctl( semId16 , 0 , IPC_RMID ) ;  
  
  // Possibilite de probleme quand on fait un reset pendant un CTRL+C
  return SpecsSuccess ;
}

//=============================================================================
//
//=============================================================================
SpecsError specs_master_softreset( SPECSMASTER * theMaster ) {
  if ( 0 == theMaster ) return InvalidParameter ;
  bool isFromRecursiveLock = true ;
  
  SpecsError theError = SpecsSuccess ;
  int kk ;
  U32 data[2];
  U8 initialSpeed , isInitialMasked ;
  U32 n ;
  int j = 0 ;

  U32 status ;
  unsigned short address ; 
  //return theError ;

  for (j=0 ;j < 256 ;j++)
    {
      theError= SpecsmasterReceiverFIFORead(theMaster, data, 1);
      if ( theError == ApiInvalidSize) 
	{
	  break;
	}
    }

  return SpecsSuccess ;
  //MTQ il y a des soft reset tres souvant

  theError = reserveMaster( theMaster ) ;
  if ( theError != SpecsSuccess ) return theError ;
  
  initialSpeed = getSpeed( theMaster ) ;
  isInitialMasked = isChecksumMasked( theMaster ) ;
  SpecsPlxStatusRead( theMaster );
  //    n = (*(theMaster -> pSpecsmasterStatus) >> 8 ) & 0xFF ;
  n = ( SpecsPlxStatusRead( theMaster ) >> 8 ) & 0xFF ;
  
  while ( n != 0 ) {
    if ( j++ >= 1000000 ) {
      // Reset this master !
      SpecsmasterCtrlWrite( theMaster , ResetProgBit ) ;
      SpecsmasterCtrlWrite( theMaster , 0 ) ;    
      return WriteAccessDenied ;
    }
    //      n = (*(theMaster -> pSpecsmasterStatus) >> 8 ) & 0xFF ;
    n = ( SpecsPlxStatusRead( theMaster ) >> 8 ) & 0xFF ;  
  }

  for ( kk = 0 ; kk < 30000 ; ++kk ) {
    n=n+1 ;
  }
  // Send pending User and I/O interrupts PERHAPS TO DO ?
  
  status = SpecsPlxStatusRead( theMaster ) ;
  address = static_cast< unsigned short >( ( status & 0x1F0000 ) >> 16 ) ;
  
  SpecsmasterCtrlWrite( theMaster , ResetProgBit ) ;
  SpecsmasterCtrlWrite( theMaster , 0 ) ;
  
  specs_master_setspeed( theMaster , initialSpeed ) ;
 
  if ( 0 != isInitialMasked ) {
    specs_master_maskchecksum( theMaster ) ;
  }
 
  return theError  ;
}

//=============================================================================
//
//=============================================================================
SpecsError specs_slave_open(  SPECSMASTER * theMaster ,
                              U8 slaveAddress         ,
                              SPECSSLAVE * theSlave          ) {
  if ( ( 0 == theMaster ) || ( slaveAddress > 64 ) || ( 0 == theSlave ) )
    return InvalidParameter ;
  theSlave -> pSpecsmaster = theMaster ;
  theSlave -> SpecsslaveAdd = slaveAddress ;
  return SpecsSuccess ; 
}

//=============================================================================
//
//=============================================================================
U8 specs_master_card_select(DEVICE_INVENT * deviceList) 
{
  DEVICE_LOCATION  * pDevice;
  U32 i;
  U32 DeviceNum=0;
  
  pDevice = &deviceList->StDevLoc;
  
  pDevice->BusNumber       = (U8) -1;
  pDevice->SlotNumber      = (U8) -1;
  pDevice->VendorId        = (U16) -1;
  pDevice->DeviceId        = 0x0005;
  pDevice->SerialNumber[0] = '\0';
  
  
  if (PlxPciDeviceFind(
                       pDevice,
                       &DeviceNum)
      !=ApiSuccess)
    {
      return 0;
    }
  
  if (DeviceNum == 0)
    {
      return 0;
    }

  // Est-ce que les cartes sont toujours dans le meme ordre ?
  
  for (i=0 ; i<DeviceNum ; i++)
    {
      pDevice = &(&deviceList[i])->StDevLoc;
      pDevice->BusNumber       = (U8) -1;
      pDevice->SlotNumber      = (U8) -1;
      pDevice->VendorId        = (U16) -1;
      pDevice->DeviceId        = 0x0005;
      pDevice->SerialNumber[0] = '\0';
      (&deviceList[i])->Index = i;
      
      PlxPciDeviceFind(
                       pDevice,
                       &i);
    }
  
  return (U8)DeviceNum;
}

//=============================================================================
//
//=============================================================================
SpecsError specs_master_open( DEVICE_INVENT theDevice ,
                              U8 MasterId ,
                              SPECSMASTER * theMasterCard ) {
  HANDLE      PlxHandle;
  RETURN_CODE rc;
  int i;
  int err=0;
  MasterId= MasterId-1;
  // Verifie que le master id est compris entre 1 et 4
  if ( ( MasterId < 0 ) || ( MasterId >= NB_MASTER ) ) return WrongDeviceInfo ;
 
  PlxHandle = EtatDev[MasterId];
 
  // MTQ int serId ;
  // Le driver ouvre le device Plx s'il n'a pas deja ete ouvert
  if ( PlxHandle  == (HANDLE)-1) {
  
    rc = PlxPciDeviceOpen(&theDevice.StDevLoc,&PlxHandle,MasterId);
    if (rc != ApiSuccess) {
      if ( ApiNullParam == rc ) return InvalidParameter ;
      else if ( ApiInvalidDeviceInfo == rc ) return WrongDeviceInfo ;
      else if ( ApiNoActiveDriver == rc ) return NoPlxDriver ;
      else if ( ApiInvalidDriverVersion == rc ) return NoPlxDriver ;
    }

    // pour faire un reset des mastesr si c'est le premier master qui est ouvert
    for (i=0 ; i< NB_MASTER ;i++)
      if ( EtatDev[i] !=  -1 ) err = 1;
    if ( ! err) 
      {
	PlxPciBoardReset(PlxHandle,MasterId);
      }
    EtatDev[MasterId] = PlxHandle;
    EtatDevCount[MasterId] += 1;

    SpecsmasterInit(theMasterCard,MasterId,PlxHandle) ;   

    // MTQ il n'y a pas de serial number serId = (int) board_serialNumber( &PlxHandle ) ;
  } else {
    EtatDevCount[MasterId] += 1;
    SpecsmasterInit(theMasterCard,MasterId,PlxHandle) ;

    // MTQ il n'y a pas de serial number  serId = (int) board_serialNumber( &PlxHandle ) ;
  }

  std::ostringstream outputStream ;
  
  // Add interrupt event maps
  PLX_NOTIFY_OBJECT Event ;


  // Store Serial ID
  outputStream.str( "" ) ;
  outputStream << "/dev/plx/" << theDevice.StDevLoc.SerialNumber ;
  serialIdMap[ theMasterCard ] = outputStream.str() ;

  rc = PlxNotificationRegisterFor( theMasterCard -> hdle , 
				   &IntSources ,MasterId);
  //MTQ                                  &IntSources , &Event ) ;
 
  if ( rc != ApiSuccess ) return WrongDeviceInfo ;
  std::pair< intEventMapIterator , bool > resEv = 
    intEventMap.insert( std::make_pair( theMasterCard , Event ) ) ;
  if ( ! resEv.second ) return WrongDeviceInfo ;
  
  // Fill Map with IDs
  std::pair< idMapIterator , bool > resId = 
    idMap.insert( std::make_pair( theMasterCard , 
                                  (unsigned short) MasterId ) ) ;
  if ( ! resId.second ) return WrongDeviceInfo ;


  bool semExists = true ;
  int semId = 0 ;
  outputStream.str( "" ) ;
  outputStream << "/dev/plx/" << theDevice.StDevLoc.SerialNumber ;

  if ( ! specsCreateSemaphore( &semExists , &semId , outputStream.str() , 
                               MasterId , 3 , &mutexMap , theMasterCard ) ) 

    return WrongDeviceInfo ;

  // initialize recursive lock variable
  recursiveLock[ theMasterCard ] = false ;  

  // raz of semaphore if needed
  int semValue = semctl( semId , 0 , GETVAL ) ;

  if ( 1 != semValue ) {
    int res = 0 ;
    struct sembuf operation ;
    operation.sem_num = 0 ;
    operation.sem_op  = -1 ;
    operation.sem_flg = SEM_UNDO ;

    if ( semExists ) {
      // try to obtain semaphore
      res = semtimedop( semId , &operation , 1 , &LockDelay ) ;
      if ( res < 0 ) printf( "Reset lost semaphore !\n" ) ;
    }
    
   if ( ( res < 0 ) || ( ! semExists ) ) { 
      // Timeout or semaphore does not exist  
      union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
      } arg;
      arg.val = 1 ;
      int retCode = semctl( semId , 0 , SETVAL , arg ) ;
      if ( retCode < 0 ) { perror("") ; return WrongDeviceInfo ; }
    } else if ( 0 == res ) {
      // release semaphore and set it to 0
      union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
      } arg;
      arg.val = 0 ;
      int retCode = semctl( semId , 0 , SETVAL , arg ) ;
      if ( retCode < 0 ) { perror("") ; return WrongDeviceInfo ; }     
      operation.sem_op = 1 ;
      res = semop( semId , &operation , 1 ) ;
    }
  }
  
  // RAZ of 3rd semaphore if new
  if ( ! semExists ) {
    union semun {
      int val;
      struct semid_ds *buf;
      unsigned short *array;
    } arg;
    arg.val = 0 ;
    int retCode = semctl( semId , 2 , SETVAL , arg ) ;
    if ( retCode < 0 ) { perror("") ; return WrongDeviceInfo ; }    
  }

  // increase semaphore count:
  struct sembuf operation ;
  operation.sem_num = 2 ;
  operation.sem_op  = 1 ;
  operation.sem_flg = SEM_UNDO ;
  semop( semId , &operation , 1 ) ;
  
  return specs_master_softreset( theMasterCard ) ;
}

//=============================================================================
//
//=============================================================================
SpecsError specs_master_close( SPECSMASTER * theMasterCard )
{
  RETURN_CODE rc;

  HANDLE PlxHdle = theMasterCard->hdle;

  // decrease semaphore count:
  struct sembuf operation ;
  operation.sem_num = 2 ;
  operation.sem_op  = -1 ;
  operation.sem_flg = SEM_UNDO ;
  semtimedop( mutexMap[ theMasterCard ] , &operation , 1 , &LockDelay ) ;   
  // Get value 
  int nOpenedMaster = semctl( mutexMap[ theMasterCard ] , 2 , GETVAL ) ;
  if ( 0 == nOpenedMaster ) 
    semctl( mutexMap[ theMasterCard ] , 0 , IPC_RMID ) ;
  
  recursiveLock[ theMasterCard ] = false ;
 
  if (EtatDev[ theMasterCard->masterID] != PlxHdle) return InvalidParameter;
  if ( EtatDev[ theMasterCard->masterID] == -1) return InvalidParameter;

  mutexMap.erase( theMasterCard ) ;
  intEventMap.erase( theMasterCard ) ;
  
  serialIdMap.erase( theMasterCard ) ;
  idMap.erase( theMasterCard ) ;

  // MTQ il n'y a pas de serial number int serId = specs_master_serialNumber( theMasterCard ) ;
  
  rc = SpecsmasterEnd(theMasterCard);
 
  if (rc != ApiSuccess) return InvalidParameter;  

  //MTQ PlxNotificationCancel(PlxHdle,&intEventMap[theMasterCard]); a revoir s'il faut faire un close l'un apres l'autre
  PlxNotificationCancel(PlxHdle,0);
  memset(&intEventMap[theMasterCard],0,sizeof(PLX_NOTIFY_OBJECT));
  intEventMap.erase( theMasterCard ) ;

  // Ferme le Plx Device. S'il y a encore des masters ouverts
  // a ce moment-la, ca ne fait rien.
  // MTQ on ferme le handle du master 
 
  rc = PlxPciDeviceClose(PlxHdle, theMasterCard->masterID);
  if ( rc == ApiSuccess)
    {  printf("close %d in master_close_\n", theMasterCard->masterID);
      EtatDev[ theMasterCard->masterID] = -1;
    }
  if (rc != ApiSuccess)
    return InvalidParameter;

  /*
    for (unsigned int i=0 ; i<MAX_CARD ; i++)
    {
    if (EtatDev[i] == PlxHdle)
    {
    printf(" Close  EtatDevCount %d EtatDev[i] %d \n", EtatDevCount[i],EtatDev[i]);
    EtatDevCount[i] -= 1;
    if (!EtatDevCount[i])
    {
    SpecsmasterNotifyClear(PlxHdle,i);
    //MTQ ??    PlxNotificationCancel(PlxHdle,&InterTab[i]);
    PlxNotificationCancel(PlxHdle,0);
    memset(&InterTab[i],0,sizeof(PLX_NOTIFY_OBJECT));
    SpecsmasterNotifyClear(PlxHdle,i);
    MTQ on fait avant le close du master utilise
    rc = PlxPciDeviceClose(PlxHdle, theMasterCard->masterID);
    if (rc != ApiSuccess)
    return InvalidParameter;
	      
    EtatDev[i] = (HANDLE)-1;
    }
    return SpecsSuccess;
    }
    else
    continue;
    }
  */	    
  return SpecsSuccess ;
}

//=============================================================================
//
//=============================================================================
SpecsError specs_dcu_register_write(  SPECSSLAVE * theSlave ,
                                      U8 registerAddress    ,
                                      U8 data                ) {
  bool fromRecursiveLock = true ;
  if ( ( 0 == theSlave ) || ( registerAddress > 7 ) ) return InvalidParameter ;
  U8 address = ( registerAddress ) | ( DcuAddress << 3 ) ;

  U8 initialSpeed , isInitialMasked ;
  SpecsError theError = SpecsSuccess ;

  if ( ! recursiveLock[ theSlave -> pSpecsmaster ] ) fromRecursiveLock = false ;
  theError = reserveMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;
  
  initialSpeed = getSpeed( theSlave -> pSpecsmaster ) ;
  isInitialMasked = isChecksumMasked( theSlave -> pSpecsmaster ) ;
  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    theError = specs_master_setspeed( theSlave -> pSpecsmaster , DCUSpeed ) ;
    if ( ! fromRecursiveLock )   recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      theError |= specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      if ( ! fromRecursiveLock)   recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
    
    if ( theError != SpecsSuccess ) {
      releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
      return theError ;
    }
  }
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = specs_i2c_write( theSlave , DcuOutputSelect , address , 1 , 
			      &data ) ;
  if ( ! fromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
    if ( ! fromRecursiveLock )   recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    
    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      if ( ! fromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
  }

  theError |= releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  
  return theError ;
}

//=============================================================================
//
//=============================================================================
SpecsError specs_dcu_register_read(  SPECSSLAVE * theSlave ,
                                     U8 registerAddress    ,
                                     U8 * data              ) {
  bool isFromRecursiveLock = true ;
  if ( ( 0 == theSlave ) || ( registerAddress > 7 ) ) return InvalidParameter ;
  U8 address = ( registerAddress ) | ( DcuAddress << 3 ) ;

  U8 initialSpeed , isInitialMasked ;
  SpecsError theError = SpecsSuccess ;
  
  theError = reserveMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  if ( !   recursiveLock[ theSlave -> pSpecsmaster ] )
    isFromRecursiveLock = false ;
  
  initialSpeed = getSpeed( theSlave -> pSpecsmaster ) ;
  isInitialMasked = isChecksumMasked( theSlave -> pSpecsmaster ) ;
  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    theError = specs_master_setspeed( theSlave -> pSpecsmaster , DCUSpeed ) ;
    if ( ! isFromRecursiveLock )   recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    
    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      theError |= specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      if ( ! isFromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
    if ( theError != SpecsSuccess ) {
      releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
      return theError ;
    }
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = specs_i2c_read( theSlave , DcuOutputSelect , address , 1 , 
			     data ) ;
  if ( ! isFromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
    if( ! isFromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;

    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      if ( ! isFromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
  }
  theError |= releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  return theError ;
}

//========================================================================
//
//========================================================================
SpecsError specs_dcu_acquire(  SPECSSLAVE * theSlave  ,
                               U8 inputChannel        ,
                               U16 * data             ,
                               double * convertedValue ) {
  if ( ( 0 == theSlave ) || ( inputChannel > 7 ) ) 
    return InvalidParameter ;
  // select input channel in CREG
  U8 retValue ;
  SpecsError readError ;
  U8 mode ;
  bool getResult ;
  int nIter ;
  U8 initialSpeed , isInitialMasked ;

  readError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( readError != SpecsSuccess ) return readError ;

  initialSpeed = getSpeed( theSlave -> pSpecsmaster ) ;
  isInitialMasked = isChecksumMasked( theSlave -> pSpecsmaster ) ;
  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    readError = specs_master_setspeed( theSlave -> pSpecsmaster , 
				       DCUSpeed ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    
    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      readError |= specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
    
    if ( readError != SpecsSuccess ) {
      releaseMaster( theSlave -> pSpecsmaster ) ;
      return readError ;
    }
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_read( theSlave , DcuCREG , &retValue ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( SpecsSuccess != readError ) {
    if ( initialSpeed != DCUSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;

      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return readError ;
  }
  
  retValue = ( retValue & 0xC8 ) | ( inputChannel & 0x07 ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_write( theSlave , DcuCREG , retValue ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError |= specs_dcu_register_write( theSlave , DcuTREG , 0x10 ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError |= specs_dcu_register_write( theSlave , DcuAREG , 0x00 ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( SpecsSuccess != readError ) {
    if ( initialSpeed != DCUSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;

      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return readError ;
  }    
  
  // start acquisition ( write 0x80 in CREG)
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_read( theSlave , DcuCREG , &retValue ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  
  if ( SpecsSuccess != readError ) {
    if ( initialSpeed != DCUSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;

      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return readError ;
  }    

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_write( theSlave , DcuCREG , 
					( retValue & 0xF) | 0x80 ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( SpecsSuccess != readError ) {
    if ( initialSpeed != DCUSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;

      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return readError ;
  }    
  
  // Wait for 0x80 in SHREG
  nIter = 0 ;
  getResult = false ;
  while ( ! getResult ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    readError = specs_dcu_register_read( theSlave , DcuSHREG , &retValue ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;

    if ( SpecsSuccess != readError ) {
      if ( initialSpeed != DCUSpeed ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
	
	if ( 0 != isInitialMasked ) {
	  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	  specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
	}
      }
      releaseMaster( theSlave -> pSpecsmaster ) ;
      return readError ;
    }
    
    nIter++ ;
    if ( nIter == 1000 ) {
      if ( initialSpeed != DCUSpeed ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
	
	if ( 0 != isInitialMasked ) {
	  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	  specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
	}
      }
      releaseMaster( theSlave -> pSpecsmaster ) ;
      return DCUAccessDenied ;
    }      
    
    getResult = ( ( retValue & 0xC0 ) == 0x80 ) ;
  }
  
  // Read lower bits in LREG
  U8 lvalue ;
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;  
  readError = specs_dcu_register_read( theSlave , DcuLREG , &lvalue ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( SpecsSuccess != readError ) {
    if ( initialSpeed != DCUSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
       
      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return readError ;
  }
  
  *data = ( ( retValue & 0x0F ) << 8 ) | ( lvalue & 0xFF ) ;
  
  mode = 0 ;
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_read_mode( theSlave , &mode ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  
  if ( SpecsSuccess != readError ) {
    if ( initialSpeed != DCUSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return readError ;
  }
  
  // Find conversion values
  if ( inputChannel < 6 ) {
    double a , b ;
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_slave_dcuconstant( theSlave , inputChannel , mode , &a , &b ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    *convertedValue = (*data) * a + b ;
  } else *convertedValue = 0. ;    
  
  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
  }
  
  releaseMaster( theSlave -> pSpecsmaster ) ;
  return SpecsSuccess ; 
}

//========================================================================
//
//========================================================================
SpecsError specs_dcu_reset(  SPECSSLAVE * theSlave ) {
  if ( 0 == theSlave ) return InvalidParameter ;
  // write '1' in bit 6 of CREG
  U8 retValue ;
  SpecsError readError = SpecsSuccess ;
  U8 initialSpeed , isInitialMasked ;

  readError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( readError != SpecsSuccess ) return readError ;

  initialSpeed = getSpeed( theSlave -> pSpecsmaster ) ;
  isInitialMasked = isChecksumMasked( theSlave -> pSpecsmaster ) ;
  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    readError = specs_master_setspeed( theSlave -> pSpecsmaster , 
				       DCUSpeed ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;

    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      readError |= specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
    
    if ( readError != SpecsSuccess ) {
      releaseMaster( theSlave -> pSpecsmaster ) ;
      return readError ;
    }
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_read( theSlave , DcuCREG , 
				       &retValue ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( SpecsSuccess != readError ) {
    if ( initialSpeed != DCUSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , 
			     initialSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return readError ;
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError |= specs_dcu_register_write( theSlave , DcuCREG , 
					 retValue | 0x40 ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  
  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
  }

  releaseMaster( theSlave -> pSpecsmaster ) ;
  return readError ;
}

//========================================================================
//
//========================================================================
SpecsError specs_dcu_initialize(  SPECSSLAVE * theSlave ) {
  if ( 0 == theSlave ) return InvalidParameter ;
  // Set bits 5 and 4 of CREG to 0
  U8 retValue ;
  SpecsError readError = SpecsSuccess ;
  U8 initialSpeed , isInitialMasked ;

  readError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( readError != SpecsSuccess ) return readError ;

  initialSpeed = getSpeed( theSlave -> pSpecsmaster ) ;
  isInitialMasked = isChecksumMasked( theSlave -> pSpecsmaster ) ;
  
  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    readError = specs_master_setspeed( theSlave -> pSpecsmaster , 
				       DCUSpeed ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;

    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      readError |= specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
    
    if ( readError != SpecsSuccess ) {
      releaseMaster( theSlave -> pSpecsmaster ) ;
      return readError ;
    }
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_read( theSlave , DcuCREG , &retValue ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  
  if ( SpecsSuccess != readError ) {
    if ( DCUSpeed != initialSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , DCUSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return readError ;
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_write( theSlave , DcuCREG , 
					retValue & 0xCF ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  
  if ( SpecsSuccess != readError ) {
    if ( DCUSpeed != initialSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , DCUSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;

      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return readError ;
  }
    
  // TREG is written with 0x10
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_write( theSlave , DcuTREG , 0x10 ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  if ( SpecsSuccess != readError ) {
    if ( DCUSpeed != initialSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , DCUSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return readError ;
  }

  // AREG is written with 0x00
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_write( theSlave , DcuAREG , 0x00 ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  
  if ( DCUSpeed != initialSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_master_setspeed( theSlave -> pSpecsmaster , DCUSpeed ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
  }
  releaseMaster( theSlave -> pSpecsmaster ) ;
  return readError ;
}

//========================================================================
//
//========================================================================
SpecsError specs_dcu_set_LIR(  SPECSSLAVE * theSlave ) {
  if ( 0 == theSlave ) return InvalidParameter ;
  // Read content of CREG
  U8 retValue ;
  SpecsError readError = SpecsSuccess ;
  U8 initialSpeed , isInitialMasked ;

  readError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( readError != SpecsSuccess ) return readError ;
    
  initialSpeed = getSpeed( theSlave -> pSpecsmaster ) ;
  isInitialMasked = isChecksumMasked( theSlave -> pSpecsmaster ) ;
  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    readError = specs_master_setspeed( theSlave -> pSpecsmaster , 
				       DCUSpeed ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    
    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      readError |= specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
    
    if ( readError != SpecsSuccess ) {
      releaseMaster( theSlave -> pSpecsmaster ) ;
      return readError ;
    }
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_read( theSlave , DcuCREG , &retValue ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  
  if ( SpecsSuccess != readError ) {
    if ( initialSpeed != DCUSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }
 
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return readError ;
  }
    
  // write '1' in bit 3 of CREG
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_write( theSlave , DcuCREG , 
					retValue | 0x08 ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;

    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
  }

  releaseMaster( theSlave -> pSpecsmaster ) ;
  return readError ;
}

//========================================================================
//
//========================================================================
SpecsError specs_dcu_set_HIR(  SPECSSLAVE * theSlave ) {
  if ( 0 == theSlave ) return InvalidParameter ;
  // Read content of CREG
  U8 retValue ;
  SpecsError readError = SpecsSuccess ;
  U8 initialSpeed , isInitialMasked ;
  
  readError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( readError != SpecsSuccess ) return readError ;
  
  initialSpeed = getSpeed( theSlave -> pSpecsmaster ) ;
  isInitialMasked = isChecksumMasked( theSlave -> pSpecsmaster ) ;
  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    readError = specs_master_setspeed( theSlave -> pSpecsmaster , 
				       DCUSpeed ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    
    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      readError |= specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
   
    if ( readError != SpecsSuccess ) {
      releaseMaster( theSlave -> pSpecsmaster ) ;
      return readError ;
    }
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_read( theSlave , DcuCREG , &retValue ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( SpecsSuccess != readError ) {
    if ( initialSpeed != DCUSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;	
      specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return readError ;
  }
    
  // write '0' in bit 3 of CREG
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  readError = specs_dcu_register_write( theSlave , DcuCREG , 
					retValue & 0xF7 ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( initialSpeed != DCUSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
  }

  releaseMaster( theSlave -> pSpecsmaster ) ;
  return readError ;
}

//========================================================================
//
//========================================================================
SpecsError specs_dcu_read_mode(  SPECSSLAVE * theSlave ,
                                 U8         * mode ) {
  if ( 0 == theSlave ) return InvalidParameter ;
  // Mode is bit 3 of CREG
  U8 retValue ;

  SpecsError readError = specs_dcu_register_read( theSlave , DcuCREG , 
                                                  &retValue ) ;
  if ( SpecsSuccess != readError ) return readError ;
  
  *mode = 0 ;
  if ( ( retValue & 0x08 ) == 0x08 ) *mode = 1 ;
  
  return SpecsSuccess ;
}

//========================================================================
//
//========================================================================
SpecsError specs_master_testreg_write(  SPECSMASTER * theMaster ,
                                        U16 value ) {
  if ( 0 == theMaster ) return InvalidParameter ;
  SpecsPlxTestWrite( theMaster , (U32) value ) ;
  //  *(theMaster -> pSpecsmasterTestRegister) = (U32) value ;
  return SpecsSuccess ;
}

//========================================================================
//
//========================================================================
SpecsError specs_master_testreg_read(  SPECSMASTER * theMaster ,
                                       U16 * value ) {
  if ( 0 == theMaster ) return InvalidParameter ;
  *value = static_cast< U16 > ( SpecsPlxTestRead( theMaster ) & 0xFFFF ) ;
  return SpecsSuccess ; 
}

//========================================================================
//
//========================================================================
U32 specs_master_date_prom( SPECSMASTER * theMaster ) {
  if ( 0 == theMaster ) return InvalidParameter ;
  U32 Data;
  RETURN_CODE rc= ApiSuccess;
  //	RETURN_CODE rc = PlxBusIopRead( theMaster -> hdle , IopSpace0 , 0x1000058 , 
  //                                FALSE , &Data , 1 , BitSize32 ) ;

  if ( rc != ApiSuccess ) return InvalidParameter ;

  U32 tempResult = static_cast< U32 > ( Data & 0xFFFFFFFF ) ;
  return tempResult ; 
}

//========================================================================
//
//========================================================================
SpecsError specsUserFIFORead( SPECSMASTER * specsMtr, U32 *specsData,
                              U32 nWordsSpecs)
{
  U32 iheader = 0x76543210;
  for ( unsigned int i = 0 ; i < nWordsSpecs ; ++i ) specsData[ i ] = 0 ;

  SpecsError theError = SpecsSuccess ; 

  RETURN_CODE errorCodeFromLowLevelSpecsLib = 
    SpecsmasterReceiverFIFORead(specsMtr,specsData,nWordsSpecs);

  if (errorCodeFromLowLevelSpecsLib == ApiSuccess ) 
    theError = SpecsSuccess ; 
  if (errorCodeFromLowLevelSpecsLib == ApiWaitTimeout ) {
    //    specs_master_softreset( specsMtr ) ;
    theError |= ReadAccessDenied ;
  }
  if (errorCodeFromLowLevelSpecsLib == ApiAccessDenied ) {
    //    specs_master_softreset( specsMtr ) ;
    theError |= ReadAccessDenied ;
  }
  if (errorCodeFromLowLevelSpecsLib == ApiInvalidData ) {
    //    printf("errorCodeFromLowLevelSpecsLib == ApiInvalidData \n");
    specs_master_softreset( specsMtr ) ;
    theError |= ChecksumError ;
  }
  if (errorCodeFromLowLevelSpecsLib == ApiInvalidSize ) {
    //    specs_master_softreset( specsMtr ) ;
    theError |= InvalidBufferSize ; 
  }

  // Test SPECS FIFO COMPATIBILITY
  if (iheader!=specsData[0]) {
    // pb ds le header il n est pas la bonne position 
    // on essaye eventuellement de relire
    //    specs_master_softreset( specsMtr ) ;
    theError |= NoFrameSeparator ;
  }
  if (specsData[0]==specsData[1]) {
    // header repete 2 fois. 
    // on essaye eventuellement de relire
    //    specs_master_softreset( specsMtr ) ;
    theError |= FrameSeparatorRepetition ;
  }
  if (specsData[1]==specsData[2]) {
    // le 2eme mot est egal au 3eme 
    // on essaye eventuellement de relire
    //    specs_master_softreset( specsMtr ) ;
    theError |= HeaderRepetition ; 
  }
 
  return theError ; 
}

//========================================================================
//
//========================================================================
SpecsError  specsUserCheckParameters(  SPECSSLAVE * theSlave )
{
  if ( NULL == theSlave )
    {
      return InvalidParameter ;
    }
  else if ( 0 == theSlave -> pSpecsmaster )
    {
      return InvalidParameter ; 
    }
  return SpecsSuccess ; 
}

//========================================================================
//
//========================================================================
SpecsError JtagWriteReadMany( SPECSSLAVE * pSpecsslave, U8 outSelect,
                              U8 *pDataIn, U8 *pDataOut, U32 nBits,
                              JTAG_OPER oper, int header, int trailler)
{
  int ptrIn[10], ptrOut[10];
  int nBytes, i, index = 0;
  int tail, head, n;
  RETURN_CODE ret ;
  SpecsError theError = SpecsSuccess ;
  
  if(nBits > 48)
    {
      nBytes = JTAG_calcNBytes(nBits);
      for(i = 0; i < nBytes; i += 6)
        {
          ptrIn[index] = i;
          ptrOut[index] = i;
          index++;
        }
      index--;

      theError = reserveMaster( pSpecsslave -> pSpecsmaster ) ;
      if ( theError != SpecsSuccess ) return theError ;

      for(i = index; i >= 0 ; i--)
	{
	  tail = 0;
	  head = 0;
	  n = 48;
	  if (i == index)
	    {
	      head = 1;
	      n = nBits % 48;
	    }
	  if(i == 0)
	    {			
	      tail = 1;
	    }
	  ret = JtagWriteRead( pSpecsslave, outSelect,
			       &pDataIn[ptrIn[i]],
			       &pDataOut[ptrOut[i]],
			       n, oper, head, tail);
	  if ( ApiInvalidSize == ret ) { 
	    releaseMaster( pSpecsslave -> pSpecsmaster ) ;
	    return InvalidBufferSize ;
	  } else if ( ApiAccessDenied == ret ) {
	    releaseMaster( pSpecsslave -> pSpecsmaster ) ;
	    return WriteAccessDenied ;
	  } else if ( ApiWaitTimeout == ret ) {
	    releaseMaster( pSpecsslave -> pSpecsmaster ) ;
	    return ReadAccessDenied ;
	  } else if ( ApiInvalidData == ret ) {
	    releaseMaster( pSpecsslave -> pSpecsmaster ) ;
	    return ChecksumError ;
	  }

	  theError |= checkSlaveIt( pSpecsslave ) ;
	  if ( SpecsSuccess != theError ) {
	    releaseMaster( pSpecsslave -> pSpecsmaster ) ;
	    return theError ;
	  }
	}
      releaseMaster( pSpecsslave -> pSpecsmaster ) ;
    } 
  else
    {
      theError = reserveMaster( pSpecsslave -> pSpecsmaster ) ;
      if ( theError != SpecsSuccess ) return theError ;

      ret = JtagWriteRead( pSpecsslave, outSelect,
			   pDataIn,
			   pDataOut,
			   nBits, oper, header, trailler);

      if ( ApiInvalidSize == ret ) { 
	releaseMaster( pSpecsslave -> pSpecsmaster ) ;
	return InvalidBufferSize ;
      } else if ( ApiAccessDenied == ret ) {
	releaseMaster( pSpecsslave -> pSpecsmaster ) ;
	return WriteAccessDenied ;
      } else if ( ApiWaitTimeout == ret ) {
	releaseMaster( pSpecsslave -> pSpecsmaster ) ;
	return ReadAccessDenied ;
      } else if ( ApiInvalidData == ret ) {
	releaseMaster( pSpecsslave -> pSpecsmaster ) ;
	return ChecksumError ;
      }

      theError |= checkSlaveIt( pSpecsslave ) ;
      theError |= releaseMaster( pSpecsslave -> pSpecsmaster ) ;
      if ( SpecsSuccess != theError ) return theError ;

    }
  return SpecsSuccess ;
}

//========================================================================
//
//========================================================================
SpecsError specs_master_enableread_reset( SPECSMASTER * theMaster ) 
{
  if ( 0 == theMaster ) return InvalidParameter ;
  
  U32 stat = SpecsPlxStatusRead( theMaster ) ;
  
  while ( stat & 0x1 ) {
    // Reset Enable Read Bit
    SpecsmasterCtrlWrite( theMaster , ResetRdBit   ) ;
    // TO DO LATER !!!!!!!!!!
    // BE ABLE TO READ BACK SPEED OF MASTER
    SpecsmasterCtrlWrite( theMaster , 0 ) ;
    stat = SpecsPlxStatusRead( theMaster ) ;
  }
  return SpecsSuccess ;
}

//========================================================================
//
//========================================================================
SpecsError specs_master_recFIFO_reset( SPECSMASTER * theMaster ) 
{
  if ( 0 == theMaster ) return InvalidParameter ;
  printf("Ne doit pas passer dans  specs_master_recFIFO_reset ! \n");
  U32 stat = SpecsPlxStatusRead( theMaster ) ;

  while ( stat & 0x1 ) {
    // Reset Receiver FIFO
    SpecsmasterCtrlWrite( theMaster , ResetRecFIFO ) ;
    // TO DO LATER !!!!!!!!!!
    // BE ABLE TO READ BACK SPEED OF MASTER
    SpecsmasterCtrlWrite( theMaster , 0 ) ;
    stat = SpecsPlxStatusRead( theMaster ) ;
  }
  return SpecsSuccess ;
}

//======================================================================
//
//======================================================================
SpecsError specs_master_setspeed( SPECSMASTER * theMaster , U8 speed ) {
  if ( 0 == theMaster ) return InvalidParameter ;
  int kk ;
  unsigned int j = 0 ;
  idMapIterator idIt ;
  U32 n ;

  SpecsError theError = reserveMaster_checkRecursive( theMaster ) ; 
  if ( theError != SpecsSuccess ) return theError ;
  
  n = ( SpecsPlxStatusRead( theMaster ) >> 8 ) & 0xFF ;
    
  while ( n != 0 ) {
    if ( j++ >= 1000000 ) {
      releaseMaster_checkRecursive( theMaster ) ;
      return WriteAccessDenied ;
    }
    n = ( SpecsPlxStatusRead( theMaster ) >> 8 ) & 0xFF ;
  }

  // TEMPORARY FIX !
  for ( kk = 0 ; kk < 30000 ; ++kk ) {
    n=n+1;
  }
  U32 ctrlValue;
  ctrlValue = SpecsmasterCtrlRead(theMaster ); //MTQ pour ne pas effacer les autres bits 
   
  SpecsmasterCtrlWrite( theMaster ,  (ctrlValue & 0xFFFFFFF8) | (speed & 0x7) ) ;
  releaseMaster_checkRecursive( theMaster ) ;
  
  return SpecsSuccess ;
}


//======================================================================
//
//======================================================================
SpecsError specs_master_maskchecksum( SPECSMASTER * theMaster ) {
  if ( 0 == theMaster ) return InvalidParameter ;
  int kk ;

  unsigned int j = 0 ;
  idMapIterator idIt ;
  U32 n ;
  U8 speed ;
  SpecsError theError = reserveMaster_checkRecursive( theMaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  n = ( SpecsPlxStatusRead( theMaster ) >> 8 ) & 0xFF ;
    
  while ( n != 0 ) {
    if ( j++ >= 1000000 ) {
      releaseMaster_checkRecursive( theMaster ) ;
      return WriteAccessDenied ;
    }
    
    n = ( SpecsPlxStatusRead( theMaster ) >> 8 ) & 0xFF ;
  }
  
  // TEMPORARY FIX !
  for ( kk = 0 ; kk < 30000 ; ++kk ) {
    n=n+1;
  }
  
  // get speed of master A REVOIR
  speed = getSpeed( theMaster ) ;

  // MTQ  a l'origine  SpecsmasterCtrlWrite( theMaster , MaskChecksumBit | speed & 0x7 ) ;
  U32 ctrlValue;
  ctrlValue = SpecsmasterCtrlRead( theMaster ); //MTQ pour ne pas effacer les autres bits 
  //  SpecsmasterCtrlWrite( theMaster ,(ctrlValue & 0xFFFFFF78) | (MaskChecksumBit |( speed & 0x7 ))) ;
 
  SpecsmasterCtrlWrite( theMaster , ( (ctrlValue & 0xFFFFFF7F)));
  releaseMaster_checkRecursive( theMaster ) ;
  return SpecsSuccess ;
}

//======================================================================
//
//======================================================================
SpecsError specs_master_unmaskchecksum( SPECSMASTER * theMaster ) {
  if ( 0 == theMaster ) return InvalidParameter ;
  int kk ;

  unsigned int j = 0 ;
  idMapIterator idIt ;
  U32 n ;
  U8 speed ;
  
  SpecsError theError = reserveMaster_checkRecursive( theMaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  n = ( SpecsPlxStatusRead( theMaster ) >> 8 ) & 0xFF ;
  
  while ( n != 0 ) {
    if ( j++ >= 1000000 ) {
      releaseMaster_checkRecursive( theMaster ) ;
      return WriteAccessDenied ;
    }
    n = ( SpecsPlxStatusRead( theMaster ) >> 8 ) & 0xFF ;
  }
  
  // TEMPORARY FIX !
  for ( kk = 0 ; kk < 30000 ; ++kk ) {
    n=n+1;
  }
  
  // get speed of master A REVOIR
  speed = getSpeed( theMaster ) ;
  U32 ctrlValue;
  ctrlValue = SpecsmasterCtrlRead(theMaster ); //MTQ pour ne pas effacer les autres bits 
  //  SpecsmasterCtrlWrite( theMaster , ( (ctrlValue & 0xFFFFFF78) | ( MaskChecksumBit | ( speed & 0x7 ) ))) ;
 
  SpecsmasterCtrlWrite( theMaster , ( (ctrlValue | 0x80)));
  
  releaseMaster_checkRecursive( theMaster ) ;
  return SpecsSuccess ;
}

//==========================================================================
//
//==========================================================================
SpecsError specs_slave_write_eeprom( SPECSSLAVE * theSlave , U8 pageNumber ,
                                     U8 startAddress , U8 nValues ,
                                     U8 * data ) {
  if ( 0 == theSlave ) return InvalidParameter ;
  if ( 0 == pageNumber ) return InvalidParameter ;
  if ( pageNumber > 127 ) return InvalidParameter ;
  if ( startAddress > 63 ) return InvalidParameter ;
  if ( nValues == 25 ) return InvalidParameter ;

  SpecsError theError = SpecsSuccess ;
  
  U8 initialSpeed , isInitialMasked ;
  U16 value ;
  U8 newData[ 70 ] ;
  U16 MemAdd = ( pageNumber << 6 ) | startAddress ;
  
  newData[ 1 ] = static_cast< U8 >( MemAdd & 0xFF ) ;
  newData[ 0 ] = static_cast< U8 >( ( MemAdd & 0xFF00 ) >> 8 ) ;

  unsigned int i ;
  for ( i = 0 ; i < nValues ; ++i ) newData[ i + 2 ] = data[ i ] ;
  
  theError = reserveMaster( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  initialSpeed = getSpeed( theSlave -> pSpecsmaster ) ;
  isInitialMasked = isChecksumMasked( theSlave -> pSpecsmaster ) ;
  if ( 3 != initialSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    theError = specs_master_setspeed( theSlave -> pSpecsmaster , 3 ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      theError |= specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
    
    if ( theError != SpecsSuccess ) {
      releaseMaster( theSlave -> pSpecsmaster ) ;
      return theError ;
    }    
  }
    
  // Recursive call
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = specs_register_read( theSlave , MezzaCtrlReg , 
				  &value ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  
  if ( theError != SpecsSuccess ) {
    if ( 3 != initialSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , 
			     initialSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return theError ;
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = specs_register_write( theSlave , MezzaCtrlReg , 
				   ( value & 0x9F ) | 0x40 ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  
  if ( theError != SpecsSuccess ) {
    if ( 3 != initialSpeed ) {
      specs_master_setspeed( theSlave -> pSpecsmaster , 
			     initialSpeed ) ;
      if ( 0 != isInitialMasked ) {
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      }
    }
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return theError ;
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = specs_i2c_write( theSlave , EEPROMOutputSelect , 
			      EEPROMAddress , nValues + 2 , newData ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( theError != SpecsSuccess ) {
    if ( 3 != initialSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , 
			     initialSpeed ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      
      if ( 0 != isInitialMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
	specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	recursiveLock[ theSlave -> pSpecsmaster ] = false ;
      }
    }

    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_register_write( theSlave , MezzaCtrlReg , 
			  ( value & 0x9F ) | 0x20 ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    
    releaseMaster( theSlave -> pSpecsmaster ) ;
    return theError ;
  }
    
  usleep( 30000 ) ;

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = specs_register_write( theSlave , MezzaCtrlReg , 
				   ( value & 0x9F ) | 0x20 ) ;
  recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( 3 != initialSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_master_setspeed( theSlave -> pSpecsmaster , 
			   initialSpeed ) ;
    recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    if ( 0 != isInitialMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
  }
  releaseMaster( theSlave -> pSpecsmaster ) ;
  return theError ;
}

//====================================================================
//
//====================================================================
SpecsError specs_slave_read_eeprom( SPECSSLAVE * theSlave , 
                                    U8 pageNumber , U8 startAddress ,
                                    U8 nValues , U8 * data ) {
  bool isFromRecursiveLock = true ;
  if ( 0 == theSlave ) return InvalidParameter ;
  if ( pageNumber > 127 ) return InvalidParameter ;
  if ( startAddress > 63 ) return InvalidParameter ;

  SpecsError theError = SpecsSuccess ;
  
  U8 initialSpeed , isInitialMasked ;
  U16 value ;
  U8 newData[ 2 ] ;
  U16 MemAdd = ( pageNumber << 6 ) | startAddress ;
  
  newData[ 1 ] = static_cast< U8 >( MemAdd & 0xFF ) ;
  newData[ 0 ] = static_cast< U8 >( ( MemAdd & 0xFF00 ) >> 8 ) ;

  if ( !   recursiveLock[ theSlave -> pSpecsmaster ] )
    isFromRecursiveLock = false ;
  
  theError = reserveMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  if ( theError != SpecsSuccess ) return theError ;

  initialSpeed = getSpeed( theSlave -> pSpecsmaster ) ;
  isInitialMasked = isChecksumMasked( theSlave -> pSpecsmaster ) ;
 
  if ( 3 != initialSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    theError = specs_master_setspeed( theSlave -> pSpecsmaster , 3 ) ;
    if ( ! isFromRecursiveLock )  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    if ( 0 != isChecksumMasked ) {  // MTQ  appel de fonction ou c'est isInitialMasked ???
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      theError |= specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      if ( ! isFromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    }
    if ( theError != SpecsSuccess ) {
      releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
      return theError ;
    }   
  }
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = specs_register_read( theSlave , MezzaCtrlReg , &value ) ;
  if( ! isFromRecursiveLock )   recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  if ( theError != SpecsSuccess ) {
    if ( 3 != initialSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
      if ( ! isFromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;

      if ( ! isFromRecursiveLock ) 

        if ( 0 != isChecksumMasked ) {
	  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
          specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	  if ( ! isFromRecursiveLock )   recursiveLock[ theSlave -> pSpecsmaster ] = false ;
        }
    }

    releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
    return theError ;
  }
    
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = specs_register_write( theSlave , MezzaCtrlReg , 
				   ( value & 0x79F ) | 0x40 ) ;
  if( ! isFromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;

  if ( theError != SpecsSuccess ) {
    if ( 3 != initialSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
      if ( ! isFromRecursiveLock )   recursiveLock[ theSlave -> pSpecsmaster ] = false ;

        if ( 0 != isChecksumMasked ) {
	  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
          specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	  if ( ! isFromRecursiveLock )   recursiveLock[ theSlave -> pSpecsmaster ] = false ;
        }
    }
    releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
    return theError ;
  } 
  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = specs_i2c_combinedread( theSlave , EEPROMOutputSelect , 
				     EEPROMAddress , newData , 2 , 
				     nValues , data ) ;
  if ( ! isFromRecursiveLock )   recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  
  if ( theError != SpecsSuccess ) {
    if ( 3 != initialSpeed ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
      if ( ! isFromRecursiveLock )  recursiveLock[ theSlave -> pSpecsmaster ] = false ;
 
      if ( 0 != isChecksumMasked ) {
	recursiveLock[ theSlave -> pSpecsmaster ] = true ;
          specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
	  if ( ! isFromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;
        }
    }
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_register_write( theSlave , MezzaCtrlReg , 
			  ( value & 0x9F ) | 0x20 ) ;
    if( ! isFromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
    return theError ;
  }

  recursiveLock[ theSlave -> pSpecsmaster ] = true ;
  theError = specs_register_write( theSlave , MezzaCtrlReg , 
				   ( value & 0x9F ) | 0x20 ) ;
  if( ! isFromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;
  if ( 3 != initialSpeed ) {
    recursiveLock[ theSlave -> pSpecsmaster ] = true ;
    specs_master_setspeed( theSlave -> pSpecsmaster , initialSpeed ) ;
    if ( ! isFromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    
    if ( 0 != isChecksumMasked ) {
      recursiveLock[ theSlave -> pSpecsmaster ] = true ;
      specs_master_maskchecksum( theSlave -> pSpecsmaster ) ;
      if (! isFromRecursiveLock ) recursiveLock[ theSlave -> pSpecsmaster ] = false ;
    } 
  }
  theError |= releaseMaster_checkRecursive( theSlave -> pSpecsmaster ) ;
  return theError ;
}

//=========================================================================
//
//=========================================================================
SpecsError specs_slave_serialNumber( SPECSSLAVE * theSlave , U16 * SN ) {
  U8 data[ 2 ] ;
  *SN = 0 ;
  SpecsError readError = 
    specs_slave_read_eeprom( theSlave , 0 , 0 , 2 , data ) ;
  if ( SpecsSuccess == readError ) 
    *SN = ( ( data[ 0 ] << 8 ) | data[ 1 ] ) ;
  
  return readError ;
}

//==========================================================================
//
//==========================================================================
SpecsError specs_slave_dcuconstant( SPECSSLAVE * theSlave , 
                                    U8 DCUChannel , U8 DCUMode , double * a , 
                                    double * b ) {
  U8 data[ 4 ] ;
  *a = 0. ;
  *b = 0. ;
  
  U8 startAddress = 2 + DCUChannel * 8 + 4 * DCUMode ;

  SpecsError readError = 
    specs_slave_read_eeprom( theSlave , 0 , startAddress , 4 , data ) ;
  if ( SpecsSuccess == readError ) {
    if ( DCUMode == 0 ) 
      *a = (double) ( data[0] << 8 | data[1] ) / 100000000. ;
    else *a = - (double) ( data[0] << 8 | data[1] ) / 100000000. ;

    *b = (double) ( data[2] << 8 | data[3] ) / 1000. ;
  }  

  return readError ;
}

//==================================================================
//
//==================================================================
U8 getSpeed( SPECSMASTER * theMaster ) {
  

  U32 plxReg = SpecsmasterCtrlRead(theMaster);
  /* MTQ ne fonctionne plus pareil
     idMapIterator idIt = idMap.find( theMaster ) ;
     unsigned int id = idIt -> second ;

  
     U8 speed = 
     static_cast< U8 >( ( ( ( ( 0x7 << ( ( id - 1 ) * 3 ) ) ) & 
     plxReg ) >> 
     ( ( id - 1 ) * 3 ) ) & 0x7 ) ;
    

     static_cast< U8 >( plxReg &0x7);
  */
  return (plxReg & 0x7) ;
}

//==================================================================
//
//==================================================================
U8 isChecksumMasked( SPECSMASTER * theMaster ) {
  RETURN_CODE rc ;
  U32 plxReg = PlxRegisterRead( theMaster -> hdle , 0x38 , &rc ) ;
  idMapIterator idIt = idMap.find( theMaster ) ;
  unsigned int id = idIt -> second ;
  
  U8 isMasked = 
    static_cast< U8 >( ( ( ( ( 0x1 << ( ( id - 1 ) + 12 ) ) ) & 
                           plxReg ) >> 
                         ( ( id - 1 ) + 12 ) ) & 0x1 ) ;

  return isMasked ;
}

//=================================================================
// Check Interrupt from Slave
//=================================================================
SpecsError checkSlaveIt( SPECSSLAVE * theSlave ) {  
  SpecsError theError = SpecsSuccess ;
  // First do the polling
  bool gotIt = false ;
  bool dontReset = false ;
  idMapIterator idIt = idMap.find( theSlave -> pSpecsmaster ) ;// MTQ
  unsigned int id = idIt -> second ;// MTQ
  // NOuveau code par MTQ le 25/4/14
  int state;
  U32 address , itType ;
  state = PlxCheckSlaveIt( theSlave -> pSpecsmaster -> hdle , theSlave -> pSpecsmaster->masterID );
  if (state) {
    if (state & 0x100)  theError = ChecksumError ;
    address= (state &0x1F0000) >>16;
    itType = ( state & ItFrame ) ;
   
    // check slave addresses:
    if ( address != theSlave -> SpecsslaveAdd ) {
      theError = WrongDeviceInfo ;
    } 
    
    printf("ATTENTION IT FRAME The error %d state = %x\n",theError,state);
  }
 
  return  theError ; // MTQ quoi faire ??

  // Wait for IT2

  //ATTENTION il faut IntSource avec une valeur diffrente pour valier les PIO
  RETURN_CODE EventStatus = 
    PlxIntrEnable( theSlave -> pSpecsmaster -> hdle , &IntSources,  theSlave -> pSpecsmaster->pSpecsmasterStatus) ;
  if ( ApiSuccess != EventStatus ) return WrongDeviceInfo ;

  unsigned nTry = 0 ;
  
  while ( ( nTry < 10000 ) && ( ! gotIt ) ) {
 
   
    //MTQ  PlxNotificationWait( theSlave -> pSpecsmaster -> hdle ,
    //                    &intEventMap[ theSlave -> pSpecsmaster ] ,
    //                    ItDelay ) ;  

    PlxNotificationWait( theSlave -> pSpecsmaster -> hdle ,
			 id,
			 ItDelay ) ;
    if ( true ) { //EventStatus == ApiSuccess
      // Read Global Reg:
      U32 globalReg ;
      PlxBusIopRead( theSlave -> pSpecsmaster -> hdle ,theSlave -> pSpecsmaster ->masterID, 
                     GLOBAL_REG , FALSE , &globalReg , 1 , BitSize32 ) ;

      // Check if it is the same Master
      if ( globalReg & 
           ( 0x1000000 << ( 2 * ( idMap[ theSlave -> pSpecsmaster ] - 1 )))) {
        gotIt = true ;
      }
      else { 
        if ( ApiSuccess != EventStatus ) {
          theError = SpecsMasterLocked ;
          break ;
        }
        ++nTry ;
        continue ;
      }      
     
      U16 intVect ;
      // Read IT Type
      if ( globalReg & 
	   ( 0x1000000 << (1+2 * ( idMap[ theSlave -> pSpecsmaster ] - 1)))){
      
        // Non empty IT
        U32 statusReg ;
        U32 address , itType ;
        // Read status

        SpecsmasterStatusRead( theSlave -> pSpecsmaster , &statusReg ) ;
        address = ( ( statusReg & 0x1F0000 ) >> 16 ) ;
        itType = ( statusReg & ItFrame ) ;
        if ( ( itType != ItIsChecksum ) && ( itType != ItIsI2cAck ) ) {
          // User or IO IT => Not waiting for it but still activate
          // but do not reset it
          gotIt = true ;
          dontReset = true ;
        } else {
          // check slave addresses:
          if ( address != theSlave -> SpecsslaveAdd ) {
            theError = WrongDeviceInfo ;
          } else {
            // ACK IT
            specs_register_read_withoutIt( theSlave , IntVect , &intVect , 
                                           false ) ;
            // Checksum Error 
            if ( itType == ItIsChecksum ) {
              if ( ( intVect & ChecksumBit ) != ChecksumBit ) 
                theError = WrongDeviceInfo ;
              else theError = ChecksumError ;
            } else if ( itType == ItIsI2cAck ) {
              if ( ( intVect & I2CAckInterruptBit ) != I2CAckInterruptBit ) 
                theError = WrongDeviceInfo ;
              else theError = NoI2cAck ;
            } else theError = WrongDeviceInfo ;
          } 
        }
      } else {
        // Empty IT
        break ;
      }      
    } else {
      theError = SpecsMasterLocked ;
      break ;
    }
  }
  
  if ( ( gotIt ) && ( ! dontReset ) ) {
    // Clear IT 
    U8 speed ;
    U32 value , Un , Zero ;
    Un = 0x40 ;
    Zero = 0x0 ;
    speed = getSpeed( theSlave -> pSpecsmaster ) ;
    value = Un | speed ;
    U32 ctrlValue;
    ctrlValue = SpecsmasterCtrlRead(theSlave -> pSpecsmaster  ); //MTQ pour ne pas effacer les autres bits 
    SpecsmasterCtrlWrite( theSlave -> pSpecsmaster ,(ctrlValue& 0xFFFFFFF8) | value ) ;
    value = Zero | speed ;
    SpecsmasterCtrlWrite( theSlave -> pSpecsmaster , (ctrlValue& 0xFFFFFFF8) | value ) ;
  } else if ( ! gotIt ) {
    theError = SpecsMasterLocked ;
  }
  
  PlxIntrDisable( theSlave -> pSpecsmaster -> hdle , &IntSources ,theSlave -> pSpecsmaster->pSpecsmasterStatus) ;
  return theError ;  
}


//========================================================================
// Read without it from It check Slave
//========================================================================
SpecsError specs_register_read_withoutIt( SPECSSLAVE * theSlave ,
                                          U8 theRegister        ,
                                          U16 * data            ,
                                          bool checkLock ) {
  /* Test of all the arguments before calling the relevant functions ... */
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( theError != SpecsSuccess ) return theError ;

  SPECSMASTER* specsMtr = theSlave->pSpecsmaster ;
  U32 specsData[5] ;
  
  for ( int ii = 0 ; ii < 5 ; ++ii ) specsData[ ii ] = 0 ;

  U32 size = 3;

  RETURN_CODE errorCodeFromLowLevelSpecsLib ;
  if (checkLock ) {
    theError = reserveMaster( theSlave -> pSpecsmaster ) ;
    if ( theError != SpecsSuccess ) return theError ;
  }
  
  errorCodeFromLowLevelSpecsLib = RegisterRead(theSlave,theRegister,data);

  // HERE a END Frame IT has been generated, so we must clear it
   
  switch ( errorCodeFromLowLevelSpecsLib ) {
  case ApiSuccess:
    theError |= SpecsSuccess ; break ;
  case ApiAccessDenied:
    theError |= WriteAccessDenied ; break ;
  case ApiInvalidSize:
    theError |= InvalidBufferSize ; break ;
  default:
    break ;
  }

  // le bazar sur la FIFO ..
  theError |= specsUserFIFORead(specsMtr,specsData,size); 

  if ( checkLock )
    releaseMaster( specsMtr ) ;
  
  if ( SpecsSuccess == theError ) {
    U8 data8Bits[2] ;
    data8Bits[0] = static_cast< U8 > ( specsData[1] & 0xFF ) ; 
    data8Bits[1] = static_cast< U8 > ( ( specsData[2] & 0xFF000000 ) 
				       >> 24 ) ; 
    // on colle les 2 morceaux de 8 bits 
    // (apres avoir decale la partie haute de 8 bits
    U16 highPart = data8Bits[0]<<8 ; 
      
    *data = highPart |  data8Bits[1] ;
  }
    
  return theError ; 
}

//===========================================================================
// Interrupt function
//===========================================================================
SpecsError specs_slave_waitForInterrupt( SPECSSLAVE * theSlave ,
                                         U16 InterruptType , 
                                         U32 timeOut , 
                                         U16 * ItOut ) 
{
  idMapIterator idIt = idMap.find( theSlave -> pSpecsmaster ) ;// MTQ
  unsigned int id = idIt -> second ;// MTQ
  (*ItOut) = 0 ;
  SpecsError theError = specsUserCheckParameters( theSlave ) ;
  if ( theError != SpecsSuccess ) return theError ;

  // Do the polling first  
  bool gotIt = false ;
  printf( " je ne dois pas passer par la\n");
  // Wait for IT2
  // attention a la valeur de INtSource
  RETURN_CODE EventStatus = 
    PlxIntrEnable( theSlave -> pSpecsmaster -> hdle , &IntSources, theSlave -> pSpecsmaster ->pSpecsmasterStatus ) ;
  if ( ApiSuccess != EventStatus ) return WrongDeviceInfo ;

  unsigned int nTry = 0 ;
 
  while ( ( ! gotIt ) && ( nTry < 1000 ) ) {
    EventStatus = 
      //  PlxNotificationWait( theSlave -> pSpecsmaster -> hdle ,
      //                   &intEventMap[ theSlave -> pSpecsmaster ] ,
      //                   timeOut * 1000 ) ;

      PlxNotificationWait( theSlave -> pSpecsmaster -> hdle ,
                           id ,
                           timeOut * 1000 ) ;
    
    if ( EventStatus != ApiSuccess ) { 
      return SpecsTimeout ;
    }
    
    // Read Global Reg
    /* 
       PlxBusIopRead( theSlave -> pSpecsmaster -> hdle , IopSpace0 , 
       GLOBAL_REG , FALSE , &globalReg , 1 , BitSize32 ) ;
    
       // Check if it is the same Master
       if ( ! ( globalReg & 
       ( 0x1000000 << ( 2 * ( idMap[ theSlave -> pSpecsmaster ] - 1))))){
       ++nTry ;
       continue ;
       }
    
       // Read IT Type
       if ( ! ( globalReg & 
       (0x1000000<<(1+2*(idMap[theSlave -> pSpecsmaster] - 1))))){
       ++nTry ;
       continue ;
       }

       // Non empty IT
       U32 statusReg ;
       U32 address , itType ;
       // Read status
       SpecsmasterStatusRead( theSlave -> pSpecsmaster , &statusReg ) ;
       address = ( ( statusReg & 0x1F0000 ) >>16 ) ;
       itType = ( statusReg & ItFrame ) ;
       if ( ( itType == ItIsChecksum ) || ( itType == ItIsI2cAck ) ) {
       ++nTry ;
       continue ;
       }

       // check IT address :
       if ( address != theSlave -> SpecsslaveAdd ) { ++nTry ; continue ; }
    
       // check User or IO IT
       if ( ( UserInterruptBit & InterruptType ) && ( itType != ItIsUserInt ) ) 
       { ++nTry ; continue ; }
       if ( ( ( IO0InterruptBit & InterruptType ) ||
       ( IO1InterruptBit & InterruptType ) ||
       ( IO2InterruptBit & InterruptType ) ||
       ( IO3InterruptBit & InterruptType ) ||
       ( IO4InterruptBit & InterruptType ) ||
       ( IO5InterruptBit & InterruptType ) ||
       ( IO6InterruptBit & InterruptType ) ||
       ( IO7InterruptBit & InterruptType ) ) &&
       ( itType != ItIsIO ) ) {
       ++nTry ;
       continue ;
       }
       MTQ    NO GLOBAL REG */
    gotIt = true ;
    // ACK IT2
    U16 intVect ;
    printf(" peut tre la .................\n");
    specs_register_read_withoutIt( theSlave , IntVect , &intVect , true ) ;
    if ( ! ( intVect & InterruptType ) ) theError = WrongDeviceInfo ;
    else (*ItOut) = intVect ;
  }

  gotIt = true ;//MTQ
  if ( gotIt ) {
    // Clear IT
    U8 speed ;
    U32 value , Un , Zero ;
    Un = 0x40 ;
    Zero = 0x0 ;
    speed = getSpeed( theSlave -> pSpecsmaster ) ;
    value = Un | speed ;
    U32 ctrlValue;
    ctrlValue = SpecsmasterCtrlRead(theSlave -> pSpecsmaster  ); //MTQ pour ne pas effacer les autres bits 
    SpecsmasterCtrlWrite( theSlave -> pSpecsmaster , (ctrlValue& 0xFFFFFFF8) |value ) ;
    value = Zero | speed ;
    SpecsmasterCtrlWrite( theSlave -> pSpecsmaster , (ctrlValue& 0xFFFFFFF8) |value ) ;
  } else theError = SpecsTimeout ;
  return ApiSuccess;
  //MTQ   return theError ;
}

//===========================================================================
// Create Semaphore (Linux)
//===========================================================================
bool specsCreateSemaphore( bool * semExists , int * semId , 
                           std::string inputKey ,
                           int secondaryKey , int nSem , 
                           std::map< SPECSMASTER * , int > * theMap , 
                           SPECSMASTER * theMasterCard ) {
  key_t theKey = ftok( inputKey.c_str() , secondaryKey ) ;

  // Check if semaphore already exist
  *semExists = true ;
  *semId = semget( theKey , nSem , 0666 ) ;
  if ( -1 == *semId ) {
    *semExists = false ;
    *semId = semget( theKey , nSem , 0666 | IPC_CREAT ) ;
    if ( -1 == *semId ) {
      perror( "Semaphore creation\n" ) ;
      return false ;
    }
  }
  
  std::pair< std::map< SPECSMASTER * , int >::iterator , bool > res = 
     theMap -> insert( std::make_pair( theMasterCard , *semId ) ) ;
  if ( ! res.second ) return false ;
  return true ;
}
