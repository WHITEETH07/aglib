/* *************************************************************************
                          1_5_ReadGps.cpp - tutorial for reading GPS data
                             -------------------
    begin                : Fri Feb 16 16:42:00 CEST 2007

    copyright            : (C) 2000 - 2007 by Dipl.-Inform. Achim Spangler
    email                : a.spangler@osb-ag:de
 ***************************************************************************/

/* *************************************************************************
 * This example main application is published NOT as GPL`ed Open Source,   *
 * so that you can include this source file in propietary closed projects. *
 * Everybody is encouraged to use the examples in the examples    *
 * directory for a quick and easy start of development for IsoAgLib        *
 * applications.                                                           *
 *                                                                         *
 * So nobody is bound to publish changes of the example source files.      *
 * But every changed example, which could help learning the use of the     *
 * GPLed "IsoAgLib", is invited to be sent to the original author, so      *
 * that it can be published with the other examples.                       *
 ***************************************************************************/

/* *************************************************************************
 *                                                                         *
 * This is an example of "IsoAgLib", an object oriented program library    *
 * to serve as a software layer between application specific program and   *
 * communication protocol details. By providing simple function calls for  *
 * jobs like starting a measuring program for a process data value on a    *
 * remote ECU, the main program has not to deal with single CAN telegram   *
 * formatting. This way communication problems between ECU's which use     *
 * this library should be prevented.                                       *
 * Everybody and every company is invited to use this library to make a    *
 * working plug and play standard out of the printed protocol standard.    *
 *                                                                         *
 * Copyright (C) 1999 - 2007 Dipl.-Inform. Achim Spangler                  *
 *                                                                         *
 * The "IsoAgLib" is free software; you can redistribute it and/or         *
 * modify it under the terms of the GNU Lesser General Public License as   *
 * published by the Free Software Foundation; either version 2 of the      *
 * License, or (at your option) any later version.                         *
 *                                                                         *
 * The "IsoAgLib" is distributed in the hope that it will be useful, but   *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with IsoAgLib; if not, write to the Free Software Foundation,     *
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111_1307  USA           *
 ***************************************************************************/

/* *********************************************************************** */
/** \example 1_5_ReadGps.cpp
 * This tutorial shall provide the simple base program, which reads GPS
 * position and time data from the ISOBUS and simply displays it on
 * the console. Later to be used in conjunction with a VT-client maybe.
 *
 * <H1>What is demonstrated</H1>
 * <ol>
 * <li>Receive data (GPS position and direction) from the bus without sending any (=implement mode)
 * <ul>
 *  <li>Data storage class IsoAgLib::iTimePosGPS_c
 *  <li>configure to receive with IsoAgLib::iTimePosGps_c::config()
 * </ul>
 * <li>Trigger periodic activities of ISO<i><sub>AgLib</sub></i>
 * <ul>
 *   <li>Core class IsoAgLib::iScheduler_c for scheduling of all periodic activities
 *   <li>Method IsoAgLib::iScheduler_c::timeEvent() which can
 *   <ul>
 *     <li>Perform activities until defined al_endTime is reached, which is important for scheduling purposes of whole system - call by IsoAgLib::iScheduler_c::timeEvent( al_endTime )
 *     <li>Process all received CAN messages until all receive buffers are empty -> simple call, but can lead to deadlock on to high CAN load
 *   </ul>
 * </ul>
 * <li>Create local identity, for which the ISO<i><sub>AgLib</sub></i> performs an address claim, so that the ECU can access the <b><i>ISO11783</i></b> BUS with IsoAgLib::iIdentItem_c and its constructor IsoAgLib::iIdentItem_c::iIdentItem_c
 * </ol>
 *
 * <H1>Where to look for further information</H1>
 * <ol>
 * <li>Overview on accessing cached main information at \ref BaseDataPage
 * <li>Overview information on Scheduling at \ref SchedulerPage
 * <li>Overview on System Management at \ref SystemMgmtPage
 * <li>Overview on global communication functions at \ref CommOverPage
 * </ol>
 *
 * <H1>Howto compile this example</H1>
 * <ol>
 * <li>Call the script update_makefile.sh with the spec file
 *     conf_1_5_ReadGps from the directory where the spec file
 *     and the script are located<br>
 *     <i>#> update_makefile.sh conf_1_5_ReadGps</i>
 * <li><b>Only Linux:</b><ul>
 *     <li><i>#>cd 1_5_ReadGps</i> to go to the subdirectory 1_5_ReadGps
 *     <li><i>#> make</i><br>
 *     <li>GNU make will then use the fresh created Makefile to create the executable 1_5_ReadGps in this directory
 *     </ul>
 * <li><b>Other Development Environments:</b><ul>
 *     <li>import the file list in filelist-1_5_ReadGps.txt into the IDE of your choice
 *     <li>set <i>-DPRJ_USE_AUTOGEN_CONFIG=config_1_5_ReadGps</i> as global project parameter
 *     <li>add target specific headers, sources or libraries to the project
 *     <li>let your IDE build the project
 *   </ul>
 * </ol>
 *
 * <H1>Needed Project Specification</H1>
 * The project specification, which is needed as input for
 * the generation script update_makefile.sh is described in
 * \ref PrjSpec1_5_ReadGps__pc_linux__simulating__simulating .
 *
 * The adapted project specifications for different setups:
 * <ul>
 * <li>Configuration Setups for Linux on PC:
 * <ul>
 *    <li> \ref PrjSpec1_5_ReadGps__pc_linux__rte__rte
 *    <li> \ref PrjSpec1_5_ReadGps__pc_linux__rte__simulating
 *    <li> \ref PrjSpec1_5_ReadGps__pc_linux__rte__sys
 *    <li> \ref PrjSpec1_5_ReadGps__pc_linux__simulating__rte
 *    <li> \ref PrjSpec1_5_ReadGps__pc_linux__simulating__simulating
 *    <li> \ref PrjSpec1_5_ReadGps__pc_linux__simulating__sys
 *  </ul>
 * <li>Configuration Setups for Win32 on PC:
 * <ul>
 *    <li> \ref PrjSpec1_5_ReadGps__pc_win32__vector_canlib__simulating
 *    <li> \ref PrjSpec1_5_ReadGps__pc_win32__vector_xl_drv_lib__simulating
 *    <li> \ref PrjSpec1_5_ReadGps__pc_win32__vector_canlib__sys
 *    <li> \ref PrjSpec1_5_ReadGps__pc_win32__vector_xl_drv_lib__sys
 *    <li> \ref PrjSpec1_5_ReadGps__pc_win32__simulating__simulating
 *    <li> \ref PrjSpec1_5_ReadGps__pc_win32__simulating__sys
 *  </ul>
 * <li>Configuration Setupts for some embedded targets:
 * <ul>
 *    <li> \ref PrjSpec1_5_ReadGps__esx__sys__sys
 *    <li> \ref PrjSpec1_5_ReadGps__imi__sys__sys
 *    <li> \ref PrjSpec1_5_ReadGps__pm167__sys__sys
 *  </ul>
 * </ul>
 *
 * <H1>Resulting Project File List</H1>
 * See \ref FileLists1_5_ReadGps__pc_linux__simulating__simulating for needed files
 * ( filelist-1_5_ReadGps-doxygen_import.txt ),
 *  with \ref SrcList1_5_ReadGps__pc_linux__simulating__simulating containing the needed sources
 *  and with \ref HdrList1_5_ReadGps__pc_linux__simulating__simulating containing the needed headers.
 *
 * The resulting file lists for different setups:
 * <ul>
 * <li>Configuration Setups for Linux on PC:
 * <ul>
 *    <li> \ref FileLists1_5_ReadGps__pc_linux__rte__rte
 *    <li> \ref FileLists1_5_ReadGps__pc_linux__rte__simulating
 *    <li> \ref FileLists1_5_ReadGps__pc_linux__rte__sys
 *    <li> \ref FileLists1_5_ReadGps__pc_linux__simulating__rte
 *    <li> \ref FileLists1_5_ReadGps__pc_linux__simulating__simulating
 *    <li> \ref FileLists1_5_ReadGps__pc_linux__simulating__sys
 *  </ul>
 * <li>Configuration Setups for Win32 on PC:
 * <ul>
 *    <li> \ref FileLists1_5_ReadGps__pc_win32__vector_canlib__simulating
 *    <li> \ref FileLists1_5_ReadGps__pc_win32__vector_xl_drv_lib__simulating
 *    <li> \ref FileLists1_5_ReadGps__pc_win32__vector_canlib__sys
 *    <li> \ref FileLists1_5_ReadGps__pc_win32__vector_xl_drv_lib__sys
 *    <li> \ref FileLists1_5_ReadGps__pc_win32__simulating__simulating
 *    <li> \ref FileLists1_5_ReadGps__pc_win32__simulating__sys
 *  </ul>
 * <li>Configuration Setupts for some embedded targets:
 * <ul>
 *    <li> \ref FileLists1_5_ReadGps__esx__sys__sys
 *    <li> \ref FileLists1_5_ReadGps__imi__sys__sys
 *    <li> \ref FileLists1_5_ReadGps__pm167__sys__sys
 *  </ul>
 * </ul>
 *
 * <H1>Resulting Project Configuration Header</H1>
 * This header is automatically included by xgpl_src/IsoAgLib/isoaglib_config.h
 * if the \#define PRJ_USE_AUTOGEN_CONFIG is set to
 * config_1_5_ReadGps ( see also at \ref PrjConfig1_5_ReadGps ).
 *                                                                         */
/* *************************************************************************/

/** the define PRJ_USE_AUTOGEN_CONFIG is used by xgpl_src/IsoAgLib/isoaglib_config.h
    to include project specific configuration settings.
    Set this define in the project file or Makefile of the whole
    project, so that each source file is compiled with this setting
  */
#ifndef PRJ_USE_AUTOGEN_CONFIG
  #define PRJ_USE_AUTOGEN_CONFIG config_1_5_ReadGps.h
#endif


// include the central interface header for the hardware adaptation layer part
// of the "IsoAgLib"

/* include some needed util headers */
//#include <IsoAgLib/util/config.h>

/* include headers for the needed drivers */
#include <IsoAgLib/driver/system/isystem_c.h>
#include <IsoAgLib/driver/can/icanio_c.h>

/* include the central interface header for the communication layer part
   of the "IsoAgLib" */
#include <IsoAgLib/comm/Scheduler/ischeduler_c.h>
#include <IsoAgLib/comm/Part5_NetworkManagement/iidentitem_c.h>
#include <IsoAgLib/comm/Part5_NetworkManagement//iisomonitor_c.h>
#include <IsoAgLib/comm/Part7_ApplicationLayer/itimeposgps_c.h>

#ifdef SYSTEM_PC
  #ifdef WIN32
    #include <windows.h>
  #endif
#endif


// the interface objects of the IsoAgLib are placed in the IsoAgLibAll namespace
// -> include all elements of this area for easy access
// with this command the text part "IsoAgLib::" can be avoided, which
// is needed for the documentation generator
using namespace IsoAgLib;



int main(int argc, char** argv)
{
  // Initialize CAN-Bus
  getIcanInstance().init (0); // CAN-Bus 0 with defaulting 250 kbit

  // Start address claim of the local identity/member
  iIdentItem_c c_myIdent (2,     // aui8_indGroup
                          1,     // aui8_devClass
                          5,     // aui8_devClassInst
                          25,    // ab_func
                          0x7FF, // aui16_manufCode
                          27);   // aui32_serNo
                          // further parameters use the default values as given in the constructor

  // configure BaseData_c to send nothing on BUS but receive data (=implement mode)
  iTimePosGps_c& rc_timePosGps = getITimePosGpsInstance();
  rc_timePosGps.config (&c_myIdent.isoName(), IsoAgLib::IdentModeImplement);

  /** IMPORTANT:
    - The following loop could be replaced of any repeating call of
      getISchedulerInstance().timeEvent();
      which is needed to perform all internal activities of the IsoAgLib.
    - Define the time intervall for getISchedulerInstance().timeEvent()
      in a way, that allows IsoAgLib to trigger all reactions on BUS
      in the ISO 11783 defined time resolution - especially the address
      claim process has some tight time restrictions, that suggest
      a trigger rate of at least 100msec ( you could call the function
      only during address claim, mask updload and other special
      circumstances in a high repetition rate )
    - The main loop is running until iSystem_c::canEn() is returning false.
      This function can be configured by the \#define CONFIG_BUFFER_SHORT_CAN_EN_LOSS_MSEC
      in isoaglib_config.h to ignore short CAN_EN loss.
    - This explicit control of power state without automatic powerdown on CanEn loss
      can be controled with the central config define
      \#define CONFIG_DEFAULT_POWERDOWN_STRATEGY IsoAgLib::PowerdownByExplcitCall
      or
      \#define CONFIG_DEFAULT_POWERDOWN_STRATEGY IsoAgLib::PowerdownOnCanEnLoss
      in the header xgpl_src/IsoAgLib/isoaglib_config.h
    - This can be also controlled during runtime with the function call:
      getIsystemInstance().setPowerdownStrategy( IsoAgLib::PowerdownByExplcitCall )
      or
      getIsystemInstance().setPowerdownStrategy( IsoAgLib::PowerdownOnCanEnLoss )
  */

  int32_t i32_idleTimeSpread;
  int32_t i32_nextTimeToPrint=0;
  while ( iSystem_c::canEn() )
  { // run main loop
    // IMPORTANT: call main timeEvent function for
    // all time controlled actions of IsoAgLib
    i32_idleTimeSpread = getISchedulerInstance().timeEvent();

    if (iSystem_c::getTime() >= i32_nextTimeToPrint)
    { // let's only print out information every second
      i32_nextTimeToPrint += 1000;
      EXTERNAL_DEBUG_DEVICE << "GPS Status at time " << (i32_nextTimeToPrint/1000) << " (seconds): ";

      if (rc_timePosGps.isPositionReceived())
      { // position received
        EXTERNAL_DEBUG_DEVICE << "Position received! ";
      }
      else
      { // no position received EVER or FOR THE LAST FEW SECONDS
        EXTERNAL_DEBUG_DEVICE << "No GPS Position!   ";
      }

      if (rc_timePosGps.isDirectionReceived())
      { // direction received
        EXTERNAL_DEBUG_DEVICE << "Direction received!";
      }
      else
      { // no direction received EVER or FOR THE LAST FEW SECONDS
        EXTERNAL_DEBUG_DEVICE << "No GPS Direction!  ";
      }

      if (rc_timePosGps.isPositionReceived())
      { // position received
        EXTERNAL_DEBUG_DEVICE << "  Pos (" << rc_timePosGps.getGpsPositionUpdateAge() <<" msec old) "
                              << "Lat="    << rc_timePosGps.getGpsLatitudeDegree10Minus7() << " "
                              << "Lon="    << rc_timePosGps.getGpsLongitudeDegree10Minus7();
      }
      if (rc_timePosGps.isDirectionReceived())
      { // direction received
        EXTERNAL_DEBUG_DEVICE << "  Dir ("      << rc_timePosGps.getGpsDirectionUpdateAge() <<" msec old) "
                              << "Speed(cm/s)=" << rc_timePosGps.getGpsSpeedCmSec() << "   "
                              << "Course="   << (((double (rc_timePosGps.getGpsCourseRad10Minus4())/10000) *360)/(2*3.1415926535));
      }

      EXTERNAL_DEBUG_DEVICE << EXTERNAL_DEBUG_DEVICE_ENDL;
    }

    // The following sleep mechanism uses idle_time of scheduler reported back
    if ( i32_idleTimeSpread > 0 ) IsoAgLib::iCanIo_c::waitUntilCanReceiveOrTimeout( i32_idleTimeSpread );

  }
  return 1;
}