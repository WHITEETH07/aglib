/*
  pdremote_c.cpp: Class for handling Remote Process Data

  (C) Copyright 2009 - 2014 by OSB AG and developing partners

  See the repository-log for details on the authors and file-history.
  (Repository information can be found at <http://isoaglib.com/download>)

  Use, modification and distribution are subject to the GNU General
  Public License with exceptions for ISOAgLib. (See accompanying
  file LICENSE.txt or copy at <http://isoaglib.com/download/license>)
*/
#include "pdremote_c.h"
#include "connectedpdremote_c.h"

#if defined(_MSC_VER)
#pragma warning( disable : 4355 )
#endif


namespace __IsoAgLib {

  // @todo Add Stop methods using these values... (or are they TC/DL specific?)
  static const int32_t sci32_stopValTimeInterval = 0;
  static const int32_t sci32_stopValDistanceInterval = 0;
  static const int32_t sci32_stopValOnChange = 0;
  static const int32_t sci32_stopValThresholdMaximum = -2147483647L; // Standard specifies (-2^31+1) instead of (-2^31)
  static const int32_t sci32_stopValThresholdMinimum = 2147483647L;


  ConnectedPd_c &
  PdRemote_c::createConnectedPd( PdConnection_c &connection )
  {
    return *new ConnectedPdRemote_c( connection, *this );
  }


  void
  PdRemote_c::init( uint16_t ddi, uint16_t element, MeasurementHandler_c* handler )
  {
    PdBase_c::init( ddi, element );

    m_remoteValue.init( handler );
  }


  void PdRemote_c::requestValue()
  {
    for( PdBase_c::ConnectedPds_t::iterator iter = connectedPds().begin(); iter != connectedPds().end(); ++iter )
      ( *iter )->sendMsg( IsoAgLib::ProcData::requestValue, 0 );
  }
  
  
  void PdRemote_c::sendSetpoint( int32_t value )
  {
    for( PdBase_c::ConnectedPds_t::iterator iter = connectedPds().begin(); iter != connectedPds().end(); ++iter )
      ( *iter )->sendMsg( IsoAgLib::ProcData::setValue, value );
  }
  

  void PdRemote_c::startMeasurement( IsoAgLib::ProcData::MeasurementCommand_t cmd, int32_t value )
  {
    for( PdBase_c::ConnectedPds_t::iterator iter = connectedPds().begin(); iter != connectedPds().end(); ++iter )
      ( *iter )->sendMsg( static_cast< IsoAgLib::ProcData::CommandType_t >( cmd ), value );
  }

}
