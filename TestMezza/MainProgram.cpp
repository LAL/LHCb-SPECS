// $Id: MainProgram.cpp,v 1.20 2007/10/03 12:43:47 robbe Exp $
// Include files 
#include "SpecsUser.h"
#include <functional>
#include "TestCommands.h"
#include <iostream>
#include <signal.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#ifdef WIN32
#include "time.h"
#else
#include <ctime>
#endif

// List of tests
// 0 - R/W Registre 3 Mezzanine
// 1 - R/W RAM bus parallel
// 2 - R/W RAM I2C output Select 0
// 3 - R/W RAM I2C output Select 1
// 4 - R/W RAM I2C output Select 2
// 5 - R/W RAM I2C output Select 3
// 6 - R/W RAM I2C output Select 4
// 7 - R/W RAM I2C output Select 5
// 8 - R/W RAM I2C output Select 6
// 9 - R/W RAM I2C output Select 7
// 10 - R/W RAM I2C output Select 8
// 11 - R/W RAM I2C output Select 9
// 12 - R/W RAM I2C output Select 10
// 13 - R/W RAM I2C output Select 11
// 14 - R/W EEPROM Output Select 12
// 15 - R/W RAM I2C output Select 13
// 16 - R/W Registre Test DCU (output Select 14)
// 17 - R/W RAM I2C output Select 15
// 18 - R DCU In0 (with DAQ set to 2000)
// 19 - R DCU In1
// 20 - R DCU In2
// 21 - R DCU In3
// 22 - R DCU In4
// 23 - R DCU In5
// 24 - R/W I/O (Needs 2 Mezzanines)
// 25 - R/W DMA RAM
// 26 - JTAG output Select 0
// 27 - JTAG output Select 1
// 28 - JTAG output Select 2
// 29 - JTAG output Select 3
// 30 - JTAG output Select 4
// 31 - JTAG output Select 5
// 32 - JTAG output Select 6
// 33 - JTAG output Select 7
// 34 - JTAG output Select 8
// 35 - JTAG output Select 9
// 36 - JTAG output Select 10
// 37 - JTAG output Select 11
// 38 - USER Interrupt
// 40 - Test Broadcast I2C
// 41 - IO Interrupt 0
// 42 - IO Interrupt 1
// 43 - IO Interrupt 2
// 44 - IO Interrupt 3
// 45 - IO Interrupt 4
// 46 - IO Interrupt 5
// 47 - IO Interrupt 6
// 48 - IO Interrupt 7


SpecsTestFunction * test ;
SPECSMASTER theMaster , theMaster2 ;


int main( int argc , char ** argv ) {

  srand( (unsigned) time( NULL ) ) ;

  if ( ( 6 != argc ) && ( 7 != argc ) ) {
    printf( " \n\n " ) ;
    printf( " Usage <Card> <Master> <Slave> <Test> <NEvents> " ) ;
    printf( "(<Slave #2>)\n" ) ;
    printf( "\n\n\n" ) ;
    exit(0);
  }

  int cardIndex   = atoi( argv[ 1 ] ) ;
  U8  masterIndex = atoi( argv[ 2 ] ) ;
  U8  slaveIndex  = atoi( argv[ 3 ] ) ;
  int testNumber  = atoi( argv[ 4 ] ) ;
  unsigned int nEvents     = atoi( argv[ 5 ] ) ;

  if ( ( ( 24 == testNumber ) || ( 38 == testNumber ) || ( testNumber > 40 ) ) 
       && ( 6 == argc ) ) {
    printf( "Needs additional argument !\n" ) ;
    exit( 0 ) ;
  }

  U8 slaveIndex2 ;
  if ( ( 24 == testNumber ) || ( 38 == testNumber ) || ( testNumber > 40 ) ) 
    slaveIndex2 =  atoi( argv[ 6 ] ) ;

  // MASTER
  DEVICE_INVENT deviceList[ MAX_CARD ] ;
  U8 nSpecs = specs_master_card_select( deviceList ) ;
  printf( "\n\n" ) ;
  
  if ( 0 == nSpecs ) {
    printf( "No SPECS MASTER Card found \n" ) ;
    exit( 0 ) ;
  }
  
  for ( unsigned int ii = 0 ; ii < nSpecs ; ++ii ) {
    if ( deviceList[ ii ].StDevLoc.SlotNumber == cardIndex ) {
      cardIndex = ii ;
      break ;
    }
  }
  
  SpecsError theSpError = 
    specs_master_open( deviceList[ cardIndex ] , masterIndex , &theMaster ) ;
  
  if ( SpecsSuccess != theSpError ) {
    printf( " Error Opening Specs with code = %X \n" , theSpError ) ;
    exit( 0 ) ;
  }
  
  if ( ( 24 == testNumber ) || ( 38 == testNumber ) ) {
    
    specs_master_card_select( deviceList ) ;
    
    if ( 1 == slaveIndex ) cardIndex = 0 ;
    else if ( 2 == slaveIndex ) cardIndex = 0 ;
    else if ( 3 == slaveIndex ) cardIndex = 2 ;
    else if ( 4 == slaveIndex ) cardIndex = 0 ;
    
    for ( unsigned int ii = 0 ; ii < nSpecs ; ++ii ) {
      if ( deviceList[ ii ].StDevLoc.SlotNumber == cardIndex ) {
        cardIndex = ii ;
        break ;
      }
    }
    
    if ( 1 == slaveIndex ) masterIndex = 2 ;
    else if ( 2 == slaveIndex ) masterIndex = 1 ;
    else if ( 3 == slaveIndex ) masterIndex = 4 ;
    else if ( 4 == slaveIndex ) masterIndex = 1 ;
    
    theSpError = 
      specs_master_open( deviceList[ cardIndex ] , masterIndex , &theMaster2 ) ;
  
    if ( SpecsSuccess != theSpError ) {
      printf( " Error Opening second Specs with code = %X : slave %d, master %d, card %d\n" , 
              theSpError , slaveIndex , masterIndex , cardIndex ) ;
    //    ConsoleFinalize() ;
      exit( 0 ) ;
    }
  }

  printf( " Successfully opened Master : \n" ) ;
  printf(
         "    Vendor Id: %X, Device Id: %X, Bus Number: %d, Slot Number: %d",
         deviceList[ cardIndex ].StDevLoc.VendorId,
         deviceList[ cardIndex ].StDevLoc.DeviceId,
  deviceList[ cardIndex ].StDevLoc.BusNumber,
         deviceList[ cardIndex ].StDevLoc.SlotNumber ) ;
  printf( ", Serial Number: %d\n" ,
          specs_master_serialNumber( &theMaster ) ) ;
  
  printf( " Version of the library: %d.%d\n" ,
          specs_major_version() , specs_minor_version() ) ;
  specs_master_softreset( &theMaster ) ;
  
  specs_master_setspeed( &theMaster , 0 ) ;
  specs_master_unmaskchecksum( &theMaster ) ;

  SPECSSLAVE theSlave , theSlaveRec ;
  specs_slave_open( &theMaster , slaveIndex , &theSlave ) ;
  printf( "Opened mezzanine %d\n" , slaveIndex ) ;
  
  if ( ( 24 == testNumber ) || ( 38 == testNumber ) || ( testNumber > 40 ) ) {
    specs_slave_open( &theMaster2 , slaveIndex2 , &theSlaveRec ) ;
  }
    
  printf( "\n\n" );

  bool result ;
  unsigned int nErrors = 0 ;


  switch ( testNumber ) {
  case 0: test = new MezzanineRegister() ; break ;
  case 1: test = new Parallel() ; break ;
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
  case 13:
  case 15:
  case 17:
    specs_register_write( &theSlave , 0x6 , 0 ) ;
    specs_register_write( &theSlave , 0x4 , 0 ) ;
    test = new I2c( testNumber - 2 ) ; 
    specs_master_setspeed( &theMaster , 3 ) ;
    break ;
  case 16:
    specs_master_setspeed( &theMaster , 3 ) ;
    test = new DCUTestReg() ; break ;
  case 14: test = new EEPROM() ; break ;
  case 18: 
  case 19:
  case 20:
  case 21:
  case 22:
  case 23:
    specs_dcu_initialize( &theSlave ) ;
    specs_dcu_set_LIR( &theSlave ) ;
    test = new DCUDaq( testNumber - 18 ) ;
    break;
  case 24:
    // 0 == emit, 1 == receive
    specs_register_write( &theSlave , 0x3 , 0xFFFF ) ;
    specs_register_write( &theSlave , 0x2 , 0xFFFF ) ;
    specs_register_write( &theSlaveRec , 0x3 , 0x00 ) ;
    specs_register_write( &theSlaveRec , 0x2 , 0x00 ) ;
    test = new IOReg( &theSlaveRec ) ;
    break ;
  case 25: test = new DMA() ; break ;
  case 26:
  case 27:
  case 28:
  case 29:
  case 30:
  case 31:
  case 32:
  case 33:
  case 34:
  case 35:
  case 36:
  case 37:
    specs_register_write( &theSlave , 0x6 , 
                          (int) pow( (double) 2 , testNumber - 26 ) ) ;
    specs_register_write( &theSlave , 0x4 , 0x8 ) ;
    specs_master_setspeed( &theMaster , 0 ) ;
    test = new JTAG( testNumber - 26 ) ;
    break ;
  case 38:
    specs_register_write( &theSlave , ConfRegOutLSBReg , 0x0 ) ;
    specs_register_write( &theSlave , ConfRegOutMSBReg , 0x0 ) ;
    specs_register_write( &theSlaveRec , ConfRegOutLSBReg , 0xFFFF ) ;
    specs_register_write( &theSlaveRec , ConfRegOutMSBReg , 0xFFFF ) ;
    test = new UserInterruptTest( &theSlaveRec ) ;
    break ;
  case 39: test = new DMA2() ; break ;

  case 41:
  case 42:
  case 43:
  case 44:
  case 45:
  case 46:
  case 47:
  case 48:
    specs_register_write( &theSlave , ConfRegOutLSBReg , 0x0 ) ;
    specs_register_write( &theSlave , ConfRegOutMSBReg , 0x0 ) ;
    specs_register_write( &theSlaveRec , ConfRegOutLSBReg , 0xFFFF ) ;
    specs_register_write( &theSlaveRec , ConfRegOutMSBReg , 0xFFFF ) ;
    test = new IOInterruptTest( &theSlaveRec , testNumber - 41 ) ;
    break ;    
    
  default:
    printf( "Unknown test number: %d\n" , testNumber ) ;
    //    ConsoleFinalize() ;
    exit(0);
    break ;
  }

  if ( 14 == testNumber ) {
    if ( nEvents > 10 ) {
      printf( "Limit number of accesses to 10 for EEPROM !\n") ;
      nEvents = 10 ;
    }
  }  

  printf( "Start Processing %d events\n\n\n" , nEvents ) ;

  // Activate Checksum Error
  if ( ( 24 != testNumber ) && ( 14 != testNumber ) && ( testNumber < 41 ) ) {
    specs_register_write( &theSlave , IntDefineVect , ChecksumBit | 
                          I2CAckInterruptBit | UserInterruptBit ) ;
    
    U16 InterruptVector ;
    specs_register_read( &theSlave , 0x7 , &InterruptVector ) ;
  } else if ( testNumber > 40 ) { 
    specs_register_write( &theSlave , IntDefineVect , ChecksumBit | 
                          I2CAckInterruptBit | IO0InterruptBit | 
                          IO1InterruptBit | IO2InterruptBit | 
                          IO3InterruptBit | IO4InterruptBit | 
                          IO5InterruptBit | IO6InterruptBit | 
                          IO7InterruptBit ) ;
    
    U16 InterruptVector ;
    specs_register_read( &theSlave , 0x7 , &InterruptVector ) ;
  } else {
    specs_register_write( &theSlave , IntDefineVect , ChecksumBit | 
                          UserInterruptBit ) ;
  }

  for ( unsigned int i = 0 ; i < nEvents ; ++i ) {
    result = (*test)( &theSlave ) ;
    if ( 0 == i % 10000 ) 
      printf( "   Processing event ...... %d\n" , i ) ;
    if ( ! result ) { 
      printf( "Error at event %d \n", i ) ;
      specs_master_softreset( &theMaster ) ;
      nErrors++ ;
    }
  }

  printf( "\n\n Number of Errors = %d \n" , nErrors ) ;

  delete test ;

  specs_master_close( &theMaster ) ;

  //  ConsoleFinalize() ;
}

//void INThandler( int sig ) {
//  signal( sig , SIG_IGN ) ;
//
//  if ( 0 != test ) delete test ;
//
//  specs_master_close( &theMaster ) ;
//  
//  ConsoleFinalize() ;
//
//  exit( 0 ) ;
//}

