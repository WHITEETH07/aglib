/***************************************************************************
    hal_can_interface.h - CAN interface between BIOS/OS and IsoAgLib
                          to concentrate CAN handling abstraction
                          within one module
    -------------------
    date                 : 18.06.2007
    copyright            : (c) 2007 GESAS GmbH
    email                : stefan.klueh@gesas:de
    type                 : Header
 ***************************************************************************/

#ifndef _HAL_AMS5_CAN_INTERFACE_H_
#define _HAL_AMS5_CAN_INTERFACE_H_

#include <IsoAgLib/hal/ams5/typedef.h>
#include <IsoAgLib/hal/ams5/errcodes.h>

namespace __IsoAgLib { class Ident_c; class CanPkg_c;}


namespace __HAL
{
   /* ************************************************** */
   /** \name Global Status Per BUS
    *  Functions for status check of global CAN BUS      */
   /* ************************************************** */
   /*@{*/

   /**
      test if the CAN BUS is in WARN state
      @param aui8_busNr number of the BUS to check (default 0)
      @return true == CAN BUS is in WARN state, else normal operation
   */
   bool can_stateGlobalWarn(uint8_t aui8_busNr = 0);

   /**
      test if the CAN BUS is in OFF state
      @param aui8_busNr number of the BUS to check (default 0)
      @return true == CAN BUS is in OFF state, else normal operation
   */
   bool can_stateGlobalOff(uint8_t aui8_busNr = 0);

   /**
      test if the CAN BUS is in Blocked state, which can be a sign
      for CAN controllers which are configured with different baudrates;
      this is the case if neither succesfull sent nor received msg
      is detcted AND CAN controller is in WARN or OFF state
      (the time since last succ. send/rec and the time of WARN/OFF
      can be defined with CONFIG_CAN_MAX_CAN_ERR_TIME_BEFORE_SLOWERING
      in the application specific config file isoaglib_config
      -> should not be to short to avoid false alarm)
      @param aui8_busNr number of the BUS to check (default 0)
      @return true == CAN BUS is in blocked state, else normal operation
   */
   bool can_stateGlobalBlocked(uint8_t aui8_busNr = 0);


#ifdef USE_CAN_MEASURE_BUSLOAD
   /**
      deliver the baudrate of the CAN BUS in [kbaud]
      @param aui8_busNr number of the BUS to check (default 0)
      @return BUS load of the last second [kbaud]
   */
   int32_t can_stateGlobalBusload(uint8_t aui8_busNr = 0);
#endif

   /**
      check if a send try on this BUS caused an Bit1Error
      (occurs if >1 controller try to send msg with same ident)
      ==> ISO 11783 forces stop of retries in this case for
          adress claim
          @param aui8_busNr number of the BUS to check
          @return true -> Bit1Err occured
   */
   bool can_stateGlobalBit1err(uint8_t aui8_busNr = 0);

   /*@}*/

   /* ************************************************** */
   /** \name Specific for one MsgObj
    *  Functions for status check of a single MsgObj     */
   /* ************************************************** */
   /*@{*/

   /**
      deliver the timestamp of last successfull CAN send action
      @param aui8_busNr number of the BUS to check
      @param aui8_msgobjNr number of the MsgObj to check
      @return timestamp of last successful send
              OR -1 if aui8_msgObjNr corresponds to no valid send obj
   */
   int32_t can_stateMsgobjTxok(uint8_t aui8_busNr, uint8_t aui8_msgobjNr);


   /**
      test if buffer of a MsgObj is full (e.g. no more
      msg can be put into buffer (important for TX objects))
      @param aui8_busNr number of the BUS to check
      @param aui8_msgobjNr number of the MsgObj to check
      @return true -> buffer is full -> no further can_send allowed
   */
   bool can_stateMsgobjOverflow(uint8_t aui8_busNr, uint8_t aui8_msgobjNr);

   /**
      deliver amount of messages in buffer
      (interesting for RX objects)
      @param aui8_busNr number of the BUS to check
      @param aui8_msgobjNr number of the MsgObj to check
      @return number of messages in buffer or negative error code:
              HAL_CONFIG_ERR == BUS not initialised
              HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_stateMsgobjBuffercnt(uint8_t aui8_busNr, uint8_t aui8_msgobjNr);

   /**
      deliver amount of messages which can be placed in buffer
      (interesting for TX objects)
      @param aui8_busNr number of the BUS to check
      @param aui8_msgobjNr number of the MsgObj to check
      @return number of messages which can be placed in buffer or negative error code:
              HAL_CONFIG_ERR == BUS not initialised
              HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_stateMsgobjFreecnt(uint8_t aui8_busNr, uint8_t aui8_msgobjNr);

   /*@}*/

   /* ************************************************** */
   /** \name Global Configuration Per BUS
    *  Functions to configure the global CAN BUS         */
   /* ************************************************** */
   /*@{*/

   /**
      init CAN BUS with given baudrate and masks
      (11bit, 29bit and last msg mask)
      @param aui8_busNr number of the BUS to init (default 0)
      @param ab_baudrate baudrate in [kbaud]
      @param aui16_maskStd 11bit global mask
      @param aui32_maskExt 29bit global mask
      @param aui32_maskLastmsg mask of last CAN msg
      @return HAL_NO_ERR == no error;
              HAL_RANGE_ERR == wrong BUS nr or wrong baudrate;
              HAL_WARN_ERR == BUS previously initialised - no problem if only masks had to be changed
   */
   int16_t can_configGlobalInit(uint8_t aui8_busNr, uint16_t ab_baudrate, uint16_t aui16_maskStd, uint32_t aui32_maskExt, uint32_t aui32_maskLastmsg);

   /**
      set the global masks
      (11bit, 29bit and last msg mask)
      @param aui8_busNr number of the BUS to config (default 0)
      @param aui16_maskStd 11bit global mask
      @param aui32_maskExt 29bit global mask
      @param aui32_maskLastmsg mask of last CAN msg
      @return HAL_NO_ERR == no error;
              HAL_RANGE_ERR == wrong BUS nr
   */
   int16_t can_configGlobalMask(uint8_t aui8_busNr, uint16_t aui16_maskStd, uint32_t aui32_maskExt, uint32_t aui32_maskLastmsg);

   /**
      close CAN BUS
      (important to close BUS before re-init with other baudrate)
      @param aui8_busNr number of the BUS to close (default 0)
      @return HAL_NO_ERR == no error;
              HAL_RANGE_ERR == wrong BUS nr;
              HAL_CONFIG_ERR == BUS previously not initialised
   */
   int16_t can_configGlobalClose(uint8_t aui8_busNr);

   /**  wait until specified timeout or until next CAN message receive
     *  @return true -> there are CAN messages waiting for process. else: return due to timeout
     */
   bool can_waitUntilCanReceiveOrTimeout(uint16_t aui16_timeoutInterval);

   /*@}*/

   /* ************************************************** */
   /** \name Configuration specific for one MsgObj
    *  Functions to configure a specific MsgObj          */
   /* ************************************************** */
   /*@{*/

   /**
      config a MsgObj
      (buffer size is defined locally in implementation of this interface;
      all received msgs should get a time stamp;
      class Ident_c has ident and type 11/29bit)
      @param aui8_busNr number of the BUS to config
      @param aui8_msgobjNr number of the MsgObj to config
      @param arc_ident filter ident of this MsgObj
      @param ab_rxtx 0==RX receive; 1==TX transmit
      @return HAL_NO_ERR == no error;
           C_BUSY == this MsgObj is already used
           HAL_CONFIG_ERR == BUS not initialised or error during buffer allocation
           HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_configMsgobjInit(uint8_t aui8_busNr, uint8_t aui8_msgobjNr, __IsoAgLib::Ident_c& arc_ident, uint8_t ab_rxtx);


   /**
      close a MsgObj
      @param aui8_busNr number of the BUS to config
      @param aui8_msgobjNr number of the MsgObj to config
      @return HAL_NO_ERR == no error;
              HAL_CONFIG_ERR == BUS not initialised, MsgObj previously not used or buffer memory not freed
              HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_configMsgobjClose(uint8_t aui8_busNr, uint8_t aui8_msgobjNr);

   /*@}*/

   /* ******************************************** */
   /** \name Use of MsgObj
    *  Functions to use a specific MsgObj          */
   /* ******************************************** */
   /*@{*/

   /**
      send a message via a MsgObj;
      CanPkg_c (or derived object) must provide (virtual)
      functions:
      * Ident_c& getIdent() -> deliver ident of msg to send
      * void getData(MASK_TYPE& rt_ident, uint8_t& rui8_identType,
                     uint8_t& rb_dlcTarget, uint8_t* pb_dataTarget)
        -> put DLC in referenced r_dlc and insert data in uint8_t string pb_data
      @param aui8_busNr number of the BUS to config
      @param aui8_msgobjNr number of the MsgObj to config
      @param apc_data pointer to CanPkg_c instance with data to send
      @return HAL_NO_ERR == no error;
              HAL_CONFIG_ERR == BUS not initialised, MsgObj is no send object
              HAL_NOACT_ERR == BUS OFF
              HAL_OVERFLOW_ERR == send buffer overflowed
              HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_useMsgobjSend(uint8_t aui8_busNr, uint8_t aui8_msgobjNr, __IsoAgLib::CanPkg_c* apc_data);

   /**
      clear th buffer of a MsgObj (e.g. to stop sending retries)
      @param aui8_busNr number of the BUS to config
      @param aui8_msgobjNr number of the MsgObj to config
      @return HAL_NO_ERR == no error;
              HAL_CONFIG_ERR == BUS not initialised
              HAL_RANGE_ERR == wrong BUS or MsgObj number
   */
   int16_t can_useMsgobjClear(uint8_t aui8_busNr, uint8_t aui8_msgobjNr);

#ifdef USE_CAN_SEND_DELAY_MEASUREMENT
   int32_t can_getMaxSendDelay(uint8_t aui8_busNr);
#endif

   /*@}*/
}
#endif