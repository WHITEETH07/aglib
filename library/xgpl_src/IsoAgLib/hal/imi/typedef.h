/***************************************************************************
                          typedef.h  -  system dependent basic typedef's
                             -------------------
    begin                : Thu Aug 08 2002
    copyright            : (C) 2002 - 2004 Dipl.-Inform. Achim Spangler
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
#ifndef TYPEDEF_IMI_H
#define TYPEDEF_IMI_H

// for Tasking 7.x EDE these includes have to be included FIRST
// because they are included by the broken stl-headers into global namespace
#if defined(__TSW_CPP__)
 #include <cstdlib>
 #include <cstddef>
 #include <cstring>
 typedef unsigned int size_t;
#endif



typedef unsigned char uint8_t;
typedef char           int8_t;
typedef unsigned int  uint16_t;
typedef int            int16_t;
typedef unsigned long uint32_t;
typedef long           int32_t;


#ifdef HANDLE_FAST_DATATYPE_AS_STRICT
/**  SIZEOF_INT = 2 */
/* Signed.  */
typedef signed char     int_fast8_t;
typedef int             int_fast16_t;
typedef long            int_fast32_t;
//typedef long long int   int_fast64_t;

/* Unsigned.  */
typedef unsigned char           uint_fast8_t;
typedef unsigned int            uint_fast16_t;
typedef unsigned long           uint_fast32_t;
//typedef unsigned long long int  uint_fast64_t;

#endif // end HANDLE_FAST_DATATYPE_AS_STRICT

#endif