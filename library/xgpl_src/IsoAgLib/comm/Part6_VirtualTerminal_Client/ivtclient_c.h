/*
  iisoterminal_c.h: central ISO terminal management

  (C) Copyright 2009 - 2012 by OSB AG and developing partners

  See the repository-log for details on the authors and file-history.
  (Repository information can be found at <http://isoaglib.com/download>)

  Use, modification and distribution are subject to the GNU General
  Public License with exceptions for ISOAgLib. (See accompanying
  file LICENSE.txt or copy at <http://isoaglib.com/download/license>)
*/
#ifndef IISO_TERMINAL_H
#define IISO_TERMINAL_H

#include "impl/vtclient_c.h"
#include "ivttypes.h"

namespace IsoAgLib {

class iScheduler_c;

/**
  class to define an interface class for the storage of Preferred ISOVT. Users can derive from
  and implement the load and store functions to their needs.
 */
class iVtClientDataStorage_c {
  public:
    /** Application needs to load the stored preferred ISOVT iIsoName_c and boottime.
        @param arc_isoname saved isoname. Set to Undefined if not known
        @param arui8_boottime_s saved boottime in second. Set to 0 or 0xFF if not known
      */
    virtual void loadPreferredVt( IsoAgLib::iIsoName_c &arc_isoname, uint8_t &arui8_boottime_s ) = 0;

    /** Application needs to store the preferred ISOVT iIsoName_c and boottime.
        @param arc_isoname isoname to be saved
        @param arui8_boottime_s boottime to be saved, in second
      */
    virtual void storePreferredVt( const IsoAgLib::iIsoName_c &arc_isoname, uint8_t aui8_bootTime) = 0;
};

/**
  central ISO11783 terminal management object

  For how to specify an ISO VT Object Pool please refer to \ref XMLspec .
  */
class iVtClient_c : private __IsoAgLib::VtClient_c {
public:

  /**
    register given object pool for uploading when possible.
  */
  iVtClientConnection_c* initAndRegisterIsoObjectPool (iIdentItem_c& arc_identItem, iVtClientObjectPool_c& arc_pool, const char* apc_versionLabel, IsoAgLib::iVtClientDataStorage_c& apc_claimDataStorage, iVtClientObjectPool_c::RegisterPoolMode_en aen_mode)
  { return VtClient_c::initAndRegisterIsoObjectPool (static_cast<__IsoAgLib::IdentItem_c&>(arc_identItem), arc_pool, apc_versionLabel, apc_claimDataStorage, aen_mode)->toInterfacePointer(); }

  bool deregisterIsoObjectPool (iIdentItem_c& arc_wsMasterIdentItem)
  { return VtClient_c::deregisterIsoObjectPool (arc_wsMasterIdentItem); }

  iVtClientConnection_c& getClientByID (uint8_t ui8_clientIndex)
  { return VtClient_c::getClientByID (ui8_clientIndex).toInterfaceReference(); }

  iVtClientConnection_c* getClientPtrByID (uint8_t ui8_clientIndex)
  { return VtClient_c::getClientPtrByID (ui8_clientIndex)->toInterfacePointer(); }

  bool isAnyVtAvailable() const { return VtClient_c::isAnyVtAvailable(); }

// the following define should be globally defined in the project settings...
#ifdef USE_IOP_GENERATOR_FAKE_VT_PROPERTIES
  void fakeVtProperties (uint16_t aui16_dimension, uint16_t aui16_skWidth, uint16_t aui16_skHeight, uint8_t aui16_colorDepth, uint16_t aui16_fontSizes)
  { VtClient_c::fakeVtProperties (aui16_dimension, aui16_skWidth, aui16_skHeight, aui16_colorDepth, aui16_fontSizes); }
#endif

 private:
  /** allow getIisoTerminalInstance() access to shielded base class.
      otherwise __IsoAgLib::getIsoTerminalInstance() wouldn't be accepted by compiler
    */
  #if defined(PRT_INSTANCE_CNT) && (PRT_INSTANCE_CNT > 1)
  friend iVtClient_c& getIisoTerminalInstance (uint8_t aui8_instance);
  #else
  friend iVtClient_c& getIisoTerminalInstance (void);
  #endif
  friend class iVtClientConnection_c;
};

#if defined(PRT_INSTANCE_CNT) && (PRT_INSTANCE_CNT > 1)
  /** C-style function, to get access to the unique VtClient_c singleton instance
    * if more than one CAN BUS is used for IsoAgLib, an index must be given to select the wanted BUS
    */
  inline iVtClient_c& getIvtClientInstance (uint8_t aui8_instance = 0)
  { return static_cast<iVtClient_c&>(__IsoAgLib::getVtClientInstance(aui8_instance)); }
#else
  /** C-style function, to get access to the unique Process_c singleton instance */
  inline iVtClient_c& getIvtClientInstance (void)
  { return static_cast<iVtClient_c&>(__IsoAgLib::getVtClientInstance()); }
#endif


}
#endif