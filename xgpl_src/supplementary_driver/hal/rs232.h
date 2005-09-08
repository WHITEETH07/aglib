/***************************************************************************
                          rs232.h - include dependent on used target (defined
													           in Application_Config/isoaglib_config.h)
																		 the suitable HAL specific header for
																		 RS232 communication
                             -------------------
    begin                : Sun Mar 09 2003
    copyright            : (C) 2003 - 2004 Dipl.-Inform. Achim Spangler
    email                : a.spangler@osb-ag:de
    type                 : Header
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
 * Copyright (C) 1999 - 2004 Dipl.-Inform. Achim Spangler                  *
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

/* ************************************************************ */
/** \file supplementary_driver/hal/rs232.h
  * include dependent on used target (defined in
	  Application_Config/isoaglib_config.h) the suitable HAL
		specific header for RS232 communication.
*/
/* ************************************************************ */
#ifndef _HAL_INDEPENDEND_RS232_H_
#define _HAL_INDEPENDEND_RS232_H_

// include interface aplication relevant configuration settings
// #include <Application_Config/isoaglib_config.h>
#include <IsoAgLib/hal/config.h>


// now include dependent on used target the suitable header
#if defined(SYSTEM_PC)
	#include "pc/rs232/rs232.h"
#elif defined(SYSTEM_ESX)
	#include "esx/rs232/rs232.h"
#elif defined(SYSTEM_ESXu)
	#include "esxu/rs232/rs232.h"
#elif defined(SYSTEM_C2C)
	#include "c2c/rs232/rs232.h"
#elif defined(SYSTEM_IMI)
	#include "imi/rs232/rs232.h"
#elif defined(SYSTEM_PM167)
	#include "pm167/rs232/rs232.h"
#elif defined(SYSTEM_MITRON167)
	#include "mitron167/rs232/rs232.h"
#endif


#endif
