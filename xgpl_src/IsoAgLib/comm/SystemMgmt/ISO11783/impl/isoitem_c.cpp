/***************************************************************************
                          isoitem_c.cpp - object which represents an item
                                            in a service monitor list
                             -------------------
    begin                : Tue Jan 01 2001
    copyright            : (C) 2000 - 2004 by Dipl.-Inform. Achim Spangler
    email                : a.spangler@osb-ag:de
    type                 : Source
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 * This file is part of the "IsoAgLib", an object oriented program library *
 * to serve as a software layer between application specific program and   *
 * communication protocol details. By providing simple function calls for  *
 * jobs like starting a measuring program for a process data value on a    *
 * remote ECU, the main program has not to deal with single CAN telegram   *
 * formatting. This way communication problems between ECU's which use     *
 * this library should be prevented.                                       *
 * Everybody and every company is invited to use this library to make a    *
 * working plug and play standard out of the printed protocol standard.    *
 *                                                                         *
 * Copyright (C) 2000 - 2004 Dipl.-Inform. Achim Spangler                  *
 *                                                                         *
 * The IsoAgLib is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published          *
 * by the Free Software Foundation; either version 2 of the License, or    *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This library is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with IsoAgLib; if not, write to the Free Software Foundation,     *
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA           *
 *                                                                         *
 * As a special exception, if other files instantiate templates or use     *
 * macros or inline functions from this file, or you compile this file and *
 * link it with other works to produce a work based on this file, this file*
 * does not by itself cause the resulting work to be covered by the GNU    *
 * General Public License. However the source code for this file must still*
 * be made available in accordance with section (3) of the                 *
 * GNU General Public License.                                             *
 *                                                                         *
 * This exception does not invalidate any other reasons why a work based on*
 * this file might be covered by the GNU General Public License.           *
 *                                                                         *
 * Alternative licenses for IsoAgLib may be arranged by contacting         *
 * the main author Achim Spangler by a.spangler@osb-ag:de                  *
 ***************************************************************************/

 /**************************************************************************
 *                                                                         *
 *     ###    !!!    ---    ===    IMPORTANT    ===    ---    !!!    ###   *
 * Each software module, which accesses directly elements of this file,    *
 * is considered to be an extension of IsoAgLib and is thus covered by the *
 * GPL license. Applications must use only the interface definition out-   *
 * side :impl: subdirectories. Never access direct elements of __IsoAgLib  *
 * and __HAL namespaces from applications which shouldnt be affected by    *
 * the license. Only access their interface counterparts in the IsoAgLib   *
 * and HAL namespaces. Contact a.spangler@osb-ag:de in case your applicat- *
 * ion really needs access to a part of an internal namespace, so that the *
 * interface might be extended if your request is accepted.                *
 *                                                                         *
 * Definition of direct access:                                            *
 * - Instantiation of a variable with a datatype from internal namespace   *
 * - Call of a (member-) function                                          *
 * Allowed is:                                                             *
 * - Instatiation of a variable with a datatype from interface namespace,  *
 *   even if this is derived from a base class inside an internal namespace*
 * - Call of member functions which are defined in the interface class     *
 *   definition ( header )                                                 *
 *                                                                         *
 * Pairing of internal and interface classes:                              *
 * - Internal implementation in an :impl: subdirectory                     *
 * - Interface in the parent directory of the corresponding internal class *
 * - Interface class name IsoAgLib::iFoo_c maps to the internal class      *
 *   __IsoAgLib::Foo_c                                                     *
 *                                                                         *
 * AS A RULE: Use only classes with names beginning with small letter :i:  *
 ***************************************************************************/
#include "isoitem_c.h"
#include "isosystempkg_c.h"
#include "isomonitor_c.h"
#include <IsoAgLib/comm/Scheduler/impl/scheduler_c.h>
#include <IsoAgLib/comm/Base/impl/base_c.h>
#include <IsoAgLib/driver/can/impl/canio_c.h>
#ifdef USE_EEPROM_IO
#include <IsoAgLib/driver/eeprom/impl/eepromio_c.h>
#endif
#ifdef USE_ISO_TERMINAL
#include <IsoAgLib/comm/ISO_Terminal/impl/isoterminal_c.h>
#endif

namespace __IsoAgLib {


/**
  constructor which can set optional all ident data
  @param ri32_time creation time of this item instance
  @param rc_gtp GETY_POS code of this item ((deviceClass << 3) | devClInst )
  @param rui8_nr number of this item
  @param rb_status state of this ident (off, claimed address, ...) (default: off)
  @param rpb_name ISO 11783 8-uint8_t NAME setting for a member
  @param rui16_saEepromAdr EEPROM adress to store actual SA -> next boot with same adr
  @param ri_singletonVecKey optional key for selection of IsoAgLib instance (default 0)
*/
ISOItem_c::ISOItem_c(int32_t ri32_time, GetyPos_c rc_gtp, uint8_t rui8_nr, IState_c::itemState_t rb_status,
      const uint8_t* rpb_name, uint16_t rui16_saEepromAdr, int ri_singletonVecKey )
  : MonitorItem_c(0, rc_gtp, rui8_nr, rb_status, ri_singletonVecKey ),
    c_isoName(rpb_name), ui16_saEepromAdr(rui16_saEepromAdr)
{ // mark this item as prepare address claim if local
  setItemState(IState_c::itemState_t(IState_c::Member | IState_c::Iso));
  wsClaimedAddress = false;

  // define this item as standalone by default
  pc_masterItem = NULL;

  // just use ri32_time to make compiler happy
  // an ISO11783 item must start with timestamp 0
  // but as it uses the same parameter signature start as the DIN9684
  // constructors, the parameter ri32_time must be defined
  if ( ri32_time == 0 ) updateTime( ri32_time );
  else updateTime(0); // state that this item didn't send an adress claim

  readEepromSa();
  // set give GTP in NAME field
  c_isoName.setGtp(rc_gtp);
  if (itemState(IState_c::Local))
  {
    setItemState(IState_c::PreAddressClaim);
    timeEvent(); // call time for further init
  }
}

/**
  constructor which can set optional all ident data
  @param ri32_time creation time of this item instance
  @param rc_gtp GETY_POS code of this item ((deviceClass << 3) | devClInst )
  @param rui8_nr number of this item
  @param rb_status state of this ident (off, claimed address, ...) (default: off)
  @param rpc_name ISO 11783 NAME setting for a member as pointer to ISOName
  @param rui16_saEepromAdr EEPROM adress to store actual SA -> next boot with same adr
  @param ri_singletonVecKey optional key for selection of IsoAgLib instance (default 0)
*/
ISOItem_c::ISOItem_c(int32_t ri32_time, GetyPos_c rc_gtp, uint8_t rui8_nr, IState_c::itemState_t rb_status,
      const ISOName_c* rpc_name, uint16_t rui16_saEepromAdr, int ri_singletonVecKey )
  : MonitorItem_c(0, rc_gtp, rui8_nr, rb_status, ri_singletonVecKey ),
    c_isoName(*rpc_name), ui16_saEepromAdr(rui16_saEepromAdr)
{ // mark this item as prepare address claim if local

  wsClaimedAddress = false;

  // define this item as standalone by default
  pc_masterItem = NULL;

  // just use ri32_time to make compiler happy
  // an ISO11783 item must start with timestamp 0
  // but as it uses the same parameter signature start as the DIN9684
  // constructors, the parameter ri32_time must be defined
  if ( ri32_time == 0 ) updateTime( ri32_time );
  else updateTime(0); // state that this item didn't send an adress claim

  readEepromSa();
  // set give GTP in NAME field
  c_isoName.setGtp(rc_gtp);
  setItemState(IState_c::itemState_t(IState_c::Member | IState_c::Iso));
  if (itemState(IState_c::Local))
  {
    setItemState(IState_c::PreAddressClaim);
    timeEvent(); // call time for further init
  }
}

/**
  constructor which can set the 64bit NAME from single flags
  @param ri32_time creation time of this item instance
  @param rc_gtp GETY_POS code of this item ((deviceClass << 3) | devClInst )
  @param rui8_nr number of this item
  @param rb_status state of this ident (off, claimed address, ...) (default: off)
  @param rb_selfConf true -> the item has a self configurable source adress
  @param rui8_indGroup industry group code (2 for agriculture)
  @param rb_func function code (25 = network interconnect)
  @param rui16_manufCode manufactor code
  @param rui32_serNo serial no specific for one ECU of one manufactor
  @param rui16_saEepromAdr EEPROM adress to store actual SA -> next boot with same adr
  @param rb_funcInst counter for devices with same function (default 0)
  @param rb_ecuInst counter for ECU with same function and function instance (default 0)
  @param ri_singletonVecKey optional key for selection of IsoAgLib instance (default 0)
*/
ISOItem_c::ISOItem_c(int32_t ri32_time, GetyPos_c rc_gtp, uint8_t rui8_nr, IState_c::itemState_t rb_status,
        bool rb_selfConf, uint8_t rui8_indGroup, uint8_t rb_func, uint16_t rui16_manufCode,
        uint32_t rui32_serNo, uint16_t rui16_saEepromAdr, uint8_t rb_funcInst, uint8_t rb_ecuInst, int ri_singletonVecKey )
    : MonitorItem_c(0, rc_gtp, rui8_nr, rb_status, ri_singletonVecKey ),
      c_isoName(rb_selfConf, rui8_indGroup, (rc_gtp.getGety()), (rc_gtp.getPos()),
        rb_func, rui16_manufCode, rui32_serNo, rb_funcInst, rb_ecuInst), ui16_saEepromAdr(rui16_saEepromAdr)
{ // mark this item as prepare address claim if local
  setItemState(IState_c::itemState_t(IState_c::Member | IState_c::Iso));
  wsClaimedAddress = false;

  // define this item as standalone by default
  pc_masterItem = NULL;

  // just use ri32_time to make compiler happy
  // an ISO11783 item must start with timestamp 0
  // but as it uses the same parameter signature start as the DIN9684
  // constructors, the parameter ri32_time must be defined
  if ( ri32_time == 0 ) updateTime( ri32_time );
  else updateTime(0); // state that this item didn't send an adress claim

  readEepromSa();
  if (itemState(IState_c::Local))
  {
    setItemState(IState_c::PreAddressClaim);
    timeEvent(); // call time for further init
  }
}

/**
  copy constructor for ISOItem
	The copy constructor checks if the source item is
	a master ( i.e. the pc_masterItem pointer points to this )
	-> it doesn't simply copy the pointer, but sets its
	own pointer also to the this-pointer of the new instance
  @param rrefc_src source ISOItem_c instance
*/
ISOItem_c::ISOItem_c(const ISOItem_c& rrefc_src)
  : MonitorItem_c(rrefc_src), c_isoName(rrefc_src.c_isoName), ui16_saEepromAdr(rrefc_src.ui16_saEepromAdr)
{ // mark this item as prepare address claim if local
  setItemState(IState_c::itemState_t(IState_c::Member | IState_c::Iso));
  wsClaimedAddress = false;

  // check if the master item pointer of the source item
	// is pointing to itself, as then this new created instance shall
	// not copy the pointer, but set the pointer to itself, as this
	// indicates the Master State
	if ( rrefc_src.isMaster() )
	{ // set our pc_masterItem also to this, to indicate master state
		pc_masterItem = this;
	}
	else
	{ // just copy the master pointer from source
		pc_masterItem = rrefc_src.getMaster ();
	}

  updateTime(0); // state that this item didn't send an adress claim
  readEepromSa();
  if (itemState(IState_c::Local))
  {
    setItemState(IState_c::PreAddressClaim);
    timeEvent(); // call time for further init
  }
}

/**
  assign constructor for ISOItem
  @param rrefc_src source ISOItem_c object
*/
ISOItem_c& ISOItem_c::operator=(const ISOItem_c& rrefc_src)
{
  MonitorItem_c::operator=(rrefc_src);
	c_isoName = rrefc_src.c_isoName;
  setItemState(IState_c::itemState_t(IState_c::Member | IState_c::Iso));
  wsClaimedAddress = false;
  updateTime(0); // state that this item didn't send an adress claim
  // mark this item as prepare address claim if local
  ui16_saEepromAdr = rrefc_src.ui16_saEepromAdr;
  readEepromSa();
  if (itemState(IState_c::Local))
  {
    setItemState(IState_c::PreAddressClaim);
    timeEvent(); // call time for further init
  }
  return *this;
}

/**
  default destructor
*/
ISOItem_c::~ISOItem_c(){
}

/**
  deliver name
  @return pointer to the name uint8_t string (7byte)
*/
const uint8_t* ISOItem_c::name() const {
  return c_isoName.outputString();
}
/**
  check if the name field is empty (no name received)
  @return true -> no name received
*/
bool ISOItem_c::isEmptyName() const {
  return false;
}
/**
  deliver name as pure ASCII string
  @param pc_name string where ASCII string is inserted
  @param rui8_maxLen max length for name
*/
void ISOItem_c::getPureAsciiName(int8_t *pc_asciiName, uint8_t rui8_maxLen){
  char c_temp[30];
  const uint8_t* pb_src = c_isoName.outputString();
  CNAMESPACE::sprintf(c_temp, "0x%02x%02x%02x%02x%02x%02x%02x%02x", pb_src[0],pb_src[1],pb_src[2],pb_src[3],
      pb_src[4],pb_src[5],pb_src[6],pb_src[7]);

  uint8_t ui8_len = CNAMESPACE::strlen(c_temp);
  if (rui8_maxLen < ui8_len) ui8_len = rui8_maxLen;
  CNAMESPACE::memcpy(pc_asciiName, c_temp, ui8_len );
  pc_asciiName[ui8_len-1] = '\0';
}

/**
  set all element data with one call
  @param ri32_time creation time of this item instance
  @param rc_gtp GETY_POS code of this item ((deviceClass << 3) | devClInst )
  @param rui8_nr number of this item
  @param rb_status state of this ident (off, claimed address, ...)
  @param rui16_saEepromAdr EEPROM adress to store actual SA -> next boot with same adr
  @param rpb_name ISO 11783 8-uint8_t NAME setting for a member
  @param ri_singletonVecKey optional key for selection of IsoAgLib instance (default 0)
*/
void ISOItem_c::set(int32_t ri32_time, GetyPos_c rc_gtp, uint8_t rui8_nr,
        itemState_t ren_status, uint16_t rui16_saEepromAdr, const uint8_t* rpb_name, int ri_singletonVecKey )
{
  MonitorItem_c::set(ri32_time, rc_gtp, rui8_nr, ren_status, ri_singletonVecKey);
  if (rpb_name != NULL) c_isoName.inputString(rpb_name);
  ui16_saEepromAdr = rui16_saEepromAdr;
  readEepromSa();
  // set give GTP in NAME field
  c_isoName.setGtp(rc_gtp);
};

/**
  set all element data with one call
  @param ri32_time creation time of this item instance
  @param rc_gtp GETY_POS code of this item ((deviceClass << 3) | devClInst )
  @param rui8_nr number of this item
  @param rb_status state of this ident (off, claimed address, ...)
  @param rui16_saEepromAdr EEPROM adress to store actual SA -> next boot with same adr
  @param rpc_name ISO 11783 NAME setting for a member as pointer to ISOName
  @param ri_singletonVecKey optional key for selection of IsoAgLib instance (default 0)
*/
void ISOItem_c::set(int32_t ri32_time, GetyPos_c rc_gtp, uint8_t rui8_nr,
        itemState_t ren_status, uint16_t rui16_saEepromAdr, const ISOName_c* rpc_name, int ri_singletonVecKey )
{
  MonitorItem_c::set(ri32_time, rc_gtp, rui8_nr, ren_status, ri_singletonVecKey);
  if (rpc_name != NULL) c_isoName.operator=(*rpc_name);
  ui16_saEepromAdr = rui16_saEepromAdr;
  readEepromSa();
  // set give GTP in NAME field
  c_isoName.setGtp(rc_gtp);
};

/**
  set all element data with one call
  @param ri32_time creation time of this item instance
  @param rc_gtp GETY_POS code of this item ((deviceClass << 3) | devClInst )
  @param rui8_nr number of this item
  @param rb_selfConf true -> the item has a self configurable source adress
  @param rui8_indGroup industry group code (2 for agriculture)
  @param rb_func function code (25 = network interconnect)
  @param rui16_manufCode manufactor code
  @param rui32_serNo serial no specific for one ECU of one manufactor
  @param ren_status state of this ident (off, claimed address, ...) (default: off)
  @param rui16_saEepromAdr EEPROM adress to store actual SA -> next boot with same adr
  @param rb_funcInst counter for devices with same function (default 0)
  @param rb_ecuInst counter for ECU with same function and function instance (default 0)
  @param ri_singletonVecKey optional key for selection of IsoAgLib instance (default 0)
*/
void ISOItem_c::set(int32_t ri32_time, GetyPos_c rc_gtp, uint8_t rui8_nr,
        bool rb_selfConf, uint8_t rui8_indGroup, uint8_t rb_func, uint16_t rui16_manufCode,
        uint32_t rui32_serNo, itemState_t ren_status, uint16_t rui16_saEepromAdr, uint8_t rb_funcInst,
        uint8_t rb_ecuInst, int ri_singletonVecKey )
{
  MonitorItem_c::set(ri32_time, rc_gtp, rui8_nr, ren_status, ri_singletonVecKey);
  c_isoName.set(rb_selfConf, rui8_indGroup, (rc_gtp.getGety()), (rc_gtp.getPos()),
        rb_func, rui16_manufCode, rui32_serNo, rb_funcInst, rb_ecuInst);
  ui16_saEepromAdr = rui16_saEepromAdr;
  readEepromSa();
  // set give GTP in NAME field
  c_isoName.setGtp(rc_gtp);
}

/**
  periodically time evented actions:
    * find free SA or check if last SA is available
    * send adress claim

  possible errors:
    * dependant error in CANIO_c during send
  @return true -> all planned time event activitie performed
*/
bool ISOItem_c::timeEvent( void )
{
  CANIO_c& c_can = getCanInstance4Comm();
  ISOMonitor_c& c_isoMonitor = getIsoMonitorInstance4Comm();
  int32_t i32_time = Scheduler_c::getLastTimeEventTrigger();

  if ( Scheduler_c::getAvailableExecTime() == 0 ) return false;

  ISOSystemPkg_c& c_pkg = c_isoMonitor.data();
  if (itemState(IState_c::PreAddressClaim))
  { // this item is in prepare address claim state -> wait for sending first adress claim
    int32_t i32_lastAdrRequestTime = c_isoMonitor.lastIsoSaRequest();
    if (i32_lastAdrRequestTime != 0)
    {
      if ((i32_time - i32_lastAdrRequestTime) > (1250 + calc_randomWait()))
      { // last iso adress claim request is still valid and should have been
        // answered till now
        //check if this item is self conf
        // unifyIsoSa delivers actual SA of this item if free
        // if actual SA isn't free and item is NOT-self-conf -> NACK flag 254
        // is answered
        // if actual SA is not free for self-conf item
        // -> a free SA is searched and answered
        // - if no free SA is found, 254 is answered for NACK
        setNr(c_isoMonitor.unifyIsoSa(this));

        if (nr() == 254)
        { // no success -> send NACK and switch to off | error state
          setItemState(IState_c::itemState_t(IState_c::Off | IState_c::Error));
          clearItemState(IState_c::PreAddressClaim);
        }
        else
        { // success -> start address claim mode
          setItemState(IState_c::AddressClaim);
        }

        // now nr() has now suitable value
        c_pkg.setIsoPri(6);
        // PGN is equivalent to definition of PF and DP in this case
        //c_pkg.setIsoPgn(ADRESS_CLAIM_PGN);
        c_pkg.setIsoDp(0);
        c_pkg.setIsoPf(238);
        c_pkg.setIsoPs(255); // global information
        c_pkg.setIsoSa(nr()); // free SA or NACK flag
        // set NAME to CANPkg
        c_pkg.setName(outputString());
        // now ISOSystemPkg_c has right data -> send
        // if (!(c_pkg.identType() == Ident_c::ExtendedIdent))
        // CANPkg_c::identType() changed to static
        if (!(CANPkg_c::identType() == Ident_c::ExtendedIdent))
        {
          // c_pkg.setIdentType(Ident_c::ExtendedIdent);
          // CANPkg_c::setIdentType changed to static
          CANPkg_c::setIdentType(Ident_c::ExtendedIdent);
        }
        c_can << c_pkg;
        // update timestamp
        updateTime();
      }
    }
    else
    { // no adress claim request sent till now
			getIsoMonitorInstance4Comm().sendRequestForClaimedAddress( true );
    }
  }
  else if (itemState(IState_c::AddressClaim))
  { // item in address claim mode (time between send of claim and
    // check if there is a CAN send conflict during send of adress claim
    // final acceptance of adr claim (wait for possible contention)
    if (c_can.stopSendRetryOnErr())
    { // item was in address claim state and send of claim caused error
      setItemState(IState_c::PreAddressClaim);
    }
    else
    {
      if (checkTime(250))
      {
        // no conflict since sent of adress claim since 250ms
        setItemState(IState_c::ClaimedAddress);
        #ifdef USE_ISO_TERMINAL
        b_oldVtState = false; // means send out master message out first, but wait for VT Status Message to arrive first!
        #endif
      }
    }
  }
  #ifdef USE_ISO_TERMINAL
  else if ( itemState(IState_c::ClaimedAddress) ) {
    // *** State ClaimedAddress ***
    if ( !getIsoTerminalInstance().isVtActive() ) {
      wsClaimedAddress = false;
    } else {
      // *** VT is Active ***
      if ( b_oldVtState == false ) {
        /* VT has just been turned ON */
        slavesToClaimAddress = -1; // start WS Announce/MaintenanceMsg'ing
      }

      if ( ( slavesToClaimAddress == -1 ) && ( pc_masterItem == this ) )
      {
        slavesToClaimAddress = getIsoMonitorInstance4Comm().getSlaveCount (this); // slavesToClaimAddress will be 0..numberOfSlaves hopefully

        c_pkg.setExtCanPkg8(7, 0, 254, 13, nr(), slavesToClaimAddress+1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
        c_can << c_pkg;     // send out "Working Set Master" message on CAN bus

        if (slavesToClaimAddress == 0) wsClaimedAddress = true;
        updateTime();
      }
      else if ( ( slavesToClaimAddress > 0) && ( pc_masterItem == this ) && ( checkTime(100) ) )
      {
        // claim address for next slave
        c_pkg.setIsoPri(7);
        c_pkg.setIsoDp(0);
        c_pkg.setIsoPf(254);
        c_pkg.setIsoPs(12); // global information
        c_pkg.setIsoSa(nr()); // free SA or NACK flag
        // set NAME to CANPkg
        c_pkg.setName(getIsoMonitorInstance4Comm().getSlave (getIsoMonitorInstance4Comm().getSlaveCount(this)-slavesToClaimAddress, this)->outputString());
        c_can << c_pkg;

        // claimed address for one...
        slavesToClaimAddress--;

        if (slavesToClaimAddress == 0) wsClaimedAddress = true;
        updateTime();
      }
      else if ( ( slavesToClaimAddress == 0 ) && ( pc_masterItem == this ) && ( checkTime(1000) ) )
      {
        // send working set maintenance message now every second
        c_pkg.setExtCanPkg8(7, 0, 231, getIsoTerminalInstance().getVtSourceAddress (), nr(),
                            0xFF, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
        c_can << c_pkg;     // G.2: Function: 255 / 0xFF Working Set Maintenance Message
        updateTime();
      }
    }
    b_oldVtState = getIsoTerminalInstance().isVtActive();
  }
  #endif

  // nothing to be done
  return true;
}


/**
  process received CAN pkg to update data and react if needed
  * update settings for remote members (e.g. change of SA)
  * react on adress claims or request for adress claims for local items
  @return true -> a reaction on the received/processed msg was sent
*/
bool ISOItem_c::processMsg(){
  ISOSystemPkg_c& c_pkg = getIsoMonitorInstance4Comm().data();
  int32_t i32_pkgTime = c_pkg.time(),
      i32_now = Scheduler_c::getLastTimeEventTrigger();
  if ((i32_now - i32_pkgTime) > 100) updateTime(i32_now);
  else updateTime(i32_pkgTime);

  switch ((c_pkg.isoPgn() & 0x1FF00))
  {
    case ADRESS_CLAIM_PGN: // adress claim
      // check if this item is local
      if ( (itemState(IState_c::Local)) && (c_pkg.isoSa() == nr()) )
      { // local item has same SA
        if ( (itemState(IState_c::AddressClaim))
          || (itemState(IState_c::ClaimedAddress))
           )
        { // local item has already claimed the same adress
          // -> react suitable for this contention
          // -> check for NAME
          if (c_isoName.higherPriThanPar(c_pkg.name()))
          { // this item has higher prio (lower val) -> send adr claim
            c_pkg.setIsoSa(nr());
          }
          else
          { // local item has lower prio -> search free SA and claim this
            // or set nr to the code 254 %e.g. no suitbla nr found -> error
            setNr(getIsoMonitorInstance4Comm().unifyIsoSa(this));
            c_pkg.setIsoSa(nr());
            if (nr() != 254)
            { // free adr found
              setItemState(IState_c::AddressClaim);
            }
            else
            { // no free self conf adr found -> send NACK with SA=254
              // (delivered by unifyIsoAdr) -> already stored in nr()
              setItemState(IState_c::itemState_t(IState_c::Error | IState_c::Off));
            }
          } // end else for local item with lower prio
          // now send adr claim (SA already set dependent on NAME)
          c_pkg.setIsoPri(6);
          c_pkg.setIsoPgn(ADRESS_CLAIM_PGN);
          c_pkg.setIsoPf(238);
          c_pkg.setIsoPs(255); // global information
          // set NAME to CANPkg
          c_pkg.setName(outputString());
          // now ISOSystemPkg_c has right data -> send
          getCanInstance4Comm() << c_pkg;
        } // end if for local item after already sent claim
      } // end if for local item with same SA
      else if ( (!itemState(IState_c::Local))
             || (!itemState(IState_c::ClaimedAddress)) )
      { // no local item or local item not yet performed complete address claim
        // -> simply update this item
        // (in case this item was local item in address claim state,
        //  Ident_Item::timeEvent will look for new POS/Device Class Instance
        //  to unify GTP -> if possible it will insert new item for local ident)
        clearItemState(IState_c::Local);
        setItemState(IState_c::ClaimedAddress);
        setNr(c_pkg.isoSa());
        inputString(c_pkg.name());
      }
    break;
    case REQUEST_PGN_MSG_PGN: // request for PGN
      int32_t i32_reqPgn = (
                          (static_cast<int32_t>(c_pkg.operator[](0)))
                        | (static_cast<int32_t>(c_pkg.operator[](1)) << 8)
                        | (static_cast<int32_t>(c_pkg.operator[](2)) << 16)
                        );
      if ((c_pkg.isoPs() == nr()) || (c_pkg.isoPs() == 255))
      { // this item should answer the request
        switch (i32_reqPgn)
        {
					case ADRESS_CLAIM_PGN: // request for adress claim
						c_pkg.setIsoPri(6);
						c_pkg.setIsoSa(nr());
						c_pkg.setIsoPgn(ADRESS_CLAIM_PGN);
						c_pkg.setIsoPf(238);
						c_pkg.setIsoPs(255); // global information
						// set NAME to CANPkg
						c_pkg.setName(outputString());
						// now ISOSystemPkg_c has right data -> send
						getCanInstance4Comm() << c_pkg;
						break;
					#ifdef USE_BASE
					case TIME_DATE_PGN: // request for calendar
						// call Base_c function to send calendar
						// isoSendCalendar checks if this item (identified by GETY_POS)
						// is configured to send calendar
						getBaseInstance4Comm().isoSendCalendar(gtp());
						break;
					#endif
        }
      }
    break;
  } // end switch

  return true;
};


/**
  set eeprom adress and read SA from there
*/
void ISOItem_c::readEepromSa()
{
  if ((ui16_saEepromAdr != 0xFFFF) && (nr() == 0xFE))
  {
    uint8_t ui8_eepromNr = 128;
#ifdef USE_EEPROM_IO
    getEepromInstance().setg(ui16_saEepromAdr);
    getEepromInstance() >> ui8_eepromNr;
#endif
    setNr(ui8_eepromNr);
  }
}

/**
  write actual SA to the given EEPROM adress
*/
void ISOItem_c::writeEepromSa()
{
#ifdef USE_EEPROM_IO
  if ((ui16_saEepromAdr != 0xFFFF) && (nr() != 0xFE))
  {
    getEepromInstance().setp(ui16_saEepromAdr);
    getEepromInstance() << nr();
  }
#endif
}
/**
  calculate random wait time between 0 and 153msec. from NAME and time
  @return wait offset in msec. [0..153]
*/
uint8_t ISOItem_c::calc_randomWait()
{ // perform some calculation from NAME
  uint16_t ui16_result = outputString()[0] * outputString()[1];
  if ( ( (outputString()[2] +1) != 0 )
    && ( System_c::getTime() != 0 )
    && ( (System_c::getTime() / (outputString()[2] +1)) != 0 ) )
    ui16_result /= (System_c::getTime() / (outputString()[2] +1));
  ui16_result += outputString()[3];
  ui16_result %= (outputString()[4] + 1);
  ui16_result -= outputString()[5];
  ui16_result *= ((outputString()[6] + 1) / (outputString()[7] + 1));
  // divide by last uint8_t of name till offset in limit
  for (;ui16_result > 153; ui16_result /= (outputString()[7] + 1));
  return ui16_result;
}

/**
  returns a pointer to the referenced master ISOItem
*/
ISOItem_c* ISOItem_c::getMaster () const
{
  return pc_masterItem;
}

/**
  attaches to a working set master and become slave hereby
*/
void ISOItem_c::setMaster ( ISOItem_c* rpc_masterItem )
{
  pc_masterItem = rpc_masterItem;
}

} // end of namespace __IsoAgLib
