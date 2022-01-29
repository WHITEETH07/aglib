/*
 * ivtobjectoutputstring_c.cpp
 *
 *  Created on: 29.01.2022
 *      Author: franz
 */


#include "ivtobjectoutputstring_c.h"

namespace IsoAgLib {


    iVtObjectOutputString_c::iVtObjectOutputString_c(
    		int ai_multitonInst,
			ObjectID ID,
			uint16_t width,
			uint16_t height,
			uint8_t backgroundColour,
			iVtObjectFontAttributes_c *fontAttributes,
			uint8_t options,
			iVtObjectStringVariable_c *variableReference,
			uint8_t horizontalJustification,
			char *value)
            :vtObjectOutputString_c(
            new iVtObjectOutputString_s(
                    ID,
                    width,
                    height,
                    backgroundColour,
                    fontAttributes,
                    options,
                    variableReference,
                    horizontalJustification,
					strlen(value),
                    value /* size length+1 (0-termination intern!) */
            )
            ,ai_multitonInst)
    {}

    uint16_t iVtObjectOutputString_c::objectType() { return VT_OBJECT_TYPE_OUTPUT_STRING; }

    uint16_t iVtObjectOutputString_c::getObjectType() const { return objectType(); }

    void iVtObjectOutputString_c::setSize(uint16_t newWidth, uint16_t newHeight, bool b_updateObject,
                                          bool b_enableReplaceOfCmd) {
        vtObjectOutputString_c::setSize(newWidth, newHeight, b_updateObject, b_enableReplaceOfCmd);
    }

    void iVtObjectOutputString_c::setHorizontalJustification(uint8_t newValue, bool b_updateObject,
                                                             bool b_enableReplaceOfCmd) {
        vtObjectOutputString_c::setHorizontalJustification (newValue, b_updateObject, b_enableReplaceOfCmd);
    }

    void iVtObjectOutputString_c::setWidth(uint16_t newValue, bool b_updateObject, bool b_enableReplaceOfCmd) {
        vtObjectOutputString_c::setWidth (newValue, b_updateObject, b_enableReplaceOfCmd);
    }

    void iVtObjectOutputString_c::setHeight(uint16_t newValue, bool b_updateObject, bool b_enableReplaceOfCmd) {
        vtObjectOutputString_c::setHeight (newValue, b_updateObject, b_enableReplaceOfCmd);
    }


}
