// $Id: TestCommands.cpp,v 1.23 2007/10/03 15:59:50 robbe Exp $
// Include files

// local
#include "TestCommands.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

bool MezzanineRegister::operator()( SPECSSLAVE * theSlave ) {
  U16 dataWrite , dataRead ;
  SpecsError theError ;

  dataWrite = 
    static_cast< U16 >( ( rand() % 0xFF ) | ( ( rand() % 0xFF ) << 8 ) ) ;
  
  theError = specs_register_write( theSlave , 0x3 , dataWrite ) ;
  
  if ( theError != SpecsSuccess ) { 
    printf( "Error write with code %X.\n" , theError ) ;
    return false ;
  }
  
  theError = specs_register_read( theSlave , 0x3 , &dataRead ) ;
  
  if ( theError != SpecsSuccess ) {
    printf( "Error read with code %X.\n" , theError ) ;
    return false ;
  }
  
  if ( dataRead != dataWrite ) { 
    return false ;
  }  
  
  return true ;
}

bool Parallel::operator()( SPECSSLAVE * theSlave ) {
  U16 dataWrite , dataRead ;
  U8 address = static_cast< U8 >( rand() % 0xFF ) ;
  SpecsError theError ;
  
  dataWrite = 
    static_cast< U16 >( ( rand() % 0xFF ) | ( ( rand() % 0xFF ) << 8 ) ) ;
  theError = specs_parallel_write( theSlave , address , 1 , &dataWrite ) ;
  
  if ( theError != SpecsSuccess ) { 
    printf( "Erreur write %X\n" , theError ) ;
    return false ;
  }
  
  theError = specs_parallel_read( theSlave , address , 1 , &dataRead ) ;

  if ( theError != SpecsSuccess ) { 
    printf( "Erreur read %X\n" , theError ) ;
    return false ;
  }
  
  if ( dataRead != dataWrite ) { 
    return false ;
  }
  
  return true ;
}

bool I2c::operator() ( SPECSSLAVE * theSlave ) {
  U8 dataWrite[ 256 ] ;
  U8 dataRead[ 256 ] ;
  SpecsError theError ;
  
  U8 nValues(0) , ramAdd ;
  
  do {
    nValues = static_cast< U8 >( rand() % 0x3F ) ;
  } while ( nValues == 0 ) ;

  //  nValues = 20 ;

  ramAdd = static_cast< U8 >( rand() % 0x3F ) ;
  dataWrite[ 0 ] = ramAdd ;
  
  for ( int k = 1 ; k < nValues + 1 ; ++k ) {
    dataWrite[ k ] = static_cast< U8 >( rand() % 0xFF ) ;
  }
  
  theError = specs_i2c_write( theSlave , m_outputSelect , 0x51 ,  nValues + 1 ,
                              dataWrite ) ;

  if ( SpecsSuccess != theError ) {
    printf( "Error write I2c#1 with code %X\n" , theError ) ;
    return false ;
  }
  
  theError = specs_i2c_write( theSlave , m_outputSelect , 0x51 , 1 , 
                              dataWrite ) ;

  if ( SpecsSuccess != theError ) { 
    printf( "Error write I2c#2 with code %X\n" , theError ) ;
    return false ;
  }

  theError = specs_i2c_read( theSlave , m_outputSelect , 0x51 , 
                             nValues , dataRead ) ;

  if ( SpecsSuccess != theError ) { 
    printf( "Error read with code %X\n" , theError ) ;
    return false ;
  }

  for ( unsigned int k = 0 ; k < nValues ; ++k ) {
    if ( dataWrite[ k + 1 ] != dataRead[ k ] ) {
      printf( "Data error %d  %X   %X  \n" , k , dataWrite[k+1] , dataRead[k] ) ;
      return false ;
    }
  }

  return true ;
}

bool DCUTestReg::operator()( SPECSSLAVE * theSlave ) {
  U8 dataWrite , dataRead ;
  SpecsError theError ;
  //usleep( 10000 ) ;
  dataWrite = static_cast< U8 >( rand() % 0xFF ) ;
  theError = specs_dcu_register_write( theSlave , DcuTREG , dataWrite ) ;
  
  if ( theError != SpecsSuccess ) {
    std::cout << "Erreur write = " << (int) theError << std::endl ;
    return false ;
  }
  
  theError = specs_dcu_register_read( theSlave , DcuTREG , &dataRead ) ;

  if ( theError != SpecsSuccess ) { 
    std::cout << "Erreur read = " << (int) theError << std::endl ;
    return false ;
  }
  
  if ( dataRead != dataWrite ) { 
    std::cout << "Mismatch " << (int) dataRead << " " << (int) dataWrite
              << std::endl ;
    return false ;
  }
  
  return true ;
}

bool EEPROM::operator() ( SPECSSLAVE * theSlave ) {
  U8 dataWrite[ 256 ] ;
  U8 dataRead[ 256 ] ;
  SpecsError theError = SpecsSuccess ;
  
  U8 nValues( 0 ) , ramAdd( 0 ) , pageNumber( 0 ) ;

  do {
    pageNumber = static_cast< U8 >( rand() % 0x7F ) ;
  } while ( pageNumber == 0 ) ;
  
  ramAdd = static_cast< U8 >( rand() % 0x1F ) ;
  
  do {
    nValues = static_cast< U8 >( rand() % 0x1F ) ;
  } while ( ( nValues == 0 ) || ( nValues == 25 ) );
  
   for ( int k = 0 ; k < nValues ; ++k ) {
     dataWrite[ k ] = static_cast< U8 >( rand() % 0xFF ) ;
   }
    
   theError = specs_slave_write_eeprom( theSlave , pageNumber , ramAdd ,
                                        nValues , dataWrite ) ;

   if ( SpecsSuccess != theError ) { 
     printf( "Error write with code %X\n" , theError ) ;
     return false ;
   }
  
  theError = specs_slave_read_eeprom( theSlave , pageNumber , ramAdd , 
                                      nValues , dataRead ) ;

  if ( SpecsSuccess != theError ) { 
    printf( "Error read with code %X\n" , theError ) ;    
    return false ;
  }
   
  for ( unsigned int k = 0 ; k < nValues ; ++k ) {
    if ( dataWrite[ k ] != dataRead[ k ] ) { 
      printf( "Mismatch: %d , %d , %X , %X \n" , nValues , k , 
              dataWrite[k] , dataRead[ k ] ) ;
      return false ;
    }
  }

  return true ;
}

bool DCUDaq::operator( ) ( SPECSSLAVE * theSlave ) {
  //  U8 data( 0 ) ;
  // U8 data[50] , dataOut[50];
  // U16 val ;
  double cVal1 , cVal2 ;

  SpecsError theError = 
//    specs_dcu_acquire( theSlave , m_channel , &data , &cVal ) ;
    specs_slave_dcuconstant( theSlave , 0 , 0 , &cVal1 , &cVal2 ) ;
  if ( theError != SpecsSuccess ) { 
    printf( "Error with type %X\n" , theError ) ;
    return false ;
  }
  
  //if ( abs( data - 1963 ) > 10 ) { 
//    std::cout << "Value read = " << (int) data << std::endl ;
    //return false ;
  //}
  

  return true ;
}

bool IOReg::operator()( SPECSSLAVE * theSlave ) {
  U16 dataLSBWrite , dataLSBRead ;
  U16 dataMSBWrite , dataMSBRead ;
  
  SpecsError theError ;

  dataLSBWrite = 
    static_cast< U16 >( ( rand() % 0xFF ) | ( ( rand() % 0xFF ) << 8 ) ) ;
  dataMSBWrite = 
    static_cast< U16 >( ( rand() % 0xFF ) | ( ( rand() % 0xFF ) << 8 ) ) ;
  
  theError = specs_register_write( theSlave , 0x1 , dataLSBWrite ) ;
  theError |= specs_register_write( theSlave , 0x0 , dataMSBWrite ) ;
  
  if ( theError != SpecsSuccess ) { 
    printf( "Error writing %X\n" , theError ) ;
    return false ;
  }  
  
  theError = specs_register_read( m_slave , 0x1 , &dataLSBRead ) ;
  theError |= specs_register_read( m_slave , 0x0 , &dataMSBRead ) ;
  
  if ( theError != SpecsSuccess ) {
    printf( "Error reading %X\n" , theError ) ;
    return false ;
  }
  
  if ( dataMSBRead != dataMSBWrite ) return false ;
  if ( dataLSBRead != dataLSBWrite ) return false ;
  
  return true ;
}


typedef struct _SpecsTestData {
  SPECSSLAVE * theSlave ;
  int intNumber ;
} SPECSTESTDATA , *PSPECSTESTDATA ;

#ifdef WIN32
DWORD WINAPI UserInterruptThread( LPVOID lpParam ) {
  PSPECSTESTDATA theData ;
  theData = (PSPECSTESTDATA) lpParam ;
  
  SPECSSLAVE * theSlave = theData->theSlave ;
  U16 intVect ;
  SpecsError theError = specs_slave_waitForInterrupt( theSlave , 
                                                      UserInterruptBit , 
                                                      10 , 
                                                      &intVect ) ;  
  if ( theError != SpecsSuccess ) {
    printf( "Error user interrupt %X\n" , theError ) ;
    HeapFree( GetProcessHeap() , 0 , theData ) ;
    ExitThread( 1 ) ;
  }
  if ( 0 == ( intVect & UserInterruptBit ) ) {
    printf( "Not a user interrupt: %X\n" , intVect ) ;
    HeapFree( GetProcessHeap() , 0 , theData ) ;
    ExitThread( 1 ) ;
  }

  HeapFree( GetProcessHeap() , 0 , theData ) ;
  ExitThread( 0 ) ;
}
#else
int theStatus ;
void * UserInterruptThread( void * arg ) {
  SPECSSLAVE * theSlave = (SPECSSLAVE *) arg ;
  U16 intVect ;
  theStatus = 0 ;
  SpecsError theError = specs_slave_waitForInterrupt( theSlave ,
    UserInterruptBit , 10 , &intVect ) ;
  if ( theError != SpecsSuccess ) {
    printf( "Error user interrupt %X\n" , theError ) ;
    theStatus = 0 ;
  } else if ( 0 == ( intVect & UserInterruptBit ) ) {
    printf( "Not a user interrupt: %X\n" , intVect ) ;
    theStatus = 0 ;
  } else {
    theStatus = 1 ;
  }
  pthread_exit( (void *) &theStatus ) ;
}
#endif

bool UserInterruptTest::operator()( SPECSSLAVE * theSlave ) {
#ifdef WIN32
  SpecsError theError;
  PSPECSTESTDATA param ;
  param = (PSPECSTESTDATA) HeapAlloc( GetProcessHeap() , 
                                      HEAP_ZERO_MEMORY ,
                                      sizeof( SPECSTESTDATA ) ) ;
  param->theSlave = theSlave ;

  HANDLE th = CreateThread( NULL , 0 , UserInterruptThread ,
                            param , 0 , NULL ) ;
  
  if ( NULL == th ) {
    printf( "Cannot create thread.\n" ) ;
    return false ;
  }
  theError = specs_register_write( m_slave , 0x1 , 0x0 ) ;

  theError |= specs_register_write( m_slave , 0x1 , 0x1 ) ;

  theError |= specs_register_write( m_slave , 0x1 , 0x0 ) ;

  if ( theError != SpecsSuccess ) 
    printf( "Error write IO %X\n" , theError ) ;
 
  WaitForSingleObject( th , INFINITE ) ;

  DWORD exitcode ;
  BOOL result = GetExitCodeThread( th , &exitcode ) ;
  if ( 0 == result ) {
    printf( "No thread exit code\n" ) ;
    CloseHandle( th ) ;
    return false ;
  }
  
  CloseHandle( th ) ;

  if ( 0 != exitcode ) return false ;
#else
  pthread_t thread ;
  int iret = pthread_create( &thread , NULL , UserInterruptThread , 
    (void *) theSlave ) ;
  if ( 0 != iret ) {
    printf( "Cannot create thread.\n" ) ;
    return false ;
  } 

  usleep( 100000 ) ;
  
  SpecsError theError = specs_register_write( m_slave , 0x1 , 0x0 ) ;

  theError |= specs_register_write( m_slave , 0x1 , 0x1 ) ;

  theError |= specs_register_write( m_slave , 0x1 , 0x0 ) ;

  if ( theError != SpecsSuccess ) 
    printf( "Error write IO %X\n" , theError ) ;
  
  void * retVal ;
  pthread_join( thread , &retVal ) ;
  
  if ( 0 == *((int *) ( retVal )) ) {
    printf( "Error in thread \n" ) ;
    return false ;
  }
#endif
  return true ;
}

#ifdef WIN32
DWORD WINAPI IOInterruptThread( LPVOID lpParam ) {
  PSPECSTESTDATA theData ;
  theData = (PSPECSTESTDATA) lpParam ;
  
  SPECSSLAVE * theSlave = theData->theSlave ;
  int intNumber = theData -> intNumber ;

  U16 intVect ;
  SpecsError theError = specs_slave_waitForInterrupt( theSlave , 
                                                      intNumber , 
                                                      10 , 
                                                      &intVect ) ;  
  if ( theError != SpecsSuccess ) {
    printf( "Error IO interrupt %X\n" , theError ) ;
    HeapFree( GetProcessHeap() , 0 , theData ) ;
    ExitThread( 1 ) ;
  }
  if ( 0 == ( intVect & intNumber ) ) {
    printf( "Not a IO interrupt: %X\n" , intVect ) ;
    HeapFree( GetProcessHeap() , 0 , theData ) ;
    ExitThread( 1 ) ;
  }

  HeapFree( GetProcessHeap() , 0 , theData ) ;
  ExitThread( 0 ) ;
}
#else
int theIOStatus ;
pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER; 
void * IOInterruptThread( void * arg ) {
  PSPECSTESTDATA theData ;
  theData = (PSPECSTESTDATA) arg ;
  int intNumber = theData -> intNumber ;
  SPECSSLAVE * theSlave = theData -> theSlave ;
  
  U16 intVect ;
  theStatus = 0 ;
//  pthread_mutex_unlock( &fastmutex ) ;
  SpecsError theError = specs_slave_waitForInterrupt( theSlave ,
    intNumber , 10 , &intVect  ) ;
  
  if ( theError != SpecsSuccess ) {
    printf( "Error IO interrupt %X\n" , theError ) ;
    theStatus = 0 ;
  } else if ( 0 == ( intVect & intNumber ) ) {
    printf( "Not a IO interrupt: %X\n" , intVect ) ;
    theStatus = 0 ;
  } else {
    theStatus = 1 ;
  }
  pthread_exit( (void *) &theStatus ) ;
}
#endif

bool IOInterruptTest::operator()( SPECSSLAVE * theSlave ) {
#ifdef WIN32
  SpecsError theError ;
  PSPECSTESTDATA param ;
  param = (PSPECSTESTDATA) HeapAlloc( GetProcessHeap() , 
                                      HEAP_ZERO_MEMORY ,
                                      sizeof( SPECSTESTDATA ) ) ;
  param->theSlave = theSlave ;
  param->intNumber = 1 << ( m_intNumber + 3 ) ;

  HANDLE th = CreateThread( NULL , 0 , IOInterruptThread ,
                            param , 0 , NULL ) ;

  if ( NULL == th ) {
    printf( "Cannot create thread.\n" ) ;
    return false ;
  }

  theError = specs_register_write( m_slave , 0x0 , 0xFFFF ) ;
  
  theError |= specs_register_write( m_slave , 0x0 , 
                                    (U16) ~(1 << ( m_intNumber + 8 ) ) ) ;
  
  theError |= specs_register_write( m_slave , 0x0 , 0xFFFF ) ;  
  
  if ( theError != SpecsSuccess ) 
    printf( "Error write IO %X\n" , theError ) ;
 
  WaitForSingleObject( th , INFINITE ) ;

  DWORD exitcode ;
  BOOL result = GetExitCodeThread( th , &exitcode ) ;
  if ( 0 == result ) {
    printf( "No thread exit code\n" ) ;
    CloseHandle( th ) ;
    return false ;
  }
  
  CloseHandle( th ) ;

  if ( 0 != exitcode ) return false ;
#else
  pthread_t thread ;
  SPECSTESTDATA param ;
  param.theSlave = theSlave ;
  param.intNumber = 1 << ( m_intNumber + 3 ) ;
  
  pthread_mutex_lock( &fastmutex ) ;
  
  int iret = pthread_create( &thread , NULL , IOInterruptThread , 
    (void *) &param ) ;
  if ( 0 != iret ) {
    printf( "Cannot create thread.\n" ) ;
    return false ;
  }
  
  pthread_mutex_lock( &fastmutex ) ;
  
  SpecsError theError = specs_register_write( m_slave , 0x0 , 0xFFFF ) ;
  
  theError |= specs_register_write( m_slave , 0x0 , 
                                    (U16) ~(1 << ( m_intNumber + 8 ) ) ) ;
  
  theError |= specs_register_write( m_slave , 0x0 , 0xFFFF ) ;
  
  pthread_mutex_unlock( &fastmutex ) ;
  
  if ( theError != SpecsSuccess ) 
    printf( "Error write IO %X\n" , theError ) ;
  
  void * retVal ;
  pthread_join( thread , &retVal ) ;
  
  if ( 0 == *((int *) ( retVal )) ) {
    printf( "Error in thread \n" ) ;
    return false ;
  }
#endif
  return true ;
}

bool DMA::operator() ( SPECSSLAVE * theSlave ) {
  U16 dataWrite[ 256 ] ;
  U16 dataRead[ 256 ] ;
  SpecsError theError ;
  
  U8 nValues( 0 ) , ramAdd( 0 ) ;

  ramAdd = static_cast< U8 >( rand() % 0x1F ) ;

  do {
    nValues = static_cast< U8 >( rand() % 0x7F ) ;
  } while ( ( nValues == 1 ) || ( nValues == 0 ) ) ;

  for ( int k = 0 ; k < nValues ; ++k ) {
    dataWrite[ k ] = 
      static_cast< U16 >( ( rand() % 0xFF ) | ( ( rand() % 0xFF ) << 8 ) ) ;
  }

  theError = specs_parallel_write( theSlave , ramAdd , nValues , 
                                   dataWrite ) ;

  if ( SpecsSuccess != theError ) { 
    std::cout << " Error write = " << std::hex << theError << std::endl ;
    return false ;  
  }

  theError = specs_parallel_read( theSlave , ramAdd , nValues , dataRead ) ;

  if ( SpecsSuccess != theError ) { 
    std::cout << " Error read = " << std::hex << theError << std::endl ;
    return false ;
  } 
   
  for ( unsigned int k = 0 ; k < nValues ; ++k ) {
    if ( dataWrite[ k ] != dataRead[ k ] ) { 
      std::cout << "Data mismatch " << std::hex << 
        k << " " << (int) dataWrite[ k ] 
                << " " << (int) dataRead[ k ] << std::endl ;
      return false ;
    }
  }

  return true ;
}


bool DMA2::operator() ( SPECSSLAVE * theSlave ) {
  U16 dataWrite[ 256 ] ;
  U16 dataRead[ 256 ] ;
  SpecsError theError ;

  static int n = 0 ;
  
  n = n + 1 ;

  U8 nValues( 0 ) , ramAdd( 0 ) ;

  ramAdd = static_cast< U8 >( rand() % 0x1F ) ;
  ramAdd = 0 ;

  do {
    nValues = static_cast< U8 >( rand() % 0x1F ) ;
  } while ( ( nValues == 1 ) || ( nValues == 0 ) ) ;
  nValues =  40 ;

  for ( int k = 0 ; k < nValues ; ++k ) {
    dataWrite[ k ] = 
      static_cast< U16 >( ( rand() % 0xFF ) | ( ( rand() % 0xFF ) << 8 ) ) ;
    dataWrite[ k ] = ( k & 0xFF ) << 8 | ( k & 0xFF ) ;
  }

  theError = specs_parallel_write( theSlave , ramAdd , nValues , 
                                   dataWrite ) ;

  if ( SpecsSuccess != theError ) { 
    std::cout << " Error write = " << std::hex << theError << std::endl ;
    return false ;  
 }

  theError = specs_parallel_read( theSlave , ramAdd , nValues , dataRead ) ;

  if ( SpecsSuccess != theError ) { 
    std::cout << " Error read = " << std::hex << theError << std::endl ;
    exit(0) ;
    return false ;
  } 
   
  for ( unsigned int k = 0 ; k < nValues ; ++k ) {
    if ( dataWrite[ k ] != dataRead[ k ] ) { 
      std::cout << "Data mismatch " << std::hex << 
        k << " " << (int) dataWrite[ k ] 
                << " " << (int) dataRead[ k ] << std::dec << " " 
                << n << std::endl ;
      exit(0);
      return false ;
    }
  }

  return true ;
}

bool JTAG::operator() ( SPECSSLAVE * theSlave ) {
  SpecsError theError ;
  theError = specs_jtag_reset( theSlave , m_outputSelect ) ;
  if ( theError != SpecsSuccess ) { 
    printf( "Error RESET with Code %X\n" , theError ) ;
    return false ;
  }
  
  theError = specs_jtag_idle( theSlave , m_outputSelect ) ;
  
  if ( theError != SpecsSuccess ) {
    printf( "Error IDLE with Code %X\n" , theError ) ;
    return false ;
  }
  
  U8 dataOut[ 256 ] ;
  U8 dataIn[ 256 ] ;
  
  // Read Id Code
  dataIn[ 0 ] = 0x81 ;
  
  theError = specs_jtag_irscan( theSlave , m_outputSelect , dataIn , 
                                dataOut , 8 ) ;

  if ( theError != SpecsSuccess ) {
    printf( "Error IRSCAN with Code %X\n" , theError ) ;
    return false ;
  }
  
  dataIn[ 0 ] = 0x00 ;
  
  theError = specs_jtag_drscan( theSlave , m_outputSelect , dataIn , 
                                dataOut , 64 ) ;

  
  if ( theError != SpecsSuccess ) { 
    printf( "Error DRSCAN with Code %X\n" , theError ) ;
    return false ;
  }
  
  if ( ( dataOut[ 0 ] != 13 ) || ( dataOut[ 1 ] != 96 ) || 
       ( dataOut[ 2 ] != 0 ) || ( dataOut[ 3 ] != 31 ) ) { 
    std::cout << "Mismatch " << (int) dataOut[ 0 ] 
              << "         " << (int) dataOut[ 1 ]
              << "         " << (int) dataOut[ 2 ] 
              << "         " << (int) dataOut[ 3 ] << std::endl ;
    return false ;
  }

  specs_jtag_reset( theSlave , m_outputSelect ) ;

  return true ;
}

