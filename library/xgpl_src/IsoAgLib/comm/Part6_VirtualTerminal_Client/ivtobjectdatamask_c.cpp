//
// Created by franz on 28.01.22.
//


#include "ivtobjectdatamask_c.h"

namespace IsoAgLib {


	iVtObjectDataMask_c::iVtObjectDataMask_c() {}

    iVtObjectDataMask_c::iVtObjectDataMask_c(const iVtObject_c::iVtObjectDataMask_s *vtObjectDataMaskSROM,
                                             int ai_multitonInst)
            : vtObjectDataMask_c(vtObjectDataMaskSROM , ai_multitonInst)
    {}

    iVtObjectDataMask_c::iVtObjectDataMask_c(
    		int ai_multitonInst,
			uint16_t ID,
			uint8_t backgroundColour,
			iVtObjectSoftKeyMask_c *softKeyMask,
			uint8_t numberOfObjectsToFollow,
			const repeat_iVtObject_x_y_iVtObjectFontAttributes_row_col_s *objectsToFollow,
			uint8_t numberOfMacrosToFollow,
			const repeat_event_iVtObjectMacro_s *macrosToFollow)
    : iVtObjectDataMask_c(
    		new iVtObjectDataMask_s(
    				ID,
    				backgroundColour,
    				softKeyMask,
    				numberOfObjectsToFollow,
    				objectsToFollow,
    				numberOfMacrosToFollow,
    				macrosToFollow),
			ai_multitonInst)
    {

    }

    void iVtObjectDataMask_c::setSoftKeyMask(iVtObjectSoftKeyMask_c *newSoftKeyMask, bool b_updateObject,
                                             bool b_enableReplaceOfCmd) {
        vtObjectDataMask_c::setSoftKeyMask (newSoftKeyMask, b_updateObject, b_enableReplaceOfCmd);
    }

    void iVtObjectDataMask_c::setBackgroundColour(uint8_t newValue, bool b_updateObject, bool b_enableReplaceOfCmd) {
        vtObjectDataMask_c::setBackgroundColour (newValue, b_updateObject, b_enableReplaceOfCmd);
    }

    uint16_t iVtObjectDataMask_c::getObjectType() const { return objectType(); }

    bool iVtObjectDataMask_c::setChildPosition(IsoAgLib::iVtObject_c *apc_childObject, int16_t dx, int16_t dy,
                                               bool b_updateObject, bool b_enableReplaceOfCmd) {
        return vtObjectDataMask_c::setChildPosition(apc_childObject, dx, dy, b_updateObject, b_enableReplaceOfCmd);
    }

    bool iVtObjectDataMask_c::moveChildLocation(iVtObject_c *apc_childObject, int8_t dx, int8_t dy, bool b_updateObject) {
        return vtObjectDataMask_c::moveChildLocation (apc_childObject, dx, dy, b_updateObject);
    }

    const iVtObject_c::iVtObjectDataMask_s &iVtObjectDataMask_c::get_vtObjectDataMask_a() { return *vtObjectDataMask_c::get_vtObjectDataMask_a(); }

    uint16_t iVtObjectDataMask_c::objectType() { return VT_OBJECT_TYPE_DATA_MASK; }
}
