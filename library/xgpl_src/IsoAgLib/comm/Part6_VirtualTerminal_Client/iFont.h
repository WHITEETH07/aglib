/*
 * iFont.h
 *
 *  Created on: 05.02.2022
 *      Author: franz
 */

#ifndef COMPONENTS_ISOAGLIB_LIBRARY_XGPL_SRC_ISOAGLIB_COMM_PART6_VIRTUALTERMINAL_CLIENT_IFONT_H_
#define COMPONENTS_ISOAGLIB_LIBRARY_XGPL_SRC_ISOAGLIB_COMM_PART6_VIRTUALTERMINAL_CLIENT_IFONT_H_

namespace IsoAgLib {

//Non-Proportional Font

enum Font : uint8_t {

	FONT6x8 = 0,
	FONT8x8 = 1,
	FONT8x12 = 2,
	FONT12x16 = 3,
	FONT16x16 = 4,
	FONT16x24 = 5,
	FONT24x32 = 6,
	FONT32x32 = 7,
	FONT32x48 = 8,
	FONT48x64 = 9,
	FONT64x64 = 10,
	FONT64x96 = 11,
	FONT96x128 = 12,
	FONT128x128 = 13,
	FONT128x192 = 14
};


}





#endif /* COMPONENTS_ISOAGLIB_LIBRARY_XGPL_SRC_ISOAGLIB_COMM_PART6_VIRTUALTERMINAL_CLIENT_IFONT_H_ */