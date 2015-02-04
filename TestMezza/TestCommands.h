// $Id: TestCommands.h,v 1.7 2007/05/29 16:07:35 robbe Exp $
#ifndef TESTBOARD_TESTCOMMANDS_H 
#define TESTBOARD_TESTCOMMANDS_H 1

// Include files
#include <functional>
#include "SpecsUser.h"

class SpecsTestFunction : public std::unary_function< SPECSSLAVE * , bool > {
public:
  virtual ~SpecsTestFunction() { ; } ;
  virtual bool operator() ( SPECSSLAVE * theSlave ) = 0 ;
};

class MezzanineRegister : public SpecsTestFunction {
public:
  virtual bool operator( ) ( SPECSSLAVE * theSlave ) ;
};

class Parallel : public SpecsTestFunction {
public:
  virtual bool operator() ( SPECSSLAVE * theSlave ) ;
};

class I2c : public SpecsTestFunction {
public:
  I2c( const U8 outputSelect ) : m_outputSelect( outputSelect ) { };
  virtual bool operator() ( SPECSSLAVE * theSlave ) ;
private:
  U8 m_outputSelect ;
};

class DCUTestReg : public SpecsTestFunction {
public:
  virtual bool operator() ( SPECSSLAVE * theSlave ) ;
};

class EEPROM : public SpecsTestFunction {
public:
  virtual bool operator() ( SPECSSLAVE * theSlave ) ;
};

class DCUDaq : public SpecsTestFunction {
public:
  DCUDaq( const U8 channel ) : m_channel( channel ) { } ;
  virtual bool operator() ( SPECSSLAVE * theSlave ) ;
private:
  U8 m_channel ;
};

class IOReg : public SpecsTestFunction {
public:
  IOReg( SPECSSLAVE * theSlave ) : m_slave( theSlave ) { } ;
  virtual bool operator() ( SPECSSLAVE * theSlave ) ;
private:
  SPECSSLAVE * m_slave ;
}; 

class UserInterruptTest : public SpecsTestFunction {
public:
  UserInterruptTest( SPECSSLAVE * theSlave ) : m_slave( theSlave ) { } ;
  virtual bool operator() ( SPECSSLAVE * theSlave ) ;
private:
  SPECSSLAVE * m_slave ;
};  

class IOInterruptTest : public SpecsTestFunction {
public:
  IOInterruptTest( SPECSSLAVE * theSlave , int intNumber ) : 
    m_slave( theSlave ) , m_intNumber( intNumber ) { } ;
  virtual bool operator() ( SPECSSLAVE * theSlave ) ;
private:
  SPECSSLAVE * m_slave ;
  int m_intNumber ;
};  

class DMA : public SpecsTestFunction {
public:
  virtual bool operator() ( SPECSSLAVE * theSlave ) ;
};

class DMA2 : public SpecsTestFunction {
public:
  virtual bool operator() ( SPECSSLAVE * theSlave ) ;
};  

class JTAG : public SpecsTestFunction {
public:
  JTAG( const U8 outputSelect ) : m_outputSelect( outputSelect ) { } ;
  virtual bool operator() ( SPECSSLAVE * theSlave ) ;
private:
  U8 m_outputSelect ;
};
#endif // TESTBOARD_TESTCOMMANDS_H
