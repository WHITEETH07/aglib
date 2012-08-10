/*
  vtclient_c.h: central ISO terminal management

  (C) Copyright 2009 - 2012 by OSB AG and developing partners

  See the repository-log for details on the authors and file-history.
  (Repository information can be found at <http://isoaglib.com/download>)

  Use, modification and distribution are subject to the GNU General
  Public License with exceptions for ISOAgLib. (See accompanying
  file LICENSE.txt or copy at <http://isoaglib.com/download/license>)
*/
#ifndef VTCLIENT_H
#define VTCLIENT_H

#include <IsoAgLib/isoaglib_config.h>
#include <IsoAgLib/util/impl/singleton.h>
#include <IsoAgLib/driver/can/impl/cancustomer_c.h>
#include <IsoAgLib/scheduler/impl/schedulertask_c.h>
#include <IsoAgLib/comm/Part3_DataLink/impl/canpkgext_c.h>
#include <IsoAgLib/comm/Part5_NetworkManagement/impl/isomonitor_c.h>
#include <IsoAgLib/comm/Part6_VirtualTerminal_Client/ivtclientobjectpool_c.h>
#include <IsoAgLib/comm/Part5_NetworkManagement/impl/identitem_c.h>
#include "vtserverinstance_c.h"
#include "vtclientconnection_c.h"

#include <list>


namespace __IsoAgLib {

class VtClientConnection_c;
class iIdentItem_c;

/** central IsoAgLib terminal management object */
class VtClient_c : public Scheduler_Task_c {
  MACRO_MULTITON_CONTRIBUTION();
public:
  virtual ~VtClient_c() {}

  void init();
  void close();

  VtClientConnection_c* initAndRegisterIsoObjectPool(
    IdentItem_c& apc_wsMasterIdentItem, 
    IsoAgLib::iVtClientObjectPool_c& arc_pool, 
    const char* apc_versionLabel, 
    IsoAgLib::iVtClientDataStorage_c& apc_claimDataStorage, 
    IsoAgLib::iVtClientObjectPool_c::RegisterPoolMode_en aen_mode );

  bool deregisterIsoObjectPool (IdentItem_c& apc_wsMasterIdentItem);

  bool timeEvent(void);

  virtual void updateEarlierAndLatestInterval() {
    updateEarlierAndLatestIntervalDefault();
  }

  /** @todo SOON-241 Remove this - this should only be temporary and a good solution needs to be implemented! */
  void TEMPORARYSOLUTION_setTimePeriod(uint16_t aui16_timePeriod) { setTimePeriod (aui16_timePeriod); }

  /** function that handles incoming can messages */
  virtual bool processMsg( const CanPkg_c& arc_data );

  bool sendCommandForDEBUG(IsoAgLib::iIdentItem_c& apc_wsMasterIdentItem, uint8_t* apui8_buffer, uint32_t ui32_size);


  VtClientConnection_c& getClientByID (uint8_t ui8_clientIndex) { return *mvec_vtClientServerComm[ui8_clientIndex]; }

  VtClientConnection_c* getClientPtrByID (uint8_t ui8_clientIndex) { return (!mvec_vtClientServerComm.empty()) ? mvec_vtClientServerComm[ui8_clientIndex] : NULL; }

  bool isAnyVtAvailable() const { return !ml_vtServerInst.empty(); }
  // is any claimed VT sending VT status
  bool isAnyVtActive( bool mustBePrimary ) const { return (getFirstActiveVtServer( mustBePrimary ) != NULL); }

  /**
   * iterate through all registered VtClientServerCommunication and notify them, maybe they have functions that need that AUX2 input status!
   */
  void notifyAllVtClientsOnAux2InputStatus( const CanPkgExt_c& refc_data ) const;

  /**
   * iterate through all registered VtClientServerCommunication and notify them, maybe they need to process that AUX2 input maintenance message!
   */
  void notifyAllVtClientsOnAux2InputMaintenance( const CanPkgExt_c& refc_data ) const;

  VtServerInstance_c* getFirstActiveVtServer( bool mustBePrimary ) const;
  VtServerInstance_c* getPreferredVtServer(const IsoName_c& aref_prefferedVTIsoName) const;

  ////////////////////////
  // INTERFACE FUNTIONS //
  ////////////////////////
// the following define should be globally defined in the project settings...
#ifdef USE_IOP_GENERATOR_FAKE_VT_PROPERTIES
  void fakeVtProperties (uint16_t aui16_dimension, uint16_t aui16_skWidth, uint16_t aui16_skHeight, uint8_t aui16_colorDepth, uint16_t aui16_fontSizes);
#endif

#if DEBUG_SCHEDULER
  virtual const char* getTaskName() const;
#endif

private:
  uint16_t getClientCount() const;

  class CanCustomerProxy_c : public CanCustomer_c {
  public:
    typedef VtClient_c Owner_t;

    CanCustomerProxy_c(Owner_t &art_owner) : mrt_owner(art_owner) {}

    virtual ~CanCustomerProxy_c() {}

  private:
    virtual bool processMsg( const CanPkg_c& arc_data ) {
      return mrt_owner.processMsg( arc_data );
    }

    virtual bool reactOnStreamStart(
        ReceiveStreamIdentifier_c const &ac_ident,
        uint32_t aui32_totalLen)
    {
      return mrt_owner.reactOnStreamStart(ac_ident, aui32_totalLen);
    }

    virtual void reactOnAbort(Stream_c &arc_stream)
    {
      mrt_owner.reactOnAbort(arc_stream);
    }

    virtual bool processPartStreamDataChunk(
        Stream_c &apc_stream,
        bool ab_isFirstChunk,
        bool ab_isLastChunk)
    {
      return mrt_owner.processPartStreamDataChunk(
          apc_stream,
          ab_isFirstChunk,
          ab_isLastChunk);
    }

    virtual void notificationOnMultiReceiveError(
        ReceiveStreamIdentifier_c const &ac_streamIdent,
        uint8_t aui8_multiReceiveError,
        bool ab_isGlobal)
    {
      mrt_owner.notificationOnMultiReceiveError(
          ac_streamIdent,
          aui8_multiReceiveError,
          ab_isGlobal);
    }

    // CanCustomerProxy_c shall not be copyable. Otherwise the
    // reference to the containing object would become invalid.
    CanCustomerProxy_c(CanCustomerProxy_c const &);

    CanCustomerProxy_c &operator=(CanCustomerProxy_c const &);

    Owner_t &mrt_owner;
  };
  typedef CanCustomerProxy_c Customer_t;
  class ControlFunctionStateHandlerProxy_c : public ControlFunctionStateHandler_c {
  public:
    typedef VtClient_c Owner_t;

    ControlFunctionStateHandlerProxy_c(Owner_t &art_owner) : mrt_owner(art_owner) {}

    virtual ~ControlFunctionStateHandlerProxy_c() {}

  private:
    virtual void reactOnIsoItemModification(
        iIsoItemAction_e at_action,
        IsoItem_c const &acrc_isoItem)
    {
      mrt_owner.reactOnIsoItemModification(at_action, acrc_isoItem);
    }

    // ControlFunctionStateHandlerProxy_c shall not be copyable. Otherwise the
    // reference to the containing object would become invalid.
    ControlFunctionStateHandlerProxy_c(ControlFunctionStateHandlerProxy_c const &);

    ControlFunctionStateHandlerProxy_c &operator=(ControlFunctionStateHandlerProxy_c const &);

    Owner_t &mrt_owner;
  };
  typedef ControlFunctionStateHandlerProxy_c Handler_t;

  /** private constructor which prevents direct instantiation in user application
    * NEVER define instance of VtClient_c within application
    */
  VtClient_c();

  virtual void reactOnIsoItemModification (ControlFunctionStateHandler_c::iIsoItemAction_e at_action, IsoItem_c const& acrc_isoItem);

  virtual bool reactOnStreamStart(
      ReceiveStreamIdentifier_c const &ac_ident,
      uint32_t aui32_totalLen)
  {
    return mt_customer.reactOnStreamStartDefault(ac_ident, aui32_totalLen);
  }

  virtual void reactOnAbort(Stream_c &arc_stream)
  {
    mt_customer.reactOnAbortDefault(arc_stream);
  }

  virtual bool processPartStreamDataChunk(
      Stream_c &apc_stream,
      bool ab_isFirstChunk,
      bool ab_isLastChunk)
  {
    return mt_customer.processPartStreamDataChunkDefault(
        apc_stream,
        ab_isFirstChunk,
        ab_isLastChunk);
  }

  virtual void notificationOnMultiReceiveError(
      ReceiveStreamIdentifier_c const &ac_streamIdent,
      uint8_t aui8_multiReceiveError,
      bool ab_isGlobal)
  {
    mt_customer.notificationOnMultiReceiveErrorDefault(
        ac_streamIdent,
        aui8_multiReceiveError,
        ab_isGlobal);
  }

  virtual uint16_t getForcedMinExecTime() const
  {
    return getForcedMinExecTimeDefault();
  }

  VtClientConnection_c* initAndRegisterIsoObjectPoolCommon(
    IdentItem_c& rc_identItem, 
    IsoAgLib::iVtClientObjectPool_c& arc_pool, 
    const char* apc_versionLabel, 
    IsoAgLib::iVtClientDataStorage_c& apc_claimDataStorage, 
    IsoAgLib::iVtClientObjectPool_c::RegisterPoolMode_en aen_mode );

protected:

private: // attributes

  STL_NAMESPACE::vector<VtServerInstance_c*> ml_vtServerInst;

  STL_NAMESPACE::vector<VtClientConnection_c*> mvec_vtClientServerComm;
  Handler_t mt_handler;
  Customer_t mt_customer;
  friend VtClient_c &getVtClientInstance(uint8_t aui8_instance);
};

/** C-style function, to get access to the unique Scheduler_c singleton instance
 * if more than one CAN BUS is used for IsoAgLib, an index must be given to select the wanted BUS
 */
VtClient_c &getVtClientInstance(uint8_t aui8_instance = 0);

}
#endif