/***************************************************************************
                          base_c.cpp - working on Base Data Msg Type 1, 2
                                    and Calendar; stores, updates  and
                                    delivers all base data informations
                                    from CANCustomer_c derived for CAN
                                    sending and receiving interaction;
                                    from ElementBase_c derived for
                                    interaction with other IsoAgLib objects
                             -------------------
    begin                 Fri Apr 07 2000
    copyright            : (C) 2000 - 2004 by Dipl.-Inform. Achim Spangler
    email                : a.spangler@osb-ag:de
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

#include <IsoAgLib/driver/system/impl/system_c.h>
#include <IsoAgLib/driver/can/impl/canio_c.h>
#include <IsoAgLib/comm/Scheduler/impl/scheduler_c.h>
#include <IsoAgLib/comm/SystemMgmt/impl/systemmgmt_c.h>
#include <IsoAgLib/comm/SystemMgmt/impl/istate_c.h>
#ifdef USE_DIN_9684
  #include <IsoAgLib/comm/SystemMgmt/DIN9684/impl/dinmonitor_c.h>
#endif
#ifdef USE_ISO_11783
  #include <IsoAgLib/comm/SystemMgmt/ISO11783/impl/isomonitor_c.h>
#endif
#include <IsoAgLib/util/config.h>
#include "base_c.h"

namespace __IsoAgLib {
#if defined( PRT_INSTANCE_CNT ) && ( PRT_INSTANCE_CNT > 1 )
  /** C-style function, to get access to the unique Base_c singleton instance
    * if more than one CAN BUS is used for IsoAgLib, an index must be given to select the wanted BUS
    */
  Base_c& getBaseInstance( uint8_t rui8_instance )
  { // if > 1 singleton instance is used, no static reference can be used
    return Base_c::instance( rui8_instance );
  };
#else
  /** C-style function, to get access to the unique Base_c singleton instance */
  Base_c& getBaseInstance( void )
  {
    static Base_c& c_lbsBase = Base_c::instance();
    return c_lbsBase;
  };
#endif

uint8_t bcd2dec(uint8_t rb_bcd);
uint8_t dec2bcd(uint8_t rb_dec);

/**
  initialise element which can't be done during construct;
  above all create the needed FilterBox_c instances, to receive
  the needed CAN msg with base msg type 1,2 and calendar

  possible errors:
      * dependant error in CANIO_c problems during insertion of new FilterBox_c entries for IsoAgLibBase
  @param rpc_gtp optional pointer to the GETY_POS variable of the ersponsible member instance (pointer enables automatic value update if var val is changed)
	@param rt_mySendSelection optional Bitmask of base data to send ( default send nothing )
*/
void Base_c::init(GetyPos_c* rpc_gtp, IsoAgLib::BaseDataGroup_t rt_mySendSelection)
{ // clear state of b_alreadyClosed, so that close() is called one time
  clearAlreadyClosed();
  // first register in Scheduler_c
  getSchedulerInstance4Comm().registerClient( this );
  c_data.setSingletonKey( getSingletonVecKey() );


  // set the member base msg value vars to NO_VAL codes
  setDay(1);
  setHour(0);
  setMinute(0);
  setMonth(1);
  setYear(0);
  setSecond(0);
  setHitchRear(NO_VAL_8);
  setHitchFront(NO_VAL_8);
  i16_engine = NO_VAL_16S;
  i16_ptoFront = i16_ptoRear = i16_speedReal = i16_speedTheor = NO_VAL_16S;

  // set distance value to 0
  i16_lastDistReal = i16_lastDistTheor = 0;
  i32_distReal = i32_distTheor = 0;

  // set the timestamps to 0
  ui8_lastBase1 = ui8_lastBase2 = ui8_lastBase3 = ui8_lastFuel
		= ui8_lastCalendar = 0;
	#ifdef USE_DIN_9684
	i16_rearLeftDraft = i16_rearRightDraft = i16_rearDraftNewton = NO_VAL_16S;
	ui8_rearDraftNominal = NO_VAL_8;
	i16_fuelRate = NO_VAL_16S;
	ui8_fuelTemperature = NO_VAL_8;
	b_dinFilterCreated = false;
	#endif
  #ifdef USE_ISO_11783
  ui8_lastIsoBase1 = ui8_lastIsoBase2 = ui8_lastIsoCalendar = 0;
  t_frontPtoEngaged = t_rearPtoEngaged
  = t_frontPto1000 = t_rearPto1000
  = t_frontPtoEconomy = t_rearPtoEconomy = t_keySwitch = IsoAgLib::IsoNotAvailable; // mark as not available
  ui8_maxPowerTime = ui8_frontLinkForce = ui8_rearLinkForce = NO_VAL_8;
  i16_frontDraft = i16_rearDraft = NO_VAL_16S;
  ui32_lastMaintainPowerRequest = 0;
  b_maintainEcuPower = b_maintainActuatorPower = b_maintainPowerForImplInTransport
    = b_maintainPowerForImplInPark = b_maintainPowerForImplInWork = false;

	b_isoFilterCreated = false;
	#endif
  i32_lastCalendarSet = 0;

  // set configure values with call for config
  config(rpc_gtp, rt_mySendSelection);
};
/** every subsystem of IsoAgLib has explicit function for controlled shutdown
  */
void Base_c::close( void ) {
  if ( ! checkAlreadyClosed() ) {
    // avoid another call
    setAlreadyClosed();
    // unregister from timeEvent() call by Scheduler_c
    getSchedulerInstance4Comm().unregisterClient( this );
  }
};

/**
  deliver reference to data pkg as reference to CANPkgExt_c
  to implement the base virtual function correct
*/
__IsoAgLib::CANPkgExt_c& Base_c::dataBase()
{
  return c_data;
}


/**
  config the Base_c object after init -> set pointer to gtp and
  config send/receive of different base msg types
  @param rpc_gtp pointer to the GETY_POS variable of the ersponsible member instance (pointer enables automatic value update if var val is changed)
	@param rt_mySendSelection optional Bitmask of base data to send ( default send nothing )
*/
void Base_c::config(GetyPos_c* rpc_gtp, IsoAgLib::BaseDataGroup_t rt_mySendSelection)
{
  // set configure values
  pc_gtp = rpc_gtp;
	t_mySendSelection = rt_mySendSelection;


  // set ui8_sendGtp to the pointed value, if pointer is valid
  if ((rpc_gtp != NULL) && ( ( t_mySendSelection & IsoAgLib::BaseDataGroup1   ) != 0 ) ) c_sendBase1Gtp = *rpc_gtp;
  else c_sendBase1Gtp.setUnspecified();
  if ((rpc_gtp != NULL) && ( ( t_mySendSelection & IsoAgLib::BaseDataGroup2   ) != 0 ) ) c_sendBase2Gtp = *rpc_gtp;
  else c_sendBase2Gtp.setUnspecified();
  if ((rpc_gtp != NULL) && ( ( t_mySendSelection & IsoAgLib::BaseDataCalendar ) != 0 ) ) c_sendCalendarGtp = *rpc_gtp;
  else c_sendCalendarGtp.setUnspecified();

  if ((rpc_gtp != NULL) && ( ( t_mySendSelection & IsoAgLib::BaseDataGroup3   ) != 0 ) ) c_sendBase3Gtp = *rpc_gtp;
  else c_sendBase3Gtp.setUnspecified();
  if ((rpc_gtp != NULL) && ( ( t_mySendSelection & IsoAgLib::BaseDataFuel     ) != 0 ) ) c_sendFuelGtp = *rpc_gtp;
  else c_sendFuelGtp.setUnspecified();
};

/**
  process received base msg and store updated value for later reading access;
  called by FilterBox_c::processMsg after receiving a msg

  possible errors:
      * LibErr_c::LbsBaseSenderConflict base msg recevied from different member than before
  @see FilterBox_c::processMsg
  @see CANIO_c::processMsg
*/
bool Base_c::processMsg(){
  bool b_result = false;
  GetyPos_c c_tempGtp( 0xF, 0xF );
  uint16_t ui16_actTime100ms = (data().time() / 100);

  #if defined(USE_ISO_11783) && defined(USE_DIN_9684)
  if (c_data.identType() == Ident_c::ExtendedIdent)
  #endif // USE_DIN_9684 && USE_ISO_11783
  #ifdef USE_ISO_11783
  { // an ISO11783 base information msg received
    return isoProcessMsg();
  }
  #endif // USE_ISO_11783
  #if defined(USE_ISO_11783) && defined(USE_DIN_9684)
  else
  #endif
  #ifdef USE_DIN_9684
  { // a DIN9684 base information msg received
    // store the gtp of the sender of base data
    if (getDinMonitorInstance4Comm().existDinMemberNr(data().send()))
    { // the corresponding sender entry exist in the monitor list
      c_tempGtp = getDinMonitorInstance4Comm().dinMemberNr(data().send()).gtp();
    }

    // interprete data accordingto BABO
    switch (data().babo())
    {
      case 4: // base data 1: speed, dist
        // only take values, if i am not the regular sender
        // and if actual sender isn't in conflict to previous sender
        if (
            ( ( t_mySendSelection & IsoAgLib::BaseDataGroup1   ) == 0 ) // I'm not the sender
         && ( // one of the following conditions must be true
             (c_sendBase1Gtp == c_tempGtp) // actual sender equivalent to last
          || (c_sendBase1Gtp.isUnspecified() ) // last sender has not correctly claimed address member
          || ((ui16_actTime100ms - ui8_lastBase1) > 10) // last sender isn't active any more
            )
           )
        { // sender is allowed to send
          // real speed
          setSpeedReal(data().val12());
          // theor speed
          setSpeedTheor(data().val34());
          // real dist -> react on 16bit int16_t overflow
          setOverflowSecure(i32_distReal, i16_lastDistReal, data().val56());
          // theor dist -> react on 16bit int16_t overflow
          setOverflowSecure(i32_distTheor, i16_lastDistTheor, data().val78());

          // set last time
          ui8_lastBase1 = ui16_actTime100ms;
          c_sendBase1Gtp = c_tempGtp;
        }
        else
        { // there is a sender conflict
          getLbsErrInstance().registerError( LibErr_c::LbsBaseSenderConflict, LibErr_c::LbsBase );
        }
        b_result = true;
        break;
      case 5: // base data 2: pto, hitch
        // only take values, if i am not the regular sender
        // and if actual sender isn't in conflict to previous sender
        if (
            ( ( t_mySendSelection & IsoAgLib::BaseDataGroup2 ) == 0 ) // I'm not the sender
         && ( // one of the following conditions must be true
             (c_sendBase2Gtp == c_tempGtp) // actual sender equivalent to last
          || (c_sendBase2Gtp.isUnspecified() ) // last sender has not correctly claimed address member
          || ((ui16_actTime100ms - ui8_lastBase2) > 10) // last sender isn't active any more
            )
           )
        { // sender is allowed to send
          // rear pto
          setPtoRear(data().val12());
          // front pto
          setPtoFront(data().val34());
          // engine speed
          setEngine(data().val56());
          // rear hitch
          setHitchRear(data().val7());
          // front hitch
          setHitchFront(data().val8());

          // set last time
          ui8_lastBase2 = ui16_actTime100ms;
          c_sendBase2Gtp = c_tempGtp;
        }
        else
        { // there is a sender conflict
          getLbsErrInstance().registerError( LibErr_c::LbsBaseSenderConflict, LibErr_c::LbsBase );
        }
        b_result = true;
        break;
      case 6: // NEW!! base data 3: rear draft
        // only take values, if i am not the regular sender
        // and if actual sender isn't in conflict to previous sender
        if (
            ( ( t_mySendSelection & IsoAgLib::BaseDataGroup3   ) == 0 ) // I'm not the sender
         && ( // one of the following conditions must be true
             (c_sendBase3Gtp == c_tempGtp) // actual sender equivalent to last
          || (c_sendBase2Gtp.isUnspecified()) // last sender was no correct announced member
          || (ui16_actTime100ms - ui8_lastBase3 > 10) // last sender isn't active any more
            )
           )
        { // sender is allowed to send
					// reaer left draft
          i16_rearLeftDraft = data().val12();
          // reaer right draft
          i16_rearRightDraft = data().val34();
          // reaer total draft Newton
          i16_rearDraftNewton = data().val56();
          // reaer total draft Nominal
          ui8_rearDraftNominal = data().val7();

          // set last time
          ui8_lastBase3 = ui16_actTime100ms;
          c_sendBase3Gtp = c_tempGtp;
        }
        else
        { // there is a sender conflict
          getLbsErrInstance().registerError( LibErr_c::LbsBaseSenderConflict, LibErr_c::LbsBase );
        }
        break;
      case 0xC: // NEW!! fuel consumption
        // only take values, if i am not the regular sender
        // and if actual sender isn't in conflict to previous sender
        if (
            ( ( t_mySendSelection & IsoAgLib::BaseDataFuel ) == 0 ) // I'm not the sender
         && ( // one of the following conditions must be true
             (c_sendFuelGtp == c_tempGtp) // actual sender equivalent to last
          || (c_sendFuelGtp.isUnspecified()) // last sender was no correct announced member
          || (ui16_actTime100ms - ui8_lastFuel > 10) // last sender isn't active any more
            )
           )
        { // sender is allowed to send
					// fuel rate
          i16_fuelRate =  data().val12();
          // fuel temperature
          ui8_fuelTemperature =  data().val3();

          // set last time
          ui8_lastFuel = ui16_actTime100ms;
          c_sendFuelGtp = c_tempGtp;
        }
        else
        { // there is a sender conflict
          getLbsErrInstance().registerError( LibErr_c::LbsBaseSenderConflict, LibErr_c::LbsBase );
        }
        break;
      case 0xF: // calendar
        // only take values, if i am not the regular sender
        // and if actual sender isn't in conflict to previous sender
        if (
            ( ( t_mySendSelection & IsoAgLib::BaseDataCalendar ) == 0 ) // I'm not the sender
         && ( // one of the following conditions must be true
             (c_sendCalendarGtp == c_tempGtp) // actual sender equivalent to last
          || (c_sendCalendarGtp.isUnspecified() ) // last sender has not correctly claimed address member
          || ((ui16_actTime100ms - ui8_lastBase2) > 10) // last sender isn't active any more
          || ( year() == 0 ) // current date is not valid - as year is 0 -> maybe this data is better
            )
           )
        { // sender is allowed to send
          // only store date with valid year ( != 0 ) or if last received
          // year is also 0
          if ( ( data().val1() != 0 ) || ( data().val2() != 0 ) || ( year() == 0 ) )
          { // store new calendar setting
            setCalendar(
              bcd2dec(data().val1()) * 100 + bcd2dec(data().val2()),
              bcd2dec(data().val3()), bcd2dec(data().val4()),
              bcd2dec(data().val5()), bcd2dec(data().val6()),
              bcd2dec(data().val7())
             );
          }
          // only handle this data source as reference data source, if year is valid
          // ( i.e. year != 0 )
          if ( ( data().val1() != 0 ) || ( data().val2() != 0 ) )
          { // set last time
            ui8_lastCalendar = ui16_actTime100ms;
            c_sendCalendarGtp = c_tempGtp;
          }
        }
        else
        { // there is a sender conflict
          getLbsErrInstance().registerError( LibErr_c::LbsBaseSenderConflict, LibErr_c::LbsBase );
        }
        b_result = true;
        break;
    }
  }
  #endif
  return b_result;
};


/**
  functions with actions, which must be performed periodically
  -> called periodically by Scheduler_c
  ==> sends base msg if configured in the needed rates

  possible errors:
      * dependant error in CANIO_c on CAN send problems
  @see CANPkg_c::getData
  @see CANPkgExt_c::getData
  @see CANIO_c::operator<<
  @return true -> all planned activities performed in allowed time
*/
bool Base_c::timeEvent( void ) {
  CANIO_c& c_can = getCanInstance4Comm();

  if ( Scheduler_c::getAvailableExecTime() == 0 ) return false;
  uint8_t ui8_actTime100ms = (Scheduler_c::getLastTimeEventTrigger()/100);

	checkCreateReceiveFilter();
  if ( Scheduler_c::getAvailableExecTime() == 0 ) return false;


	#ifdef USE_ISO_11783
  ISOMonitor_c& c_isoMonitor = getIsoMonitorInstance4Comm();
  if ((pc_gtp != NULL)&& (c_isoMonitor.existIsoMemberGtp(*pc_gtp))
       && (c_isoMonitor.isoMemberGtp(*pc_gtp).itemState(IState_c::ClaimedAddress)))
  { // stored base information sending ISO member has claimed address
    isoTimeEvent();
  }
  if ( Scheduler_c::getAvailableExecTime() == 0 ) return false;
  #endif
  #ifdef USE_DIN_9684
  DINMonitor_c& c_din_monitor = getDinMonitorInstance4Comm();
  if ((pc_gtp != NULL)&& (c_din_monitor.existDinMemberGtp(*pc_gtp))
       && (c_din_monitor.dinMemberGtp(*pc_gtp).itemState(IState_c::ClaimedAddress)))
  {
    // retreive the actual dynamic sender no of the member with the registered gtp
    uint8_t b_send = c_din_monitor.dinMemberGtp(*pc_gtp).nr();

		if ( ( ( CNAMESPACE::abs(ui8_actTime100ms - ui8_lastBase1 ) ) >= 1 )
			&& ( ( t_mySendSelection & IsoAgLib::BaseDataGroup1 ) != 0       ) )
    { // send actual base1 data
      c_sendBase1Gtp = *pc_gtp;
      data().setBabo(4);
      data().setSend(b_send);
      data().setVal12(i16_speedReal);
      data().setVal34(i16_speedTheor);
      data().setVal56(long2int(i32_distReal));
      data().setVal78(long2int(i32_distTheor));

      // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
      // then it sends the data
      c_can << data();

      // update time
      ui8_lastBase1 = ui8_actTime100ms;
    }

		if ( ( ( CNAMESPACE::abs(ui8_actTime100ms - ui8_lastBase2 ) ) >= 1 )
			&& ( ( t_mySendSelection & IsoAgLib::BaseDataGroup2 ) != 0       ) )
    { // send actual base2 data
      c_sendBase2Gtp = *pc_gtp;
      data().setBabo(5);
      data().setSend(b_send);
      data().setVal12(i16_ptoRear);
      data().setVal34(i16_ptoFront);
      data().setVal56(i16_engine);
      data().setVal7(hitchRear());
      data().setVal8(hitchFront());

      // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
      // then it sends the data
      c_can << data();

      // update time
      ui8_lastBase2 = ui8_actTime100ms;
    }

		if ( ( ( CNAMESPACE::abs(ui8_actTime100ms - ui8_lastBase3 ) ) >= 1 )
			&& ( ( t_mySendSelection & IsoAgLib::BaseDataGroup3 ) != 0       ) )
    { // send actual base3 data
      c_sendBase3Gtp = *pc_gtp;
      data().setBabo(6);
      data().setSend(b_send);
      data().setVal12(i16_rearLeftDraft);
      data().setVal34(i16_rearRightDraft);
      data().setVal56(i16_rearDraftNewton);
      data().setVal7(ui8_rearDraftNominal);

      // CAN_IO::operator<< retreives the information with the help of CAN_Pkg::get_data
      // then it sends the data
      c_can << data();

      // update time
      ui8_lastBase3 = ui8_actTime100ms;
    }

		if ( ( ( CNAMESPACE::abs(ui8_actTime100ms - ui8_lastFuel ) ) >= 1 )
			&& ( ( t_mySendSelection & IsoAgLib::BaseDataFuel ) != 0        ) )
    { // send actual base3 data
      c_sendFuelGtp = *pc_gtp;
      data().setBabo(0xC);
      data().setSend(b_send);
      data().setVal12(i16_fuelRate);
      data().setVal3(ui8_fuelTemperature);

      // CAN_IO::operator<< retreives the information with the help of CAN_Pkg::get_data
      // then it sends the data
      c_can << data();

      // update time
      ui8_lastFuel = ui8_actTime100ms;
    }

		if (
        (
         ( ( ui8_actTime100ms - ui8_lastCalendar) >= 10 )
      || (
			     ( ( ( 0xFF - ui8_lastCalendar ) + ui8_actTime100ms ) >= 10 )
			  && ( ui8_actTime100ms < ui8_lastCalendar )
				 )
        )
       && ( ( t_mySendSelection & IsoAgLib::BaseDataCalendar ) != 0 ) )
    { // send actual calendar data
      c_sendCalendarGtp = *pc_gtp;
      data().setBabo(0xF);
      data().setSend(b_send);
			data().setVal1(dec2bcd(year() / 100));
      data().setVal2(dec2bcd(year() % 100));
      data().setVal3(dec2bcd(month()));
      data().setVal4(dec2bcd(day()));
      data().setVal5(dec2bcd(hour()));
      data().setVal6(dec2bcd(minute()));
      data().setVal7(dec2bcd(second()));

      // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
      // then it sends the data
      c_can << data();

      // update time
      ui8_lastCalendar = ui8_actTime100ms;
    }
  }
  #endif
  return true;
}

/** check if filter boxes shall be created - create only ISO or DIN filters based
		on active local idents which has already claimed an address
		--> avoid to much Filter Boxes
	*/
void Base_c::checkCreateReceiveFilter( void )
{
  SystemMgmt_c& c_systemMgmt = getSystemMgmtInstance();
  CANIO_c &c_can = getCanInstance4Comm();
  #ifdef USE_DIN_9684
	if ( ( !b_dinFilterCreated ) && (c_systemMgmt.existActiveLocalDinMember() ) )
	{ // check if needed receive filters for DIN are active
		b_dinFilterCreated = true;
		// filter for base data 1
		c_can.insertFilter(*this, (0x7F << 4),(0x14 << 4), false);
		// filter for base data 2
		c_can.insertFilter(*this, (0x7F << 4),(0x15 << 4), false);
		// NEW filter for base data 3
		c_can.insertFilter(*this, (0x7F << 4),(0x16 << 4), false);
		// filter for lower priority base data fuel consumption & base data data calendar
		c_can.insertFilter(*this, (0x7C << 4),(0x1C << 4), true);
	}
	#endif
  #ifdef USE_ISO_11783

	if ( ( ! b_isoFilterCreated ) && ( c_systemMgmt.existActiveLocalIsoMember() ) )
	{ // check if needed receive filters for ISO are active
		b_isoFilterCreated = true;
		// create FilterBox_c for PGN TIME_DATE_PGN, PF 254 - mask for DP, PF and PS
		// mask: (0x1FFFF << 8) filter: (TIME_DATE_PGN << 8)
		c_can.insertFilter(*this, (static_cast<MASK_TYPE>(0x1FFFF) << 8),
											(static_cast<MASK_TYPE>(TIME_DATE_PGN) << 8), false, Ident_c::ExtendedIdent);
		// create FilterBox_c for PGN GROUND_BASED_SPEED_DIST_PGN, PF 254 - mask for DP, PF and PS
		// mask: (0x1FFFF << 8) filter: (TIME_DATE_PGN << 8)
		c_can.insertFilter(*this, (static_cast<MASK_TYPE>(0x1FFFF) << 8),
											(static_cast<MASK_TYPE>(GROUND_BASED_SPEED_DIST_PGN) << 8), false, Ident_c::ExtendedIdent);
		// create FilterBox_c for PGN WHEEL_BASED_SPEED_DIST_PGN, PF 254 - mask for DP, PF and PS
		// mask: (0x1FFFF << 8) filter: (TIME_DATE_PGN << 8)
		c_can.insertFilter(*this, (static_cast<MASK_TYPE>(0x1FFFF) << 8),
											(static_cast<MASK_TYPE>(WHEEL_BASED_SPEED_DIST_PGN) << 8), false, Ident_c::ExtendedIdent);
		// create FilterBox_c for PGN FRONT_HITCH_STATE_PGN, PF 254 - mask for DP, PF and PS
		// mask: (0x1FFFF << 8) filter: (TIME_DATE_PGN << 8)
		c_can.insertFilter(*this, (static_cast<MASK_TYPE>(0x1FFFF) << 8),
											(static_cast<MASK_TYPE>(FRONT_HITCH_STATE_PGN) << 8), false, Ident_c::ExtendedIdent);
		// create FilterBox_c for PGN BACK_HITCH_STATE_PGN, PF 254 - mask for DP, PF and PS
		// mask: (0x1FFFF << 8) filter: (TIME_DATE_PGN << 8)
		c_can.insertFilter(*this, (static_cast<MASK_TYPE>(0x1FFFF) << 8),
											(static_cast<MASK_TYPE>(BACK_HITCH_STATE_PGN) << 8), false, Ident_c::ExtendedIdent);
		// create FilterBox_c for PGN FRONT_PTO_STATE_PGN, PF 254 - mask for DP, PF and PS
		// mask: (0x1FFFF << 8) filter: (TIME_DATE_PGN << 8)
		c_can.insertFilter(*this, (static_cast<MASK_TYPE>(0x1FFFF) << 8),
											(static_cast<MASK_TYPE>(FRONT_PTO_STATE_PGN) << 8), false, Ident_c::ExtendedIdent);
		// create FilterBox_c for PGN BACK_PTO_STATE_PGN, PF 254 - mask for DP, PF and PS
		// mask: (0x1FFFF << 8) filter: (TIME_DATE_PGN << 8)
		c_can.insertFilter(*this, (static_cast<MASK_TYPE>(0x1FFFF) << 8),
											(static_cast<MASK_TYPE>(BACK_PTO_STATE_PGN) << 8), false, Ident_c::ExtendedIdent);

		// create FilterBox_c for PGN GPS_STATE_PGN, PF 254 - mask for DP, PF and PS
		// mask: (0x1FFFF << 8) filter: (GPS_STATE_PGN << 8)
		c_can.insertFilter(*this, (static_cast<MASK_TYPE>(0x1FFFF) << 8),
											(static_cast<MASK_TYPE>(GPS_STATE_PGN) << 8), false, Ident_c::ExtendedIdent);
		// create FilterBox_c for PGN GPS_LATITUDE_LONGITUDE_PGN, PF 254 - mask for DP, PF and PS
		// mask: (0x1FFFF << 8) filter: (GPS_LATITUDE_LONGITUDE_PGN << 8)
		c_can.insertFilter(*this, (static_cast<MASK_TYPE>(0x1FFFF) << 8),
											(static_cast<MASK_TYPE>(GPS_LATITUDE_LONGITUDE_PGN) << 8), false, Ident_c::ExtendedIdent);
		// create FilterBox_c for PGN GPS_SPEED_HEADING_ALTITUDE_PGN, PF 254 - mask for DP, PF and PS
		// mask: (0x1FFFF << 8) filter: (GPS_SPEED_HEADING_ALTITUDE_PGN << 8)
		c_can.insertFilter(*this, (static_cast<MASK_TYPE>(0x1FFFF) << 8),
											(static_cast<MASK_TYPE>(GPS_SPEED_HEADING_ALTITUDE_PGN) << 8), true, Ident_c::ExtendedIdent);

	}
	#endif
}


#ifdef USE_ISO_11783


/**
  process a ISO11783 base information PGN
*/
bool Base_c::isoProcessMsg()
{
  bool b_result = false;
  GetyPos_c c_tempGtp( 0xF, 0xF );
  uint16_t ui16_actTime100ms = (data().time()/100);
  // store the gtp of the sender of base data
  if (getIsoMonitorInstance4Comm().existIsoMemberNr(data().isoSa()))
  { // the corresponding sender entry exist in the monitor list
    c_tempGtp = getIsoMonitorInstance4Comm().isoMemberNr(data().isoSa()).gtp();
  }

  switch (data().isoPgn() & 0x1FFFF)
  {
    case TIME_DATE_PGN:
      // time - date
      // only take values, if i am not the regular sender
      // and if actual sender isn't in conflict to previous sender
      if (
          ( ( t_mySendSelection & IsoAgLib::BaseDataCalendar ) == 0 ) // I'm not the sender
       && ( // one of the following conditions must be true
           (c_sendCalendarGtp == c_tempGtp) // actual sender equivalent to last
        || (c_sendCalendarGtp.isUnspecified() ) // last sender has not correctly claimed address member
        || ((ui16_actTime100ms - ui8_lastIsoBase2) > 10) // last sender isn't active any more
          )
         )
      { // sender is allowed to send
        // store new calendar setting
        setCalendar(
          (data().val6() + 1985), data().val4(), (data().val5() / 4), (data().val3() + data().val8()),
           (data().val2() + data().val7()), (data().val1() / 4));
        // set last time
        ui8_lastIsoCalendar = ui16_actTime100ms;
        c_sendCalendarGtp = c_tempGtp;
      }
      else
      { // there is a sender conflict
        getLbsErrInstance().registerError( LibErr_c::LbsBaseSenderConflict, LibErr_c::LbsBase );
      }
      b_result = true;
      break;
    case GROUND_BASED_SPEED_DIST_PGN:
    case WHEEL_BASED_SPEED_DIST_PGN:
      // only take values, if i am not the regular sender
      // and if actual sender isn't in conflict to previous sender
      if (
          ( ( t_mySendSelection & IsoAgLib::BaseDataGroup1 ) == 0 ) // I'm not the sender
       && ( // one of the following conditions must be true
           (c_sendBase1Gtp == c_tempGtp) // actual sender equivalent to last
        || (c_sendBase1Gtp.isUnspecified() ) // last sender has not correctly claimed address member
        || ((ui16_actTime100ms - ui8_lastIsoBase1) > 10) // last sender isn't active any more
          )
         )
      { // sender is allowed to send
        int16_t i16_tempSpeed = data().val12();
        switch (data().val8() & 3 ) {
         case 0: // driving reverse
          if ( (i16_tempSpeed != ERROR_VAL_16S)
            && (i16_tempSpeed != NO_VAL_16S) ) i16_tempSpeed *= -1;
          break;
         case 1: // driving forward
          break;
         case 2: // ERROR
          i16_tempSpeed = ERROR_VAL_16S;
          break;
         case 3: // not available
          i16_tempSpeed = NO_VAL_16S;
          break;
        }

        if (data().isoPgn() == GROUND_BASED_SPEED_DIST_PGN)
        { // real speed
          setSpeedReal(i16_tempSpeed);
          // real dist
          setDistReal(static_cast<int32_t>(data().val36()));
        }
        else
        { // wheel based speed
          setSpeedTheor(i16_tempSpeed);
          // wheel based dist
          setDistTheor(static_cast<int32_t>(data().val36()));
          // additionally scan for key switch and maximum power time
          t_keySwitch = IsoAgLib::IsoActiveFlag_t( ( data().val8() >> 2 ) & 3 );
          ui8_maxPowerTime = data().val7();
        }
        // set last time
        ui8_lastIsoBase1 = ui16_actTime100ms;
        c_sendBase1Gtp = c_tempGtp;
      }
      else
      { // there is a sender conflict
        getLbsErrInstance().registerError( LibErr_c::LbsBaseSenderConflict, LibErr_c::LbsBase );
      }
      b_result = true;
      break;
    case FRONT_HITCH_STATE_PGN:
    case BACK_HITCH_STATE_PGN:
      // only take values, if i am not the regular sender
      // and if actual sender isn't in conflict to previous sender
      if (
          ( ( t_mySendSelection & IsoAgLib::BaseDataGroup2 ) == 0 ) // I'm not the sender
       && ( // one of the following conditions must be true
           (c_sendBase2Gtp == c_tempGtp) // actual sender equivalent to last
        || (c_sendBase2Gtp.isUnspecified() ) // last sender has not correctly claimed address member
        || ((ui16_actTime100ms - ui8_lastIsoBase2) > 10) // last sender isn't active any more
          )
         )
      { // sender is allowed to send
        uint8_t ui8_tempHitch = ((data().val1() * 4) / 10);
        if ( (ui8_tempHitch != ERROR_VAL_8)
            && (ui8_tempHitch != NO_VAL_8) ) {
          switch ( (data().val2() >> 6) & 3 ) {
           case 0: // not in work
            break;
           case 1: // in work
            ui8_tempHitch |= 0x80;
            break;
           case 2: // Error
            ui8_tempHitch = ERROR_VAL_8S;
            break;
           case 3: // Not available
            ui8_tempHitch = NO_VAL_8;
            break;
          }
        }
        if (data().isoPgn() == FRONT_HITCH_STATE_PGN)
        { // front hitch
          setHitchFront(ui8_tempHitch);
          ui8_frontLinkForce = data().val3();
          i16_frontDraft = static_cast<int16_t>(data().val4()) + (static_cast<int16_t>(data().val5()) << 8);
        }
        else
        { // back hitch
          setHitchRear(ui8_tempHitch);
          ui8_rearLinkForce = data().val3();
          i16_rearDraft = static_cast<int16_t>(data().val4()) + (static_cast<int16_t>(data().val5()) << 8);
        }
        // set last time
        ui8_lastIsoBase2 = ui16_actTime100ms;
        c_sendBase2Gtp = c_tempGtp;
      }
      else
      { // there is a sender conflict
        getLbsErrInstance().registerError( LibErr_c::LbsBaseSenderConflict, LibErr_c::LbsBase );
      }
      b_result = true;
      break;
    case FRONT_PTO_STATE_PGN:
    case BACK_PTO_STATE_PGN:
      // only take values, if i am not the regular sender
      // and if actual sender isn't in conflict to previous sender
      if (
          ( ( t_mySendSelection & IsoAgLib::BaseDataGroup2 ) == 0 ) // I'm not the sender
       && ( // one of the following conditions must be true
           (c_sendBase2Gtp == c_tempGtp) // actual sender equivalent to last
        || (c_sendBase2Gtp.isUnspecified() ) // last sender has not correctly claimed address member
        || ((ui16_actTime100ms - ui8_lastIsoBase2) > 10) // last sender isn't active any more
          )
         )
      { // sender is allowed to send
        if (data().isoPgn() == FRONT_PTO_STATE_PGN)
        { // front PTO
          setPtoFront(data().val12());
          t_frontPtoEngaged = IsoAgLib::IsoActiveFlag_t( (data().val5() >> 6) & 3);
          t_frontPto1000 = IsoAgLib::IsoActiveFlag_t( (data().val5() >> 4) & 3);
          t_frontPtoEconomy = IsoAgLib::IsoActiveFlag_t( (data().val5() >> 2) & 3);
        }
        else
        { // back PTO
          setPtoRear(data().val12());
          t_rearPtoEngaged = IsoAgLib::IsoActiveFlag_t( (data().val5() >> 6) & 3);
          t_rearPto1000 = IsoAgLib::IsoActiveFlag_t( (data().val5() >> 4) & 3);
          t_rearPtoEconomy = IsoAgLib::IsoActiveFlag_t( (data().val5() >> 2) & 3);
        }
        // set last time
        ui8_lastIsoBase2 = ui16_actTime100ms;
        c_sendBase2Gtp = c_tempGtp;
      }
      else
      { // there is a sender conflict
        getLbsErrInstance().registerError( LibErr_c::LbsBaseSenderConflict, LibErr_c::LbsBase );
      }
      b_result = true;
      break;
    case MAINTAIN_POWER_REQUEST_PGN: // maintain power request
      if ( ( (data().val1() >> 2) & 3) == 1)
        b_maintainEcuPower = true;
      if ( ( (data().val1() >> 4) & 3) == 1)
        b_maintainActuatorPower = true;


      if ( ( (data().val2() >> 6) & 3) == 1)
        b_maintainPowerForImplInTransport = true;
      if ( ( (data().val2() >> 4) & 3) == 1)
        b_maintainPowerForImplInPark = true;
      if ( ( (data().val2() >> 2) & 3) == 1)
        b_maintainPowerForImplInWork = true;

      ui32_lastMaintainPowerRequest = data().time();
      b_result = true;
      break;
		case GPS_STATE_PGN:
			/** \todo check for correct GPS mode position in CAN msg - try with Byte1 */
			t_gpsMode = IsoAgLib::IsoGpsRecMode_t( data().val1() );
      b_result = true;
			break;
		case GPS_LATITUDE_LONGITUDE_PGN:
			/** \todo check if open accessible example for Long/Lat was a correct source */
			i32_latitudeRaw  = data().getInt32Data( 0 );
			i32_longitudeRaw = data().getInt32Data( 4 );
      b_result = true;
			break;
		case GPS_SPEED_HEADING_ALTITUDE_PGN:
			/** \todo check and correct decoding of GPS-Speed, GPS-Heading and Altitude */
			i16_speedGps   = data().getInt16Data( 0 );
			i16_headingGps = data().getInt16Data( 2 );
			ui32_altitude  = data().getUint32Data( 4 );
      b_result = true;
			break;
  }
  return b_result;
}
/**
  send a ISO11783 base information PGN
*/
bool Base_c::isoTimeEvent( void )
{
  CANIO_c& c_can = getCanInstance4Comm();

  if ( Scheduler_c::getAvailableExecTime() == 0 ) return false;
  uint8_t ui8_actTime100ms = (Scheduler_c::getLastTimeEventTrigger()/100);
  if ( Scheduler_c::getAvailableExecTime() == 0 ) return false;

  // retreive the actual dynamic sender no of the member with the registered gtp
  uint8_t b_sa = getIsoMonitorInstance4Comm().isoMemberGtp(*pc_gtp).nr();
  data().setIdentType(Ident_c::ExtendedIdent);
  data().setIsoPri(3);
  data().setIsoSa(b_sa);

	if ( ( ( CNAMESPACE::abs(ui8_actTime100ms - ui8_lastIsoBase1 ) ) >= 1    )
		&& ( ( t_mySendSelection & IsoAgLib::BaseDataGroup1 ) != 0 ) )
  { // send actual base1 data: ground/wheel based speed/dist
    c_sendBase1Gtp = *pc_gtp;
    data().setIsoPgn(GROUND_BASED_SPEED_DIST_PGN);
    data().setVal12(CNAMESPACE::abs(i16_speedReal));
    data().setVal36(i32_distReal);
    switch (i16_speedReal) {
     case ERROR_VAL_16S:
      data().setVal8(IsoAgLib::IsoError);
      break;
     case NO_VAL_16S:
      data().setVal8(IsoAgLib::IsoNotAvailable);
      break;
     default:
      if (i16_speedReal < 0) data().setVal8(0);
      else data().setVal8(1);
      break;
    }
    // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
    // then it sends the data
    c_can << data();

    data().setIsoPgn(WHEEL_BASED_SPEED_DIST_PGN);
    data().setVal12(CNAMESPACE::abs(i16_speedTheor));
    data().setVal36(i32_distTheor);

    data().setVal7(ui8_maxPowerTime);

    uint8_t b_val8 = IsoAgLib::IsoInactive;
    switch (i16_speedTheor) {
     case ERROR_VAL_16S:
      b_val8 |= IsoAgLib::IsoError;
      break;
     case NO_VAL_16S:
      b_val8 |= IsoAgLib::IsoNotAvailable;
      break;
     default:
      if (i16_speedTheor >= 0) b_val8 |= IsoAgLib::IsoActive;
      break;
    }
    b_val8 |= (t_keySwitch << 2);
    data().setVal8(b_val8);
    // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
    // then it sends the data
    c_can << data();


    // update time
    ui8_lastIsoBase1 = ui8_actTime100ms;
  }

	if ( ( ( CNAMESPACE::abs(ui8_actTime100ms - ui8_lastIsoBase2 ) ) >= 1    )
		&& ( ( t_mySendSelection & IsoAgLib::BaseDataGroup2 ) != 0 ) )
  { // send actual base2 data
    c_sendBase2Gtp = *pc_gtp;
    data().setIsoPgn(FRONT_HITCH_STATE_PGN);
    switch (hitchFront()) {
     case ERROR_VAL_16S:
      data().setVal1(hitchFront());
      data().setVal2(IsoAgLib::IsoError);
      break;
     case NO_VAL_16S:
      data().setVal1(hitchFront());
      data().setVal2(IsoAgLib::IsoNotAvailable);
      break;
     default:
      data().setVal1(((hitchFront() & 0x7F) * 10) / 4);
      if ((hitchFront() & 0x80) != 0) data().setVal2(1 << 6); // work
      else data().setVal2(1);
      break;
    }
    data().setVal3(ui8_frontLinkForce);
    data().setVal4(i16_frontDraft& 0xFF);
    data().setVal5(i16_frontDraft >> 8);
    // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
    // then it sends the data
    c_can << data();

    data().setIsoPgn(BACK_HITCH_STATE_PGN);
    switch (hitchRear()) {
     case ERROR_VAL_16S:
      data().setVal1(hitchRear());
      data().setVal2(IsoAgLib::IsoError);
      break;
     case NO_VAL_16S:
      data().setVal1(hitchRear());
      data().setVal2(IsoAgLib::IsoNotAvailable);
      break;
     default:
      data().setVal1(((hitchRear() & 0x7F) * 10) / 4);
      if ((hitchRear() & 0x80) != 0) data().setVal2(1 << 6); // work
      else data().setVal2(1);
      break;
    }
    data().setVal3(ui8_rearLinkForce);
    data().setVal4(i16_rearDraft& 0xFF);
    data().setVal5(i16_rearDraft >> 8);

    // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
    // then it sends the data
    c_can << data();

    data().setIsoPgn(FRONT_PTO_STATE_PGN);
    data().setVal12(ptoFront());
    data().setVal12(NO_VAL_16);

    uint8_t ui8_val5 = (t_frontPtoEngaged << 6);
    ui8_val5 |= (t_frontPto1000 << 4);
    ui8_val5 |= (t_frontPtoEconomy << 2);
    data().setVal5(ui8_val5);
    // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
    // then it sends the data
    c_can << data();

    data().setIsoPgn(BACK_PTO_STATE_PGN);
    data().setVal12(ptoRear());
    data().setVal12(NO_VAL_16);

    ui8_val5 = (t_frontPtoEngaged << 6);
    ui8_val5 |= (t_frontPto1000 << 4);
    ui8_val5 |= (t_frontPtoEconomy << 2);
    data().setVal5(ui8_val5);
    // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
    // then it sends the data
    c_can << data();

    // update time
    ui8_lastIsoBase2 = ui8_actTime100ms;
  }
  if ( Scheduler_c::getAvailableExecTime() == 0 ) return false;

  if (
      (
       ((ui8_actTime100ms - ui8_lastIsoCalendar) >= 10)
    || (((0xFF - ui8_lastCalendar) + ui8_actTime100ms) >= 10)
      )
     && ( ( t_mySendSelection & IsoAgLib::BaseDataCalendar ) != 0 )
     )
  { // send actual calendar data
    c_sendCalendarGtp = *pc_gtp;
    isoSendCalendar(*pc_gtp);
  }
  return true;
}

/**
  send ISO11783 calendar PGN
  @param rc_gtp GETY_POS code off calling item which wants to send
  @param ri32_time timestamp where calendar was last sent (default autodetect)

  possible errors:
      * dependant error in CANIO_c on CAN send problems
  @see CANPkg_c::getData
  @see CANPkgExt_c::getData
  @see CANIO_c::operator<<
*/
void Base_c::isoSendCalendar(GetyPos_c rc_gtp)
{
  if ( ( ( t_mySendSelection & IsoAgLib::BaseDataCalendar ) != 0 )
		&& ( c_sendCalendarGtp == rc_gtp                   ) )
  { // this item (identified by GETY_POS is configured to send
    data().setIsoPgn(TIME_DATE_PGN);
    data().setVal1(second() * 4);
    data().setVal2(minute());
    data().setVal3(hour());
    data().setVal4(month());
    data().setVal5(day() * 4);
    data().setVal6(year() - 1985);
    data().setVal78(0);

    // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
    // then it sends the data
    getCanInstance4Comm() << data();

    // update time
    ui8_lastIsoCalendar = Scheduler_c::getLastTimeEventTrigger()/100;
  }
}
/** force maintain power from tractor
  * @param rb_ecuPower true -> maintain ECU power
  * @param rb_actuatorPower -> maintain actuator power
  * @param rt_implTransport true -> implement is in transport state
  * @param rt_implPark true -> implement is in park state
  * @param rt_implWork true -> implement is in work state
  */
void Base_c::forceMaintainPower( bool rb_ecuPower, bool rb_actuatorPower, IsoAgLib::IsoActiveFlag_t rt_implTransport,
  IsoAgLib::IsoActiveFlag_t rt_implPark, IsoAgLib::IsoActiveFlag_t rt_implWork)
{
  uint8_t val1 = IsoAgLib::IsoInactive,
          val2 = IsoAgLib::IsoInactive;
  if (rb_ecuPower)      val1 |= ( IsoAgLib::IsoActive << 6);
  if (rb_actuatorPower) val1 |= ( IsoAgLib::IsoActive << 4);

  val2 |= ( rt_implTransport << 6);
  val2 |= ( rt_implPark << 4);
  val2 |= ( rt_implWork << 2);

  data().setIsoPgn(MAINTAIN_POWER_REQUEST_PGN);
  data().setVal1(val1);
  data().setVal2(val2);
  data().setVal34(0);
  data().setVal56(0);
  data().setVal78(0);
  // CANIO_c::operator<< retreives the information with the help of CANPkg_c::getData
  // then it sends the data
  getCanInstance4Comm() << data();
}

#endif




/**
  update distance values ; react on int16_t overflow
  @param reflVal to be updated value as int32_t variable
  @param refiVal to be updated value as 16bit int16_t variable
  @param rrefiNewVal new value given as reference to 16bit int
*/
void Base_c::setOverflowSecure(int32_t& reflVal, int16_t& refiVal, const int16_t& rrefiNewVal)
{
  // definition area [0..i32_maxVal] with overflow
  // 10000 of Fendt; 32767 of DIN
  const int32_t i32_maxDefFendt = 10000,
                i32_maxDefDin = 32767;
  int32_t i32_newValFendt = reflVal,
          i32_newValDin = reflVal;
  int16_t i16_diff = rrefiNewVal - refiVal;

  // check if there was an overflow = diff is greater than half of def area (per sign side)
  if ((CNAMESPACE::abs(i16_diff) > i32_maxDefFendt/2) || (CNAMESPACE::abs(i16_diff) > i32_maxDefDin/2))
  { // one of the overflow checks triggers
    if (CNAMESPACE::abs(i16_diff) > i32_maxDefFendt/2)
    { // the old wrong fendt limit triggers
      if (rrefiNewVal > refiVal)
      { // dist decreased lower than 0 -> lower underflow
        i32_newValFendt -= refiVal; // max reducable before underflow
        i32_newValFendt -= (i32_maxDefFendt - rrefiNewVal); // decreased after underflow (jump to int16 max pos val)
      }
      else
      { // dist increased upper than 0x7FFF -> overflow
        i32_newValFendt += (i32_maxDefFendt - refiVal); // max decrease before overflow
        i32_newValFendt += rrefiNewVal; // decreased after overflow
      }
    }
    if (CNAMESPACE::abs(i16_diff) > i32_maxDefDin/2)
    { // the correct DIN limit triggers
      if (rrefiNewVal > refiVal)
      { // dist decreased lower than 0 -> lower underflow
        i32_newValDin -= refiVal; // max reducable before underflow
        i32_newValDin -= (i32_maxDefDin - rrefiNewVal); // decreased after underflow (jump to int16 max pos val)
      }
      else
      { // dist increased upper than 0x7FFF -> overflow
        i32_newValDin += (i32_maxDefDin - refiVal); // max decrease before overflow
        i32_newValDin += rrefiNewVal; // decreased after overflow
      }
    }
    // check, which new value should be used -> take the value which mean less distance to old val
    if (CNAMESPACE::labs(i32_newValFendt - reflVal) < CNAMESPACE::labs(i32_newValDin - reflVal))
    {
      reflVal = i32_newValFendt;
    }
    else
    {
      reflVal = i32_newValDin;
    }
  }
  else
  { // there's no overflow -> easy increment reflVal
    reflVal += i16_diff;
  }
  // store the given int16 val as last in16 val in refiVal
  refiVal = rrefiNewVal;
};

/**
  deliver the gtp of the sender of the base data

  possible errors:
      * Err_c::range rui8_typeNr doesn't match valid base msg type number
  @param rt_typeGrp base msg type no of interest: BaseDataGroup1 | BaseDataGroup2 | BaseDataCalendar
  @return GETY_POS code of member who is sending the intereested base msg type
*/
GetyPos_c Base_c::senderGtp(IsoAgLib::BaseDataGroup_t rt_typeGrp) {
  GetyPos_c c_result( 0xF, 0xF );
	// simply answer first matching result if more than one type is selected
	if ( ( rt_typeGrp & IsoAgLib::BaseDataGroup1   ) != 0 ) return c_sendBase1Gtp;
	if ( ( rt_typeGrp & IsoAgLib::BaseDataGroup2   ) != 0 ) return c_sendBase2Gtp;
	if ( ( rt_typeGrp & IsoAgLib::BaseDataGroup3   ) != 0 ) return c_sendBase3Gtp;
	if ( ( rt_typeGrp & IsoAgLib::BaseDataFuel     ) != 0 ) return c_sendFuelGtp;
	if ( ( rt_typeGrp & IsoAgLib::BaseDataCalendar ) != 0 ) return c_sendCalendarGtp;
	else return c_result;
}

/**
  translate BCD to normal value
*/
uint8_t bcd2dec(uint8_t rb_bcd)
{
  return ((rb_bcd >> 4) * 10) + (rb_bcd & 0xF);
}
/**
  translate normal value to BCD
*/
uint8_t dec2bcd(uint8_t rb_dec)
{
	const uint8_t ui8_v10 = rb_dec / 10;
	const uint8_t ui8_v0  = rb_dec % 10;
	const uint8_t ui8_result = ( ui8_v10 << 4 ) + ui8_v0;
  return ui8_result;
}

/**
  set the real (radar measured) driven distance with int32_t val
  @param rreflVal value to store as real radar measured distance
*/
void Base_c::setDistReal(const int32_t& rreflVal)
{
  i32_distReal = rreflVal;
  // set the int16_t value regarding the overflow counting
  i16_lastDistReal = long2int(rreflVal);
};

/**
  set the theoretical (gear calculated) driven distance with int32_t val
  @param rreflVal value to store as theoretical (gear calculated) driven distance
*/
void Base_c::setDistTheor(const int32_t& rreflVal)
{
  i32_distTheor = rreflVal;
  // set the int16_t value regarding the overflow counting
  i16_lastDistTheor = long2int(rreflVal);
};

/**
  set the calendar value
  @param ri16_year value to store as year
  @param rb_month value to store as month
  @param rb_day value to store as day
  @param rb_hour value to store as hour
  @param rb_minute value to store as minute
  @param rb_second value to store as second
*/
void Base_c::setCalendar(int16_t ri16_year, uint8_t rb_month, uint8_t rb_day, uint8_t rb_hour, uint8_t rb_minute, uint8_t rb_second)
{
  i32_lastCalendarSet = System_c::getTime();
  setYear(ri16_year);
  setMonth(rb_month);
  setDay(rb_day);
  setHour(rb_hour);
  setMinute(rb_minute);
  setSecond(rb_second);
};

/**
  get int16_t overflowed val from long
  @param rreflVal value as int32_t (32bit) variable
  @return 16bit int16_t calculated with counting overflow from 32767 to (-32766)
*/
int16_t Base_c::long2int(const int32_t& rreflVal)
{
  // Version mit [0..10000] und Uberlauf a la Fendt Vario
  //int32_t i32_mod =  rreflVal % 10000;
  // Version mit [0..32767] und Uberlauf a la Scheduler_c DIN 9684
  int32_t i32_mod =  rreflVal % 32767;

  return i32_mod;
};
} // end of namespace __IsoAgLib
