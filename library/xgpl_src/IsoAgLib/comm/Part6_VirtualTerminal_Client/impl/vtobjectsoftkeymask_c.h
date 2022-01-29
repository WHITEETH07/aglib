/*
  vtobjectsoftkeymask_c.h

  (C) Copyright 2009 - 2019 by OSB AG

  See the repository-log for details on the authors and file-history.
  (Repository information can be found at <http://isoaglib.com/download>)

  Usage under Commercial License:
  Licensees with a valid commercial license may use this file
  according to their commercial license agreement. (To obtain a
  commercial license contact OSB AG via <http://isoaglib.com/en/contact>)

  Usage under GNU General Public License with exceptions for ISOAgLib:
  Alternatively (if not holding a valid commercial license)
  use, modification and distribution are subject to the GNU General
  Public License with exceptions for ISOAgLib. (See accompanying
  file LICENSE.txt or copy at <http://isoaglib.com/download/license>)
*/
#ifndef VTOBJECTSOFTKEYMASK_C_H
#define VTOBJECTSOFTKEYMASK_C_H

#include "vtobject_c.h"
#include "vtclient_c.h"
#include "vtclientconnection_c.h"


namespace __IsoAgLib {

class vtObjectSoftKeyMask_c : public vtObject_c
{
#ifdef ENABLE_SKM_HANDLER
public:
  class iSkmHandler_c
  {
  public:
    virtual ~iSkmHandler_c() {}
  public:
    virtual void fitTerminal() = 0;
    virtual uint8_t numberOfObjectsToFollow() const = 0;
    virtual IsoAgLib::iVtObject_c* objectsToFollow( uint16_t nrObject ) const = 0;
  };
#endif // ENABLE_SKM_HANDLER

public:
  int16_t stream(uint8_t* destMemory,
                 uint16_t maxBytes,
                 objRange_t sourceOffset);

  vtObjectSoftKeyMask_c(const iVtObjectSoftKeyMask_s* vtObjectSoftKeyMaskSROM , int ai_multitonInst);

#ifdef ENABLE_SKM_HANDLER
  void registerSkmHandler_c( iSkmHandler_c* _SkmHandler );
  void unRegisterSkmHandler_c( iSkmHandler_c* _SkmHandler );
#endif // ENABLE_SKM_HANDLER

  iVtObjectSoftKeyMask_s* get_vtObjectSoftKeyMask_a() { return dynamic_cast<iVtObjectSoftKeyMask_s *>(&(get_vtObject_a())); }

  virtual ~vtObjectSoftKeyMask_c();

  uint32_t fitTerminal() const;

  uint8_t get_numberOfObjectsToFollow() const;

  void setOriginSKM(bool b_SKM);

  // //////////////////////////////////
  // All special Attribute-Set methods
  void setBackgroundColour(uint8_t newValue, bool b_updateObject=false, bool b_enableReplaceOfCmd=false) {
    saveValue8SetAttribute ((b_updateObject) ? MACRO_getStructOffset(get_vtObjectSoftKeyMask_a(), backgroundColour) : 0, sizeof(iVtObjectSoftKeyMask_s), 1, newValue, __IsoAgLib::getVtClientInstance4Comm().getClientByID(s_properties.clientId).getUserConvertedColor (newValue, this, IsoAgLib::BackgroundColour), b_enableReplaceOfCmd);
  }
#ifdef USE_ISO_TERMINAL_GETATTRIBUTES
  // ///////////////////////// getter for attributes
  /** that attribute is in parentheses in the spec, so commented out here
  uint8_t updateObjectType() const { return 4; }
  */

  uint8_t updateBackgroundColour(bool b_SendRequest=false);

  void saveReceivedAttribute (uint8_t attrID, uint8_t* pui8_attributeValue);
#endif

#ifdef ENABLE_SKM_HANDLER
private:
  mutable iSkmHandler_c* m_SkmHandler;
  mutable bool b_autoGenerated_SkmHandler;
#endif // ENABLE_SKM_HANDLER
};

} //__IsoAgLib
#endif
