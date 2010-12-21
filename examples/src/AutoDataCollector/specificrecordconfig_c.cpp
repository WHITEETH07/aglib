/***************************************************************************
                          specificrecordconfig_c.cpp  -  description
                             -------------------
    begin                : Tue Jul 18 2000
    copyright            : (C) 2000 by Dipl.-Inform. Achim Spangler
    email                : spangler@tec.agrar.tu-muenchen.de
    type                 : Source
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 * The "LBS Library" is an object oriented program library to serve as a   *
 * software layer between application specific program and communication   *
 * protocol details. By providing simple function calls for jobs like      *
 * starting a measuring program for a process data value on a remote ECU,  *
 * the main program has not to deal with single CAN telegram formatting.   *
 * This way communication problems between ECU's which use this library    *
 * should be prevented.                                                    *
 * Everybody and every company is invited to use this library to make a    *
 * working plug and play standard out of the printed protocol standard.    *
 *                                                                         *
 * Copyright (C) 1999  Dipl.-Inform. Achim Spangler                        *
 *                                                                         *
 * This library is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU Lesser General Public License as published   *
 * by the Free Software Foundation; either version 2 of the License, or    *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This library is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * Lesser General Public License for more details.                         *
 *                                                                         *
 * You should have received a copy of the GNU Lesser General Public License*
 * along with this library; if not, write to the Free Software Foundation, *
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA           *
 ***************************************************************************/

#include "specificrecordconfig_c.h"
#include <ctype.h>
#include <IsoAgLib/driver/eeprom/ieepromio_c.h>



/**
	constructor which initialises all data and can call init to read
	config data from EEPROM (if eeprom adress is given)
	@param aui16_eepromAdr adress in EEPROM where config data is stored
*/
SpecificRecordConfig_c::SpecificRecordConfig_c(uint16_t aui16_eepromAdr, DefaultRecordConfig_c* apc_defaultConfig, IsoAgLib::iDINItem_c* apc_memberItem)
{ // init vars
	ui16_eepromAdr = aui16_eepromAdr;

	ui16_procEepromAdr = 0;
	ui16_cachedProcEepromAdr = 0;
	i16_cachedProcInd = -1;
	memset(pui8_cachedTagName, '\0', TAG_NAME_LEN + 1);
	memset(pui8_cachedTagData, '\0', TAG_VALUE_LEN + 1);

	ui8_configDataCnt = 0;
	ui8_devClass = 0;
	ui8_devClassInst = 0;
	ui8_nameLen = 0;
	memset(pui8_name, 0, 11);

	// optional data
	ui8_timeWert = 0;
	ui8_workWertInst = 0;
	ui8_applrateRecording = 0;
	b_transportDummyWidth = false;
	b_transportWorkDist = false;
	b_fieldstarSend = false;
	ui8_timeDistDevClass = 0xFF;
	// default only measure prog for IMI and not even single measure request for other ECU
	ui8_useMeasureProgs = 1;
	c_recordAsISOName.setUnspecified();
	ui8_procCnt = 0;

	// check if pointers and eepromd are valid -> call init
	if ((ui16_eepromAdr != 0) && (apc_defaultConfig != NULL) && (apc_memberItem != NULL))
	{ // valid data -> call init
		init(ui16_eepromAdr, apc_defaultConfig, apc_memberItem);
	}
}
/**
	copy Constructor
	@param acrc_src reference to source instance
*/
SpecificRecordConfig_c::SpecificRecordConfig_c(const SpecificRecordConfig_c& acrc_src)
{
	ui16_eepromAdr = acrc_src.ui16_eepromAdr;

	ui16_procEepromAdr = acrc_src.ui16_procEepromAdr;
	ui16_cachedProcEepromAdr = acrc_src.ui16_cachedProcEepromAdr;
	i16_cachedProcInd = acrc_src.i16_cachedProcInd;
	memmove(pui8_cachedTagName, acrc_src.pui8_cachedTagName, TAG_NAME_LEN + 1);
	memmove(pui8_cachedTagData, acrc_src.pui8_cachedTagData, TAG_VALUE_LEN + 1);

	ui8_configDataCnt = acrc_src.ui8_configDataCnt;
	ui8_devClass = acrc_src.ui8_devClass;
	ui8_devClassInst = acrc_src.ui8_devClassInst;
	ui8_nameLen = acrc_src.ui8_nameLen;
	memmove(pui8_name, acrc_src.pui8_name, 11);

	// optional data
	ui8_timeWert = acrc_src.ui8_timeWert;
	ui8_workWertInst = acrc_src.ui8_workWertInst;
	ui8_applrateRecording = acrc_src.ui8_applrateRecording;
	b_transportDummyWidth = acrc_src.b_transportDummyWidth;
	b_transportWorkDist = acrc_src.b_transportWorkDist;
	b_fieldstarSend = acrc_src.b_fieldstarSend;
	ui8_useMeasureProgs = acrc_src.ui8_useMeasureProgs;
	c_recordAsISOName = acrc_src.c_recordAsISOName;
	ui8_procCnt = acrc_src.ui8_procCnt;
	ui8_timeDistDevClass = acrc_src.ui8_timeDistDevClass;
}
/**
	destructor
*/
SpecificRecordConfig_c::~SpecificRecordConfig_c()
{
	ui16_eepromAdr = 0;
}

/**
	utility function to get a string with only lower letters
	@param pui8_string string which is then transformed
	@param aui8_len length of string
*/
void tolowerStr(uint8_t* pui8_string, uint8_t aui8_len)
{
	for (uint8_t ui8_ind = 0; ui8_ind < aui8_len; ui8_ind++)
		pui8_string[ui8_ind] = tolower(pui8_string[ui8_ind]);
}

/**
	initialise with reading the config data from EEPROM in flags
	@param aui16_eepromAdr adress in EEPROM where config data begins
	@return true-> parameters valid
*/
bool SpecificRecordConfig_c::init(uint16_t aui16_eepromAdr, DefaultRecordConfig_c* apc_defaultConfig, IsoAgLib::iDINItem_c* apc_memberItem)
{ // set data
	ui16_eepromAdr = aui16_eepromAdr;
	IsoAgLib::iSystem_c::triggerWd();

	// exit if data not valid
	if ( (ui16_eepromAdr == 0) || (apc_defaultConfig == NULL) || (apc_memberItem == NULL))
	{ // wrong data
		return false;
	}

	// first read the default config data
	ui8_timeWert = apc_defaultConfig->timeWert();
	ui8_workWertInst = apc_defaultConfig->workWertInst();
	ui8_applrateRecording = apc_defaultConfig->applrateRecording();
	b_transportDummyWidth = apc_defaultConfig->transportDummyWidth();
	b_transportWorkDist = apc_defaultConfig->transportWorkDist();

	// decide if measure programs should be used
	// a) if default config is 0 ==> no measure prog
	// b) if default config is 1 ==> measure prog for IMI == DEVCLASS different from 4:seed, 5:fertilizer, 6:spreader
	//                                or DEVCLASSINST 5:"Aufsattelposition" or 7:"Anhaengeposition2"
	// c) if default config is 2 ==> measure prog
	ui8_devClass = apc_memberItem->devClass();
	ui8_timeDistDevClass = ui8_devClass;
	ui8_devClassInst = apc_memberItem->devClassInst();

	// clear config setting for application record of x/min for devClass4:seeder
	// -> clear bit of value 2
	if (ui8_devClass == 4) ui8_applrateRecording &= ~2;

	// activate record of work dist if DEVCLASS != 11 (if device is not transport)
	if (ui8_devClass != 11) b_transportWorkDist = 1;

	bool b_isImiDevice;
	if ((ui8_devClass < 4) || (ui8_devClass > 10)
	 || (ui8_devClassInst == 5) || (ui8_devClassInst == 7)
	 || (memcmp(apc_memberItem->name(), "IMI", 3) == 0)
	   )
	{
		b_isImiDevice = true;
		// clear as default every application rate record
		ui8_applrateRecording = 0;
	}
	else b_isImiDevice = false;

	switch (apc_defaultConfig->useMeasureProgs())
	{
		case 0:
			ui8_useMeasureProgs = 0;
			break;
		case 1:
		case 2:
			// decide device type dependent
			// force measure_prog for IMI
			if (b_isImiDevice) ui8_useMeasureProgs = 3;
			// use default setting from EEPROM
			// if not overwritten by specific settings this decides
			// between pure passive mode or sending of single measurement requests
			else ui8_useMeasureProgs = apc_defaultConfig->useMeasureProgs();
			break;
		case 3:
			// use everytime measure_prog
			ui8_useMeasureProgs = 3;
			break;
	}

	c_recordAsISOName = apc_memberItem->devKey();
	b_fieldstarSend = false;

	// get the amount of device records
	uint8_t b_stored_devices_cnt = apc_defaultConfig->deviceSpecificConfigCnt();

	// reset data
	ui16_procEepromAdr = 0;
	ui16_cachedProcEepromAdr = 0;
	i16_cachedProcInd = -1;
	ui8_procCnt = 0;

	ui8_configDataCnt = 0;
	ui8_devClass = 0xFF;
	ui8_devClassInst = 0xFF;
	ui8_nameLen = 0;
	memset(pui8_name, 0, 11);


	// now read data
	int	i_val;

	bool b_device_found = false;
	IsoAgLib::iEepromIo_c& c_eeprom = IsoAgLib::getIeepromInstance();
	c_eeprom.setg(ui16_eepromAdr);

	for (uint8_t b_compare_ind = 0; ((b_compare_ind < b_stored_devices_cnt) && (b_device_found == false));b_compare_ind++)
	{ // outer search loop for the different devices
		readIdentData();
		// now compare read data set with pointed iMember_Item
		b_device_found = true;

		if ((ui8_devClass != 0xFF) && (ui8_devClass != apc_memberItem->devClass())) b_device_found = false;
		if ((ui8_devClassInst != 0xFF) && (ui8_devClassInst != apc_memberItem->devClassInst())) b_device_found = false;
		const uint8_t *pb_member_name = apc_memberItem->name();
		if (memcmp(pui8_name, pb_member_name, ui8_nameLen) != 0) b_device_found = false;
		if (!b_device_found)
		{ // actual device doesn't match searched device -> jump ahead in EEPROM
		  // first 5 data tuples were read for identifying; ui8_configDataCnt is whole tuple count
			c_eeprom.setg(c_eeprom.tellg() + ((ui8_configDataCnt - 5) * TAG_TUPLE_LEN));
		}
	}
	// device is found, or all entries were compared
	if (b_device_found == true)
	{ // first set ui16_eepromAdr to beginning of config data for this device
		ui16_eepromAdr = c_eeprom.tellg() - (5 * TAG_TUPLE_LEN);
		// actual device matches searched device -> read lasting data from EEPROM

		bool ui8_procCnt_found = false; // stop reading here, if cnt of specific proc data was found
		// the first 5 tuples of this device were read -> whole amount of data tuple is ui8_configDataCnt
		for (uint8_t b_tag_ind = 5; ((b_tag_ind < ui8_configDataCnt) && (ui8_procCnt_found == false)); b_tag_ind++)
		{ // inner search loop to identifying read data from one device
			c_eeprom.readString(pui8_cachedTagName, TAG_NAME_LEN);
			c_eeprom.readString(pui8_cachedTagData, TAG_VALUE_LEN);
			tolowerStr(pui8_cachedTagName, TAG_NAME_LEN);

			for (uint8_t b_test = 0; b_test < 1; b_test++)
			{ // loop to read one tag
				if (strstr((const char*)pui8_cachedTagName, "timewert") != NULL)
				{ // read tag is TIMEWERT of data sets for this device
					sscanf((const char*)pui8_cachedTagData, "%i", &i_val);
					ui8_timeWert = i_val;
					break; // exit while loop
				}
				if (strstr((const char*)pui8_cachedTagName, "work_wi") != NULL)
				{ // read tag is TIMEWERT of data sets for this device
					sscanf((const char*)pui8_cachedTagData, "%i", &i_val);
					ui8_workWertInst = i_val;
					break; // exit while loop
				}
				if (strstr((const char*)pui8_cachedTagName, "appl_rec") != NULL)
				{ // read tag is APPL_REC of data sets for this device
					sscanf((const char*)pui8_cachedTagData, "%i", &i_val);
					ui8_applrateRecording = i_val;
					break; // exit while loop
				}
				if (strstr((const char*)pui8_cachedTagName, "transp_w") != NULL)
				{ // read tag is TRANSP_W of data sets for this device
					tolowerStr(pui8_cachedTagData, TAG_VALUE_LEN);
					if ((strchr((const char*)pui8_cachedTagData, '1') != NULL)
					 || (strstr((const char*)pui8_cachedTagData, "true") != NULL))
						b_transportDummyWidth = true;
					else b_transportDummyWidth = false;
					break; // exit while loop
				}
				if (strstr((const char*)pui8_cachedTagName, "trawdist") != NULL)
				{ // read tag is trawdist of data sets for this device
					tolowerStr(pui8_cachedTagData, TAG_VALUE_LEN);
					if ((strchr((const char*)pui8_cachedTagData, '1') != NULL)
					 || (strstr((const char*)pui8_cachedTagData, "true") != NULL))
						b_transportWorkDist = true;
					else b_transportWorkDist = false;
					break; // exit while loop
				}
				if (strstr((const char*)pui8_cachedTagName, "fiel_snd") != NULL)
				{ // read tag is Owner devClass_pos of data sets for this device
					if ((strchr((const char*)pui8_cachedTagData, '1') != NULL)
					 || (strstr((const char*)pui8_cachedTagData, "true") != NULL))
						b_fieldstarSend = true;
					else b_fieldstarSend = false;
					break; // exit while loop
				}
				if (strstr((const char*)pui8_cachedTagName, "use_mprg") != NULL)
				{ // read tag is IS_IMI_DEVICE of data sets for this device
					tolowerStr(pui8_cachedTagData, TAG_VALUE_LEN);
					i_val = -1;
					sscanf((const char*)pui8_cachedTagData, "%i", &i_val);
					switch (i_val)
					{
						case 0: // no measure prog, no single request
						case 1: // no measure prog, no single request
							ui8_useMeasureProgs = 0;
							break;
						case 2: // no measure prog, but single request
							ui8_useMeasureProgs = 2;
							break;
						case 3:	// use measure prog
							ui8_useMeasureProgs = 3;
							break;
						default:
							// tag data is true or false
							if (strstr((const char*)pui8_cachedTagData, "true") != NULL)
								ui8_useMeasureProgs = 3;
							else ui8_useMeasureProgs = 0;
							break;
					}
					break; // exit while loop
				}
				if (strstr((const char*)pui8_cachedTagName, "rec_asgp") != NULL)
				{ // read tag is record_as_devKey of data sets for this device
					sscanf((const char*)pui8_cachedTagData, "%i", &i_val);
					c_recordAsISOName.setCombinedDin( i_val );
					break; // exit while loop
				}
				if (strstr((const char*)pui8_cachedTagName, "td_devClass") != NULL)
				{ // read tag is time_dist_devClass to define specific DEVCLASS for time, dist, area vals
					sscanf((const char*)pui8_cachedTagData, "%i", &i_val);
					ui8_timeDistDevClass = i_val;
					break; // exit while loop
				}
				if (strstr((const char*)pui8_cachedTagName, "proc_cnt") != NULL)
				{ // read tag is PROC_CNT of data sets for this device
					sscanf((const char*)pui8_cachedTagData, "%i", &i_val);
					ui8_procCnt = i_val;
					// the definitions of specific proc data are following in EEPROM
					ui16_procEepromAdr = c_eeprom.tellg();
					// the actual reading position is equivalent to caching proc ind 0
					i16_cachedProcInd = 0;
					ui16_cachedProcEepromAdr = ui16_procEepromAdr;
					ui8_procCnt_found = true; // stop reading
					break; // exit while loop
				}
			} // end for (uint8_t b_test = 0; b_test < 1; b_test++) to find info of tag
		} // end for to read specific tags of searched device
	}
	return true;
}

/**
	private function to read the ident data for one device from EEPROM
	during search
*/
void SpecificRecordConfig_c::readIdentData()
{
	int16_t i_val;
	IsoAgLib::iEepromIo_c& c_eeprom = IsoAgLib::getIeepromInstance();
	for (uint8_t b_tag_ind = 0; b_tag_ind < 5; b_tag_ind++)
	{ // inner search loop to identifying read data from one device
		c_eeprom.readString(pui8_cachedTagName, TAG_NAME_LEN);
		c_eeprom.readString(pui8_cachedTagData, TAG_VALUE_LEN);
		tolowerStr(pui8_cachedTagName, TAG_NAME_LEN);
		for (uint8_t b_test = 0; b_test < 1; b_test++)
		{ // loop to read one tag
			if (strstr((const char*)pui8_cachedTagName, "count") != NULL)
			{ // read tag is COUNT of data sets for this device
				sscanf((const char*)pui8_cachedTagData, "%i", &i_val);
				ui8_configDataCnt = i_val;
				break; // exit while loop
			}
			if (strstr((const char*)pui8_cachedTagName, "devClass") != NULL)
			{ // read tag is DEVCLASS of data sets for this device
				sscanf((const char*)pui8_cachedTagData, "%i", &i_val);
				ui8_devClass = i_val;
				break; // exit while loop
			}
			if (strstr((const char*)pui8_cachedTagName, "devClassInst") != NULL)
			{ // read tag is device class inst of data sets for this device
				sscanf((const char*)pui8_cachedTagData, "%i", &i_val);
				ui8_devClassInst = i_val;
				break; // exit while loop
			}
			if (strstr((const char*)pui8_cachedTagName, "name_len") != NULL)
			{ // read tag is device class inst of data sets for this device
				sscanf((const char*)pui8_cachedTagData, "%i", &i_val);
				ui8_nameLen = i_val;
				break; // exit while loop
			}
			if (strstr((const char*)pui8_cachedTagName, "name") != NULL)
			{ // read tag is device class inst of data sets for this device
				if (strchr((const char*)pui8_cachedTagData, '{') != NULL)
				{ // name given as {0,1,2,3..}
					char *pb_scan_string;
					int16_t i_scanned_val;
					uint8_t b_tag_data_ind = 0;
					for (pb_scan_string = strchr((const char*)pui8_cachedTagData, '{');
					     pb_scan_string != NULL;
					     pb_scan_string = strchr(pb_scan_string, ',')
					    )
			    {
				    pb_scan_string += 1; // set pointer next to searched char "{" or ","
				    sscanf((const char*)pb_scan_string, "%i", &i_scanned_val);
						pui8_cachedTagData[b_tag_data_ind] = i_scanned_val;
						b_tag_data_ind += 1;
						if (b_tag_data_ind ==  10) break;
			    }
					pui8_cachedTagData[b_tag_data_ind] = '\0';
				}
				else
				{
					if ((strstr((const char*)pui8_cachedTagData, "0x") != NULL)
						|| (strstr((const char*)pui8_cachedTagData, "0X") != NULL))
					{ // hex fieldstar code
						long l_name_nr;
						sscanf((const char*)pui8_cachedTagData, "%li", &l_name_nr);
						memset(pui8_name, 0, 3); // set first 3 bytes to 0
						for (uint8_t b_ind = 0; b_ind < 4; b_ind++)
							pui8_name[b_ind+3]= ((l_name_nr >> ((3 - b_ind)*8)) & 0xFF);
					}
					else
					{
						memmove(pui8_name, pui8_cachedTagData, 10);
						pui8_name[10] = '\0';
					}
				}
				break; // exit while loop
			}
		} // end for (uint8_t b_test = 0; b_test < 1; b_test++) to find info of tag
	} // end for to read identifying tags of actual device
}

/**
	utility function to find data for a proc data info
	-> if success is reported, the searched proc data is in the cached
	tuple
	@param ai_ind index [0..n-1] of the wanted proc data definition
	@return true -> wanted proc data is now cached
*/
bool SpecificRecordConfig_c::findProcInd(int ai_ind)
{
	bool b_result = false;
	IsoAgLib::iEepromIo_c& c_eeprom = IsoAgLib::getIeepromInstance();
	// check if wanted proc def can be found
	if (ai_ind >= ui8_procCnt) return false;

	if (ai_ind == i16_cachedProcInd)
	{ // wanted proc data already cached
		c_eeprom.setg(ui16_cachedProcEepromAdr);
		b_result = true;
	}
	else
	{ // search needed  -> define search end
		uint16_t ui16_eepromSearchEnd = get_eeprom_search_end();
		// detection of first "proc_wi" eeprom tag leads to i16_procInd := 0
		// --> match first proc item with index 0 ( [0..(n-1)] )
		int16_t i16_procInd = -1;

		if (ai_ind > i16_cachedProcInd)
		{ // searched proc is behind cached -> start with search from cached
			c_eeprom.setg(ui16_cachedProcEepromAdr);
			i16_procInd = i16_cachedProcInd - 1;
		}
		else
		{
			c_eeprom.setg(ui16_procEepromAdr);
		}

		// search till "proc_wi" (the first tupel of each proc data def) was found ai_ind times
		// first occurence of "proc_wi" means ai_ind == 0
		while ((c_eeprom.tellg() < ui16_eepromSearchEnd) && (i16_procInd < ai_ind))
		{ // not found, and limit of this device config in EEPROM not reached
			c_eeprom.readString(pui8_cachedTagName, TAG_NAME_LEN);
			c_eeprom.readString(pui8_cachedTagData, TAG_VALUE_LEN);
			tolowerStr(pui8_cachedTagName, TAG_NAME_LEN);
			if (strstr((const char*)pui8_cachedTagName, "proc_wi") != NULL)
			{ // definition of new proc_data begins in EEPROM
				i16_procInd += 1;
			}
		}

		if (i16_procInd == ai_ind)
		{
			b_result = true;
			// first item has index 0; [0..(n-1)]; --> as i16_procInd := -1 at search start, search end condition is
			// i16_procInd == ai_ind
			i16_cachedProcInd = ai_ind;
			// the "proc_wi" tuple of wanted proc data was read -> actual read pos is
			// one tuple too far
			ui16_cachedProcEepromAdr = c_eeprom.tellg() - TAG_TUPLE_LEN;
			c_eeprom.setg( ui16_cachedProcEepromAdr );
		}
	}
	return b_result;
}

/**
	utility function to get a tag value for a proc data info
*/
bool SpecificRecordConfig_c::getProcIndString(uint8_t aui8_ind, const char* apui8_tagName)
{
	bool b_result = false;
	IsoAgLib::iEepromIo_c& c_eeprom = IsoAgLib::getIeepromInstance();
	if ((aui8_ind == i16_cachedProcInd) && (strstr((const char*)apui8_tagName, (const char*)pui8_cachedTagName) != NULL))
	{ // wanted tag already cached
		return true;
	}
	if (findProcInd(aui8_ind))
	{ // find_proc_ind set the read position in EEPROM and w_cached_proc_eeprom_adr
		// to first definition tuple of aui8_ind'th proc def of actual device

		// set the adress where to stop the search -> proc def are last config settings
		// of a device -> stop if all data to this device are evaluated
		uint16_t ui16_eepromSearchEnd = get_eeprom_search_end();
		while (c_eeprom.tellg() < ui16_eepromSearchEnd)
		{
			c_eeprom.readString(pui8_cachedTagName, TAG_NAME_LEN);
			c_eeprom.readString(pui8_cachedTagData, TAG_VALUE_LEN);
			tolowerStr(pui8_cachedTagName, TAG_NAME_LEN);
			if (strstr((const char*)pui8_cachedTagName, (const char*)apui8_tagName) != NULL)
			{ // searched tag found
				b_result = true;
				pui8_cachedTagData[TAG_VALUE_LEN] = '\0';
				break;
			}
			// stop if next proc data definition was found
			// but don't stop if first tuple of actual cached proc data was read
			if ((strstr((const char*)pui8_cachedTagName, "proc_wi") != NULL)
			 && (c_eeprom.tellg() != (ui16_cachedProcEepromAdr + TAG_TUPLE_LEN))
			   ) break;
			// in case of some error - stop search if next device config is reached
			if (strstr((const char*)pui8_cachedTagName, "count") != NULL) break;
		}
	}
	return b_result;
}

/**
	deliver the wert_inst of the aui8_ind'th specific process data of
	this device
	@param aui8_ind number of the wanted specific process data
	@return wert_inst or -1 if proc data wasn't found
*/
int16_t SpecificRecordConfig_c::procDataIndWertinst(uint8_t aui8_ind)
{
	if (getProcIndString(aui8_ind, "proc_wi"))
	{
		int16_t i_tag_val;
		sscanf((const char*)pui8_cachedTagData, "%i", &i_tag_val);
		return i_tag_val;
	}
	else
	{
		return -1;
	}
}
/**
	deliver the header of the aui8_ind'th specific process data of
	this device
	@param aui8_ind number of the wanted specific process data
	@return pointer to header or NULL if proc data wasn't found
*/
const uint8_t* SpecificRecordConfig_c::procDataIndHeader(uint8_t aui8_ind)
{
	if (getProcIndString(aui8_ind, "proc_nam"))
	{
		return pui8_cachedTagData;
	}
	else
	{
		return NULL;
	}
}
/**
	deliver the LIS of the aui8_ind'th specific process data of
	this device (default is 0)
	@param aui8_ind number of the wanted specific process data
	@return LIS or the default value 0 if proc data wasn't found
*/
uint8_t SpecificRecordConfig_c::procDataIndLis(uint8_t aui8_ind)
{
	if (getProcIndString(aui8_ind, "proc_lis"))
	{
		int16_t i_tag_val;
		sscanf((const char*)pui8_cachedTagData, "%i", &i_tag_val);
		return i_tag_val;
	}
	else
	{
		return 0;
	}
}
/**
	deliver the PRI of the aui8_ind'th specific process data of
	this device (default is 2)
	@param aui8_ind number of the wanted specific process data
	@return PRI or the default value 2 if proc data wasn't found
*/
uint8_t SpecificRecordConfig_c::procDataIndPri(uint8_t aui8_ind)
{
	if (getProcIndString(aui8_ind, "proc_pri"))
	{
		int16_t i_tag_val;
		sscanf((const char*)pui8_cachedTagData, "%i", &i_tag_val);
		return i_tag_val;
	}
	else
	{
		return 2;
	}
}