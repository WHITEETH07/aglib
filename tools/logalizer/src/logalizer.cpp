#define NDEBUG

#ifdef WIN32
#  define SYSTEM_PC_VC
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include "../../../library/xgpl_src/IsoAgLib/hal/pc/typedef.h"
#include "../../../library/xgpl_src/IsoAgLib/isoaglib_config.h"

using namespace std;

ifstream ifs_in;

uint64_t can_time;
uint32_t can_id;
uint8_t  can_data[8];
uint8_t can_bytes;
bool can_ext; /// @todo to be used! =)

bool b_tpcts = false;
bool b_tprts = false;
uint32_t ui32_tpembeddedPgn = 0;
uint8_t i_tpsa = 0;
uint8_t i_tpda = 0;
uint16_t i_tplengthData = 0;
uint8_t *i_tpData = NULL;

bool b_etpcts = false;
bool b_etprts = false;
uint32_t ui32_etpembeddedPgn = 0;
uint8_t i_etpsa = 0;
uint8_t i_etpda = 0;
uint32_t i_etplengthData = 0;
uint32_t i_etpPacketOffSet = 0;
uint8_t *i_etpData = NULL;

uint8_t  canGetPrio() { return (can_id >> 26)       ; }
uint8_t  canGetDp()   { return (can_id >> 24) & 0x03; }
uint8_t  canGetSa()   { return (can_id      ) & 0xFF; }
uint8_t  canGetPs()   { return (can_id >>  8) & 0xFF; }
uint8_t  canGetDa()   { return (can_id >>  8) & 0xFF; } // only if we have PDU1-format!
uint8_t  canGetPf()   { return (can_id >> 16) & 0xFF; }
uint32_t canGetPgn()  { return ((canGetDp() << 16) | (canGetPf() << 8) | ((canGetPf() >= 0xF0) ? canGetPs():0)); }
uint64_t canGetTime() { return (can_time); }
uint8_t  canGetData(int i) { return (can_data[i]); }
uint8_t  canGetBytes() {return (can_bytes); }
bool     canIsPdu1()  { return (canGetPf() < 0xF0); }// with DA

uint64_t rte_time = 0;

void interpretePgnsCl2Fs();
void interpretePgnsFs2Cl();

typedef enum {
  logTypeCanServer=0, // default!
  logTypeRte,
  logTypeCANMon,
  logTypeCANoe,
  logTypeA1ASCII,
  logTypeTrc,
  logTypeJohnDeere,
  logTypeRte2
} logType_t;

logType_t logType;

void analyzeCl2FS(uint8_t, uint8_t, uint16_t, uint8_t[]);
void analyzeFS2Cl(uint8_t, uint8_t, uint16_t, uint8_t[]);
void analyzeRest(uint8_t, uint8_t, uint16_t, uint8_t[]);
void decodeErrorCode(uint8_t);
void decodeAttributes(uint8_t);
void decodeDate(uint16_t);
void decodeTime(uint16_t);
void interpretePgnsCl2FS();
void interpretePgnsFS2Cl();
void interpretePgnsVtEcu(bool);


void exit_with_usage(const char* progname)
{
  cout << "Usage: " << progname << " <logFile> [logType(default=0=can_server)]" << endl << endl;
  cout << "logType=0 is can_server: '104916846 0 1 1 3 6 18eafffe   0   ee  0   0   0   0   0   0'"<<endl;
  cout << "logType=1 is rte:        '[0] HW             97.41  X   9f80182 8 67 34 b0 1c 54 01 e6 06' (with OR without Channel-Nr. in []. Autodetected.)"<<endl;
  cout << "logType=2 is CANMon:     'RX        4     1   CFE5182x| 98  2B  97  6F  FD  00  FF  EB'"<<endl;
  cout << "logType=3 is CANoe:      '  18.9530 1  0CFE4980x        Rx   d 8 00 00 FF FF FF FF FF FF'"<<endl;
  cout << "logType=4 is A1ASCII:    'm e 0x0cf00203 8  0xff 0x00 0x00 0xfa 0xff 0xf0 0x18 0xff       446270'"<<endl;
  cout << "logType=5 is PCANView:   '    13)       116.6  Rx     18EF808B  8  12 15 15 15 15 15 15 15'"<<endl;
  cout << "logType=6 is JohnDeere:  'r Xtd 2 1CAAF883 8 20 03 03 02 00 5C 5C FF 0   0 0060846488      17920  ....... '"<<endl;

  exit(0);
}

void exit_with_error(const char* error_message)
{
  cerr << error_message << endl;
  exit(-1);
}



bool parseLogLineCANMon() //RX        4     1   CFE5182x| 98  2B  97  6F  FD  00  FF  EB
{
  string line;
  getline (ifs_in, line);
#ifdef DEBUG
  cout << line << endl;
#endif

  int i1, i2, i3, i4, i5, i6, i7, i8, iA, iB; // "%i* %x
  int parsed_count = sscanf (line.c_str(), "%*s %i %*i %x%*s%x %x %x %x %x %x %x %x", &iA, &iB, &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8);

  can_time = iA;
  can_id = iB;
  can_bytes = parsed_count-2;
  can_data[0] = i1;
  can_data[1] = i2;
  can_data[2] = i3;
  can_data[3] = i4;
  can_data[4] = i5;
  can_data[5] = i6;
  can_data[6] = i7;
  can_data[7] = i8;

  return (parsed_count >= 2);
}

bool parseLogLineJohnDeere() // "r Xtd 2 1CAAF883 8 20 03 03 02 00 5C 5C FF 0   0 0060846488      17920  ....\\. "
{
  string line;
  getline (ifs_in, line);
#ifdef DEBUG
  cout << line << endl;
#endif
  int i1, i2, i3, i4, i5, i6, i7, i8, iA, iB; // "%i* %x
  int parsed_count = sscanf (line.c_str(), "r Xtd %*i %x %*i %x %x %x %x %x %x %x %x %*i   %*i %i      17920  %s ", &iB, &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8, &iA);
  can_time = iA;
  can_id = iB;
  can_bytes = parsed_count-2;
  can_data[0] = i1;
  can_data[1] = i2;
  can_data[2] = i3;
  can_data[3] = i4;
  can_data[4] = i5;
  can_data[5] = i6;
  can_data[6] = i7;
  can_data[7] = i8;

  return (parsed_count >= 2);
}

bool parseLogLineCanServer() // "104916846 0 1 1 3 6 18eafffe   0   ee  0   0   0   0   0   0"
{
  string line;
  getline (ifs_in, line);
#ifdef DEBUG
  cout << line << endl;
#endif
  int i1, i2, i3, i4, i5, i6, i7, i8, iB; // "%i* %x
  long long iA;
  int parsed_count = sscanf (line.c_str(), "%Li %*i %*i %*i %*i %*i %x %x %x %x %x %x %x %x %x", &iA, &iB, &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8);

  can_time = iA;
  can_id = iB;
  can_bytes = parsed_count-2;
  can_data[0] = i1;
  can_data[1] = i2;
  can_data[2] = i3;
  can_data[3] = i4;
  can_data[4] = i5;
  can_data[5] = i6;
  can_data[6] = i7;
  can_data[7] = i8;

  return (parsed_count >= 2);
}


bool parseLogLineCANoe() // "  18.9530 1  0CFE4980x        Rx   d 8 00 00 FF FF FF FF FF FF "
{
  string line;
  getline (ifs_in, line);
#ifdef DEBUG
  cout << line << endl;
#endif
  int i1, i2, i3, i4, i5, i6, i7, i8, iB, iDb; // "%i* %x
  float f1;
  int parsed_count = sscanf (line.c_str(), " %f %*i %xx %*s %*s %u %x %x %x %x %x %x %x %x", &f1, &iB, &iDb, &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8);

  can_time = uint64_t (f1*1000);
  can_id = iB;
  can_bytes = iDb;
  can_data[0] = i1;
  can_data[1] = i2;
  can_data[2] = i3;
  can_data[3] = i4;
  can_data[4] = i5;
  can_data[5] = i6;
  can_data[6] = i7;
  can_data[7] = i8;

  return (parsed_count == (3+iDb));
}


bool parseLogLineA1ASCII() // "m e 0x0cf00203 8  0xff 0x00 0x00 0xfa 0xff 0xf0 0x18 0xff       446270"
{
  string line;
  getline (ifs_in, line);
#ifdef DEBUG
  cout << line << endl;
#endif
// @todo handle std/ext, too...
  int i1, i2, i3, i4, i5, i6, i7, i8, iA, iB, iDb; // "%i* %x
  int parsed_count = sscanf (line.c_str(), "m e 0x%x %u 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x %i", &iA, &iDb, &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8, &iB);

  can_time = iB;
  can_id = iA;
  can_bytes = iDb;
  can_data[0] = i1;
  can_data[1] = i2;
  can_data[2] = i3;
  can_data[3] = i4;
  can_data[4] = i5;
  can_data[5] = i6;
  can_data[6] = i7;
  can_data[7] = i8;

  return (parsed_count == (3+iDb));
}



bool parseLogLineRte() // "[0] HW             97.41  X   9f80182 8 67 34 b0 1c 54 01 e6 06"
{
  string line;
  getline (ifs_in, line);
#ifdef DEBUG
  cout << line << endl;
#endif

  static int smode=0;
  if (smode == 0)
  { // perform logfile test
    uint8_t x [1024];
    smode = sscanf (line.c_str(), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", x,x,x,x,x,x,x,x,x,x,x,x,x,x,x);
    if ((smode < 14) || (smode > 15)) exit_with_error("wrong rte-log format. use -a for absolute timestamps!");
  }

  int i1, i2, i3, i4, i5, i6, i7, i8, iB, iDb; // "%i* %x
  uint64_t iA;
  int parsed_count = (smode == 15)
                    ? sscanf (line.c_str(), "%*s %*s %Li.%*s %*i.%*s X %x %u %x %x %x %x %x %x %x %x", &iA, &iB, &iDb, &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8)
                    : sscanf (line.c_str(),     "%*s %Li.%*s %*i.%*s X %x %u %x %x %x %x %x %x %x %x", &iA, &iB, &iDb, &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8);

  can_time = iA;
  can_id = iB;
  can_bytes = iDb;
  can_data[0] = i1;
  can_data[1] = i2;
  can_data[2] = i3;
  can_data[3] = i4;
  can_data[4] = i5;
  can_data[5] = i6;
  can_data[6] = i7;
  can_data[7] = i8;

  return (parsed_count == (3+iDb));
}


int parseLogLineRte2 ()
{
    int big;
    int little;
    char *cursor;
    int byte;
    string line;

    getline (ifs_in, line);
#ifdef DEBUG
    cout << line << endl;
#endif

    /* skip over any initial HW */
    for (cursor = const_cast<char *>(line.c_str()); *cursor != 0 && !isspace (*cursor); cursor++)
        /* empty */;

    /*
     * must specify base 10 because of leading 0's
     */
    big = strtol (cursor, &cursor, 10);
    if (*cursor != '.') {
        fprintf (stderr, "unexpected terminator %c (%d): %s\n", *cursor, *cursor, line.c_str());
        return -4;
    }
    little = strtol (&cursor[1], &cursor, 10);
    while (isspace (*cursor))
        cursor++;
    if (*cursor == 'X')
        cursor++;
    else {
        fprintf (stderr, "unexpected field %c\n", *cursor);
        return -5;
    }

    rte_time += big;
    can_time = rte_time;
    can_id = strtol (cursor, &cursor, 16);
    can_bytes = strtol (cursor, &cursor, 0);


    if (can_bytes > 8) {
        fprintf (stderr, "protocol violation at time %lld: dlc of %d too long! (forcing to 8)\n", can_time, can_bytes);
        can_bytes = 8;
    }

    for (byte = 0; byte < 8; byte++) {
        if (byte > can_bytes)     /* TODO: shouldn't this be >= ? */
            break;

        can_data[byte] = strtol (cursor, &cursor, 16);
    }

    return true;
}




bool parseLogLineTrc() // "    13)       116.6  Rx     18EF808B  8  12 15 15 15 15 15 15 15"
{
  string line;
  getline (ifs_in, line);
#ifdef DEBUG
  cout << line << endl;
#endif
  int i1, i2, i3, i4, i5, i6, i7, i8, iA, iB, iDb; // "%i* %x
  int parsed_count = sscanf (line.c_str(), " %*s %u.%*u %*s %x %u %x %x %x %x %x %x %x %x", &iA, &iB, &iDb, &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8);

  can_time = iA;
  can_id = iB;
  can_bytes = iDb;
  can_data[0] = i1;
  can_data[1] = i2;
  can_data[2] = i3;
  can_data[3] = i4;
  can_data[4] = i5;
  can_data[5] = i6;
  can_data[6] = i7;
  can_data[7] = i8;

  return (parsed_count == (3+iDb));
}


void interpretePgnsVtEcu(bool rb_toEcu)
{
  if (canGetBytes() != 8)
  {
    cout << "*** ILLEGAL - THIS PGN *MUST* HAVE 8 DATABYTES ***";
  }
  else
  {
    if (canGetData(0) < 0x10) cout << "Activation ";
    if ((canGetData(0) >=0x10) && (canGetData(0) < 0xF0)) cout << "Command ";

    int8_t i8_uidBytePos = -1;

    switch (canGetData(0))
    {
      case 0x00: cout << "Soft Key"; i8_uidBytePos = 3; break;
      case 0x01: cout << "Button"; i8_uidBytePos = 3; break;
      case 0x02: cout << "Pointing Event"; break;
      case 0x03: cout << "VT Select Input Object"; i8_uidBytePos = 2; break;
      case 0x04: cout << "VT ESC"; i8_uidBytePos = 2; break;
      case 0x05: cout << "VT Change Numeric Value"; i8_uidBytePos = 2; break;
      case 0x06: cout << "VT Change Active Mask"; i8_uidBytePos = 2; break;
      case 0x07: cout << "VT Change Soft Key Mask"; i8_uidBytePos = 4; break;
      case 0x08: cout << "Input String Value"; i8_uidBytePos = 2; break;

      case 0x11: cout << "Object Pool Transfer"; break;
      case 0x12: cout << "End of Object Pool"; break;

      case 0x92: cout << "ESC"; i8_uidBytePos = 2; break;

      case 0xC0: cout << "Get Memory"; break;
      case 0xC2: cout << "Get Number Of Soft Keys"; break;
      case 0xC3: cout << "Get Text Font Data"; break;
      case 0xC7: cout << "Get Hardware"; break;

      case 0xD0: cout << "Store Version"; break;
      case 0xD1: cout << "Load Version"; break;
      case 0xD2: cout << "Delete Version"; break;

      case 0xA0: cout << "Hide/Show Object"; i8_uidBytePos = 2; if (canGetData(4-1)) cout << " -show-"; else cout << " -hide-"; break;
      case 0xA1: cout << "Enable/Disable Object"; i8_uidBytePos = 2; if (canGetData(4-1)) cout << " -enable-"; else cout << " -disable-"; break;
      case 0xA2: cout << "Select Input Object"; i8_uidBytePos = 2; break;
      case 0xA3: cout << "Control Audio Device"; break;
      case 0xA4: cout << "Set Audio Volume"; break;
      case 0xA5: cout << "Change Child Location"; i8_uidBytePos = 4; break;
      case 0xA6: cout << "Change Size"; i8_uidBytePos = 2; break;
      case 0xA7: cout << "Change Background Colour"; i8_uidBytePos = 2; break;
      case 0xA8: cout << "Change Numeric Value"; i8_uidBytePos = 2; break;
      case 0xA9: cout << "Change End Point"; i8_uidBytePos = 2; break;
      case 0xAA: cout << "Change Font Attributes"; i8_uidBytePos = 2; break;
      case 0xAB: cout << "Change Line Attributes"; i8_uidBytePos = 2; break;
      case 0xAC: cout << "Change Fill Attributes"; i8_uidBytePos = 2; break;
      case 0xAD: cout << "Change Active Mask"; i8_uidBytePos = 4; break;
      case 0xAE: cout << "Change Soft Key Mask"; i8_uidBytePos = 5; break;
      case 0xAF: cout << "Change Attribute"; i8_uidBytePos = 2; cout << " AID="<<uint16_t(canGetData(4-1)); break;

      case 0xB0: cout << "Change Priority"; i8_uidBytePos = 2; break;
      case 0xB1: cout << "Change List Item"; i8_uidBytePos = 2; break;
      case 0xB2: cout << "Delete Object Pool"; break;
      case 0xB3: cout << "Change String Value"; i8_uidBytePos = 2; break;
      case 0xB4: cout << "Change Child Position"; i8_uidBytePos = 4; break;

      case 0xFE: cout << "VT Status Message: Active WS-Master's SA: "<<uint16_t(canGetData(1)); break;
      case 0xFF: cout << "Working Set Maintenance"; break;
    }
    if (i8_uidBytePos > 0) cout << " ObjectID="<<dec<<( (uint16_t(canGetData(i8_uidBytePos+1-1)<<8)) | (uint16_t(canGetData(i8_uidBytePos-1))) );
    if ((!rb_toEcu) && (canGetData(0) < 0x10)) cout << " Response";
    if ((rb_toEcu)  && (canGetData(0) >=0x10) && (canGetData(0) < 0xF0)) cout << " Response";
  }
}



void interpretePgnsCl2Fs()
{
  analyzeCl2FS(canGetSa(), canGetDa(),  canGetBytes(), can_data);
}

void interpretePgnsFs2Cl()
{
  analyzeFS2Cl(canGetSa(), canGetDa(), canGetBytes(), can_data);
}


void analyzeCl2FS(uint8_t SA, uint8_t DA, uint16_t lengthData, uint8_t data[])
{
  uint16_t i_pathNameLength = 0, i_destPathNameLength = 0;

  cout << "SA: " << setw(2)<<setfill('0') << uint32_t(SA) << " DA: " << setw(2)<<setfill('0') << uint32_t(DA) << " length Data: " << setw(4)<<setfill('0') << uint32_t(lengthData);
  switch (uint32_t(data[0]))
  {
    case 0x00: cout << "\tClient Connection Maintenance";
      break;

    case 0x01: cout << "\tGet File Server Properties Request ";
      break;

    case 0x10: cout << "\tGet Current Directory Request TAN: " << setw(2) << hex << uint32_t(data[1]);
      break;

    case 0x11: cout << "\tChange Current Directory Request TAN: " << setw(2) << hex << uint32_t(data[1]);
               i_pathNameLength = uint16_t((data[3]<<8) | (data[2]));
               cout << " Path Name Length : " << i_pathNameLength;
               cout << " Path Name : ";
              for (uint16_t i = 0; i < i_pathNameLength; i++)
              {
                 cout << uint8_t(data[4 + i]);
              }
      break;

    case 0x20: cout << "\tOpen File Request TAN: " << setw(2) << hex << uint32_t(data[1]);

              if (data[2] == 0) cout << " Open file for reading only";
              if (data[2] == 1) cout << " Open file for writing only";
              if (data[2] == 2) cout << " Open file for reading and writing only";
              if (data[2] == 3) cout << " Open directory";

              (data[2] & 4)? cout << " Create a new File if not existing": cout << " Open an existing File(fails if non-existent file)";

              (data[2] & 8)? cout << " Open file for appending data the end of the file": cout << " Open file for random access";

              (data[2] & 16)? cout << " Open file with exclusive access(fails if already open)": cout << " Open file for shared access";

              i_pathNameLength = uint16_t((data[4]<<8) | (data[3]));

               cout << " Path Name Length: " << hex << i_pathNameLength;
               cout << " Path Name: ";
              for (uint16_t i = 0; i <   i_pathNameLength; i++)
              {
                 cout << uint8_t(data[5 + i]);
              }
      break;

    case 0x21: cout << "\tSeek File Request TAN: " << setw(2) << hex << uint32_t(data[1]);
               cout << " Handle: " << hex << uint32_t(data[2]);
               cout << " Position Mode: " << hex << uint32_t(data[3]);
               cout << " Offset: " << hex << ( (uint32_t(data[7])<<24) | (uint32_t(data[6])<<16) | (uint32_t(data[5])<<8) | (uint32_t(data[4])) );
      break;

    case 0x22: cout << "\tRead File Request TAN: " << setw(2) << hex << uint32_t(data[1]);
               cout << " Handle: "     <<hex<<uint32_t(data[2]);
               cout << " Count: "      <<hex<<( (uint16_t(data[4])<<8) | (uint16_t(data[3])) );
               if (data[5] == 0x00) cout << " Do not report hidden files a directory listing";
               if (data[5] == 0x01) cout << " Report hidden files in a directory listing";
               if (data[5] == 0xFF) cout << " Parameter ist not available, FS shall not report hidden in a directory listing";
               if ( (data[5] >= 0x02) && (data[5] <= 0xFE) ) cout << " ERROR wrong Error Code";
      break;

    case 0x23: cout << "\tWrite File Request TAN: " << setw(2) << hex << uint32_t(data[1]);
               cout << " Handle: " << hex << uint32_t(data[2]);
               cout << " Number of Byte written Data: " <<dec<<( (uint16_t(data[4])<<8) | (uint16_t(data[3])) );
               cout << " Data: ";
               for (int i = 0; i < ( (uint16_t(data[4])<<8) | (uint16_t(data[3])) ); i++)
               {
                 cout << "0x" << hex << uint32_t(data[5 + i]) << " ";
               }
      break;

    case 0x24: cout << "\tClose File Request TAN: " << setw(2) << hex << uint32_t(data[1]);
               cout << " Handle: " << hex << uint32_t(data[2]);
      break;

    case 0x30: cout << "\tMove / Copy / Rename File Request TAN: " << setw(2) << hex << uint32_t(data[1]);
               cout << ((data[2] && 0x04)? " recursive" : " not recursive");
               cout << ((data[2] && 0x02)? " force" : " not force");
               cout << ((data[2] && 0x01)? " copy" : " not copy");

               i_pathNameLength = uint16_t((data[4]<<8) | (data[3]));

               cout << " Source Path Name Length: "  << hex << i_pathNameLength;
               cout << " Source Path Name: ";
               for (int i = 0; i < i_pathNameLength; i++) {
                   cout << uint8_t(data[7 + i]);
               }

               i_destPathNameLength = uint16_t((data[6]<<8) | (data[5]));

               cout << " Destination Path Name Length: " << hex << i_destPathNameLength;
               cout << " Destination Path Name: ";
               for (int i = 0; i < i_destPathNameLength; i++) {
                 cout << uint8_t(data[7 + i_pathNameLength + i]);
               }
      break;

    case 0x31: cout << "\tDelete File Request TAN: " << setw(2) << hex << uint32_t(data[1]);

               cout << ((data[2] && 0x04)? " recursive" : " not recursive");
               cout << ((data[2] && 0x02)? " force" : " not force");
               cout << ((data[2] && 0x01)? " copy" : " not copy");

               i_pathNameLength = uint16_t((data[4]<<8) | (data[3]));

               cout << " Path Name Length: " << hex << i_pathNameLength;
               cout << " Path Name: ";
              for (uint16_t i = 0; i <  i_pathNameLength; i++)
              {
                 cout << uint8_t(data[5 + i]);
              }

      break;
    case 0x32: cout << "\tGet Files Attributes Request TAN: " << setw(2) << hex << uint32_t(data[1]);

               i_pathNameLength = uint16_t((data[3]<<8) | (data[2]));

               cout << " Path Name Length: " << hex << i_pathNameLength;
               cout << " Path Name: " ;
               for (int i = 0; i < i_pathNameLength; i++) {
                  cout << uint8_t(data[4 + i]);
               }
      break;
    case 0x33: cout << "\tSet File Attributes Request TAN: " << setw(2) << hex << uint32_t(data[1]);

               if (data[2] == 0x1C) cout <<" Clear read-only attribute";
               if (data[2] == 0x1D) cout <<" Set read-only attribute";
               if (data[2] == 0x13) cout <<" Clear hidden attribute";
               if (data[2] == 0x17) cout <<" Set hidden attribute";

               i_pathNameLength = uint16_t((data[4]<<8) | (data[3]));

               cout << " Path Name Length: " << hex << i_pathNameLength;
               cout << " Path Name: " ;
               for (int i = 0; i < i_pathNameLength; i++) {
                  cout << uint8_t(data[5 + i]);
               }
      break;
    case 0x34: cout << "\tGet File Date & File Time Request TAN: " << setw(2) << hex << uint32_t(data[1]);

               i_pathNameLength = uint16_t((data[3]<<8) | (data[2]));

               cout << " Path Name Length: " << hex << i_pathNameLength;
               cout << " Path Name: " ;
               for (int i = 0; i < i_pathNameLength; i++) {
                  cout << uint8_t(data[4 + i]);
               }
      break;
    case 0x40: cout << "\tInitialize Volume Request TAN: " << setw(2) << hex << uint32_t(data[1]);
               cout << " Space : "        <<hex<<( (uint32_t(data[5])<<24) |  (uint32_t(data[4])<<16) |  (uint32_t(data[3])<<8) | (uint32_t(data[2])) );
               if (data[6] == 0x00) cout <<" create  a new volume using all available space";
               if (data[6] == 0x02) cout <<" create  a new volume using space specified";
               if (data[6] == 0x01) cout <<" Overwrite the existing volume";

               i_pathNameLength = uint16_t((data[8]<<8) | (data[7]));

               cout << " Path Name Length: " << hex << i_pathNameLength;
               cout << " Path Name: " ;
               for (int i = 0; i < i_pathNameLength; i++) {
                  cout << uint8_t(data[9 + i]);
               }
      break;
    default: return;
  }
}





void interpretePgn(uint32_t);

void interpretePgnAcknowledge()
{
}

void interpretePgnsCl2FS()
{
  analyzeCl2FS(canGetSa(), canGetDa(), 8, can_data);
}

void interpretePgnsFS2Cl()
{
  analyzeFS2Cl(canGetSa(), canGetDa(), 8, can_data);
}


void interpretePgnsTPETP()
{
  if (canGetBytes() != 8)
  {
    cout << "*** ILLEGAL - THIS PGN *MUST* HAVE 8 DATABYTES ***";
  }
  else
  {
    if ((canGetPgn() == ETP_CONN_MANAGE_PGN) || (canGetPgn() == TP_CONN_MANAGE_PGN))
    {
      switch (canGetData(0))
      {
        case 0x10: 
          cout << "RTS - Request to Send (TP)            "; 

          b_tprts = true;
          b_tpcts = false;
          i_tpsa = canGetSa();
          i_tpda = canGetDa();
          i_tplengthData = (canGetData(2)<<8) | (canGetData(1));
          i_tpData = new uint8_t[i_tplengthData];
          break;
        case 0x14:
          cout << "RTS - Request to Send (ETP)           ";

          b_etprts = true;
          b_etpcts = false;
          i_etpsa = canGetSa();
          i_etpda = canGetDa();
          i_etplengthData = (canGetData(4)<<24) | (canGetData(3)<<16) | (canGetData(2)<<8) | (canGetData(1));
          i_etpPacketOffSet = 0;
          i_etpData = new uint8_t[i_etplengthData];
          break;
        case 0x11: 
           cout << "CTS - Clear to Send (TP)              ";

          b_tpcts = true;
          break;
        case 0x15: 
          cout << "CTS - Clear to Send (ETP)             "; 
          b_etpcts = true;
          break;

        case 0x16:
          cout << "DPO - Data Packet Offset (ETP)        ";
          i_etpPacketOffSet = (canGetData(4)<<16) | (canGetData(3)<<8) | (canGetData(2));
          break;
        case 0x13: 
           //cout << "EoMACK - End of Message Ack (TP)      ";

          if (ui32_tpembeddedPgn == 0xab00)
            analyzeFS2Cl(i_tpsa, i_tpda, i_tplengthData, i_tpData);
          else if (ui32_tpembeddedPgn == 0xaa00)
            analyzeCl2FS(i_tpsa, i_tpda, i_tplengthData, i_tpData);
          else
            analyzeRest(i_tpsa, i_tpda, i_tplengthData, i_tpData);

          b_tprts = false;
          b_tpcts = false;
          i_tpsa = 0;
          i_tpda = 0;
          i_tplengthData = 0;
          i_tpData = NULL;
          ui32_tpembeddedPgn = 0;
          return;
        case 0x17: 
          cout << "EoMACK - End of Message Ack (ETP)     ";

          if (ui32_etpembeddedPgn == 0xab00)
            analyzeFS2Cl(i_etpsa, i_etpda, i_etplengthData, i_etpData);
          else if (ui32_etpembeddedPgn == 0xaa00)
            analyzeCl2FS(i_etpsa, i_etpda, i_etplengthData, i_etpData);
          else
            analyzeRest(i_etpsa, i_etpda, i_etplengthData, i_etpData);

          b_etprts = false;
          b_etpcts = false;
          i_etpsa = 0;
          i_etpda = 0;
          i_etplengthData = 0;
          i_etpData = NULL;
          i_etpPacketOffSet = 0;
          ui32_etpembeddedPgn = 0;
          return;
        case 0x20: 
           cout << "BAM - Broadcast Announce Msg (TP)     ";
          break;
        case 0xFF: 
           cout << "CONNABORT - Connection Abort (TP/ETP) ";

          b_tprts = false;
          b_tpcts = false;
          i_tpsa = 0;
          i_tpda = 0;
          i_tplengthData = 0;
          i_tpData = NULL;
          ui32_tpembeddedPgn = 0;
          break;
        default: return;
      }
      if ((canGetPgn() == TP_CONN_MANAGE_PGN)) {
        ui32_tpembeddedPgn = (canGetData(7)<<16) | (canGetData(6)<<8) | (canGetData(5));
        cout << " for PGN "<<setw(6)<<setfill('0')<<ui32_tpembeddedPgn<< " (";
        interpretePgn (ui32_tpembeddedPgn);
        cout << ")";
      } else if (canGetPgn() == ETP_CONN_MANAGE_PGN) {
        ui32_etpembeddedPgn = (canGetData(7)<<16) | (canGetData(6)<<8) | (canGetData(5));
        cout << " for PGN "<<setw(6)<<setfill('0')<<ui32_etpembeddedPgn<< " (";
        interpretePgn (ui32_etpembeddedPgn);
        cout << ")";
      }

    }
    else if ((canGetPgn() == ETP_DATA_TRANSFER_PGN) || (canGetPgn() == TP_DATA_TRANSFER_PGN))
    {
      cout << "DATA - Data Packet #"<<setw(2)<<setfill(' ')<<dec<<uint16_t(canGetData(0));
      if ((canGetPgn() == TP_DATA_TRANSFER_PGN)) {
        for (int i = 0; i < 7; i++) {
          if ((7 *  (canGetData(0) - 1)) + i >= i_tplengthData)
            break;
          i_tpData[(7 *  (canGetData(0) - 1)) + i] = canGetData(i + 1);
        }
      } else if (canGetPgn() == ETP_DATA_TRANSFER_PGN) {
        for (int i = 0; i < 7; i++) {
          if ((((i_etpPacketOffSet) * 7) + (7 *  (canGetData(0) - 1))) + i >= i_etplengthData)
            break;
          i_etpData[(((i_etpPacketOffSet) * 7) + (7 *  (canGetData(0) - 1))) + i] = canGetData(i + 1);
        }
      }
    }
  }
}

void interpretePgnData (uint32_t rui32_pgn)
{
  switch (rui32_pgn)
  {
    case VT_TO_ECU_PGN:                  interpretePgnsVtEcu(true); break; // to ECU
    case ECU_TO_VT_PGN:                  interpretePgnsVtEcu(false); break; // from ECU
    case ACKNOWLEDGEMENT_PGN:            interpretePgnAcknowledge(); break;
    case PROCESS_DATA_PGN:               break;
    case CLIENT_TO_FS_PGN:               interpretePgnsCl2Fs(); break;
    case FS_TO_CLIENT_PGN:               interpretePgnsFs2Cl(); break;
    case GUIDANCE_MACHINE_STATUS:        break;
    case GUIDANCE_SYSTEM_CMD:            break;
    case ISOBUS_CERTIFICATION:           break;
    case ETP_DATA_TRANSFER_PGN:          interpretePgnsTPETP(); break;
    case ETP_CONN_MANAGE_PGN:            interpretePgnsTPETP(); break;
    case REQUEST_PGN_MSG_PGN:            break;
    case TP_DATA_TRANSFER_PGN:           interpretePgnsTPETP(); break;
    case TP_CONN_MANAGE_PGN:             interpretePgnsTPETP(); break;
    case ADDRESS_CLAIM_PGN:               break;
    case PROPRIETARY_A_PGN:              break;
    case PROPRIETARY_A2_PGN:             break;
    case WORKING_SET_MEMBER_PGN:         break;
    case WORKING_SET_MASTER_PGN:         break;
    case LANGUAGE_PGN:                   break;
    case LIGHTING_DATA_PGN:              break;
    case LIGHTING_COMMAND_PGN:           break;
    case HITCH_PTO_COMMANDS:             break;
    case REAR_PTO_STATE_PGN:             break;
    case FRONT_PTO_STATE_PGN:            break;
    case REAR_HITCH_STATE_PGN:           break;
    case FRONT_HITCH_STATE_PGN:          break;
    case MAINTAIN_POWER_REQUEST_PGN:     break;
    case WHEEL_BASED_SPEED_DIST_PGN:     break;
    case GROUND_BASED_SPEED_DIST_PGN:    break;
    case SELECTED_SPEED_CMD:             break;
    case SELECTED_SPEED_MESSAGE:         break;
    case SOFTWARE_IDENTIFICATION_PGN:    break;
    case TIME_DATE_PGN:                  break;
    case PROPRIETARY_B_PGN:              break;
    case NMEA_GPS_POSITION_RAPID_UPDATE_PGN: break;
    case NMEA_GPS_COG_SOG_RAPID_UPDATE_PGN:  break;
    case NMEA_GPS_POSITION_DATA_PGN:     break;
    case NMEA_GPS_DIRECTION_DATA_PGN:    break;
    case 129542: /* 0x1Fa06 */           break;
    default:                             break;
        /// @todo to be done...
#define AUX_VALVE_0_ESTIMATED_FLOW  0x00FE10LU
#define AUX_VALVE_1_ESTIMATED_FLOW  0x00FE11LU
        //...
#define AUX_VALVE_0_MEASURED_FLOW   0x00FE20LU
#define AUX_VALVE_1_MEASURED_FLOW   0x00FE21LU
        //...
#define AUX_VALVE_0_COMMAND         0x00FE30LU
#define AUX_VALVE_1_COMMAND         0x00FE31LU
        //...
  }
}

void interpretePgn (uint32_t rui32_pgn)
{
  switch (rui32_pgn)
  {
    case VT_TO_ECU_PGN:                  cout << "VT_TO_ECU         "; break;
    case ECU_TO_VT_PGN:                  cout << "ECU_TO_VT         "; break;
    case ACKNOWLEDGEMENT_PGN:            cout << "ACKNOWLEDGEMENT   "; break;
    case PROCESS_DATA_PGN:               cout << "PROCESS_DATA      "; break;
    case CLIENT_TO_FS_PGN:               cout << "CLIENT_TO_FS      "; break;
    case FS_TO_CLIENT_PGN:               cout << "FS_TO_CLIENT      "; break;
    case GUIDANCE_MACHINE_STATUS:        cout << "GUIDANCE_MACH_ST  "; break;
    case GUIDANCE_SYSTEM_CMD:            cout << "GUIDANCE_SYS_CMD  "; break;
    case ISOBUS_CERTIFICATION:           cout << "ISOBUS_CERTIFIC.  "; break;
    case ETP_DATA_TRANSFER_PGN:          cout << "ETP_DATA_TRANSFER "; break;
    case ETP_CONN_MANAGE_PGN:            cout << "ETP_CONN_MANAGE   "; break;
    case REQUEST_PGN_MSG_PGN:            cout << "REQUEST_MSG       "; break;
    case TP_DATA_TRANSFER_PGN:           cout << "TP_DATA_TRANSFER  "; break;
    case TP_CONN_MANAGE_PGN:             cout << "TP_CONN_MANAGE    "; break;
    case ADDRESS_CLAIM_PGN:               cout << "ADDRESS_CLAIM     "; break;
    case PROPRIETARY_A_PGN:              cout << "PROPRIETARY_A     "; break;
    case PROPRIETARY_A2_PGN:             cout << "PROPRIETARY_A2    "; break;
    case WORKING_SET_MEMBER_PGN:         cout << "WORKING_SET_MEMBER"; break;
    case WORKING_SET_MASTER_PGN:         cout << "WORKING_SET_MASTER"; break;
    case LANGUAGE_PGN:                   cout << "LANGUAGE          "; break;
    case LIGHTING_DATA_PGN:              cout << "LIGHTING_DATA     "; break;
    case LIGHTING_COMMAND_PGN:           cout << "LIGHTING_COMMAND  "; break;
    case HITCH_PTO_COMMANDS:             cout << "HITCH_PTO_COMMANDS"; break;
    case REAR_PTO_STATE_PGN:             cout << "REAR_PTO_STATE    "; break;
    case FRONT_PTO_STATE_PGN:            cout << "FRONT_PTO_STATE   "; break;
    case REAR_HITCH_STATE_PGN:           cout << "REAR_HITCH_STATE  "; break;
    case FRONT_HITCH_STATE_PGN:          cout << "FRONT_HITCH_STATE "; break;
    case MAINTAIN_POWER_REQUEST_PGN:     cout << "MAINTAIN_POWER_REQ"; break;
    case WHEEL_BASED_SPEED_DIST_PGN:     cout << "WHEEL_BASED_SPEED_DIST"; break;
    case GROUND_BASED_SPEED_DIST_PGN:    cout << "GROUND_BASED_SPEED_DIST"; break;
    case SELECTED_SPEED_CMD:             cout << "SELECTED_SPEED_CMD"; break;
    case SELECTED_SPEED_MESSAGE:         cout << "SELECTED_SPEED_MESSAGE"; break;
    case SOFTWARE_IDENTIFICATION_PGN:    cout << "SOFTWARE_IDENTIFICATION"; break;
    case TIME_DATE_PGN:                  cout << "TIME_DATE         "; break;
    case PROPRIETARY_B_PGN:              cout << "PROPRIETARY_B(1of)"; break;
    case NMEA_GPS_POSITION_RAPID_UPDATE_PGN: cout << "NMEA_GPS_POSITION_RAPID_UPDATE"; break;
    case NMEA_GPS_COG_SOG_RAPID_UPDATE_PGN:  cout << "NMEA_GPS_COG_SOG_RAPID_UPDATE"; break;
    case NMEA_GPS_POSITION_DATA_PGN:     cout << "NMEA_GPS_POSITION_DATA"; break;
    case NMEA_GPS_DIRECTION_DATA_PGN:    cout << "NMEA_GPS_DIRECTION_DATA"; break;
    case 129542: /* 0x1Fa06 */           cout << "GNSS Pseudorange Noise Statistics"; break;
    default:                             cout << "                  "; break;
        /// @todo to be done...
#define AUX_VALVE_0_ESTIMATED_FLOW  0x00FE10LU
#define AUX_VALVE_1_ESTIMATED_FLOW  0x00FE11LU
        //...
#define AUX_VALVE_0_MEASURED_FLOW   0x00FE20LU
#define AUX_VALVE_1_MEASURED_FLOW   0x00FE21LU
        //...
#define AUX_VALVE_0_COMMAND         0x00FE30LU
#define AUX_VALVE_1_COMMAND         0x00FE31LU
        //...
  }
}




bool parseLogLine()
{
  bool b_result;
  switch (logType)
  {
    case logTypeCanServer: b_result = parseLogLineCanServer();   break;
    case logTypeRte:       b_result = parseLogLineRte();         break;
    case logTypeCANMon:    b_result = parseLogLineCANMon();      break;
    case logTypeCANoe:     b_result = parseLogLineCANoe();       break;
    case logTypeA1ASCII:   b_result = parseLogLineA1ASCII();     break;
    case logTypeTrc:       b_result = parseLogLineTrc();         break;
    case logTypeJohnDeere: b_result = parseLogLineJohnDeere();   break;
    case logTypeRte2:      b_result = parseLogLineRte2();        break;
    default:               exit_with_error("Unknown Log-Type!"); return false; // return just to satisfy compiler. exit_with_error will exit anyway ;)
  }
  if (b_result)
  {
    cout << setfill (' ') << dec << setw (10) << (can_time/1000) << "." << setfill('0')<<setw(3)<<(can_time%1000)<< "   ";
    cout << setfill ('0') << hex << setw (8) << can_id << "  " << uint16_t(can_bytes) << " ";
    int i;
    for (i=0; i< canGetBytes(); i++) cout << " " << setw (2) <<uint16_t(canGetData(i));
    for (;    i<8;              i++) cout << "   ";
    cout << "   "<<setw(2) << uint16_t(canGetSa()) << "->";
    if (canIsPdu1()) cout << setw(2) << uint16_t(canGetPs());
    else cout << "FF";
    cout << "  prio:" << uint16_t(canGetPrio()) << "  pgn:" << setw(6) << canGetPgn() << " => ";
    interpretePgn (canGetPgn());
    interpretePgnData (canGetPgn());
    cout << endl;
  }
  else
  {
    cout << "---- line missing - error in parsing!----"<<endl; /// @todo replace by the original line!
  }
  return b_result;
}


typedef enum
{
  msgTypePeriodic,
  msgTypeCommand,
  msgTypeResponse,
  msgTypeRTS,
  msgTypeCTS,
  msgTypeDATA,
  msgTypeDPO,
  msgTypeEOMACK,
  msgTypeCONNABORT
} msgType_t;

typedef struct
{
  vector<uint64_t> svec_alives [256];
  vector<msgType_t> svec_response [256];
  const char* name;
  int32_t alivePeriod; // ==0 => single Event - >0 ==> periodic Event - <0 ==> handshaking Event
} msgType_s;

msgType_s struct_messages [5];



void checkAlives()
{
  if ( ((canGetPgn() == VT_TO_ECU_PGN) && (canGetData (0) == 0xFE))
       ||
       ((canGetPgn() == ECU_TO_VT_PGN) && (canGetData (0) == 0xFF))
     )
  {
    struct_messages[0].svec_alives [canGetSa()].push_back (canGetTime());
  }


  if ( ((canGetPgn() == PROCESS_DATA_PGN) && (canGetData (0) == 0x0E))
       ||
       ((canGetPgn() == PROCESS_DATA_PGN) && (canGetData (0) == 0x0F))
     )
  {
    struct_messages[1].svec_alives [canGetSa()].push_back (canGetTime());
  }

}



void checkSingles()
{
  if (canGetPgn() == ACKNOWLEDGEMENT_PGN)
  {
    struct_messages[2].svec_alives [canGetSa()].push_back (canGetTime());
  }
}



void checkHandshakingsVtCommands()
{
  int mode; // no need to initialize.

  if ((canGetData(0) >= 0x60) && (canGetData(0) < 0xF0))
  { // not a maintenance command!
    msgType_t msgType_direction;
    uint8_t ui8_saClient;
    if (canGetPgn() == VT_TO_ECU_PGN)
    {
      mode = 3;
      msgType_direction = msgTypeResponse;
      ui8_saClient = canGetPs();
    }
    else if (canGetPgn() == ECU_TO_VT_PGN)
    {
      mode = 3;
      msgType_direction = msgTypeCommand;
      ui8_saClient = canGetSa();
    }
    else return;
    struct_messages[mode].svec_response [ui8_saClient].push_back (msgType_direction);
    struct_messages[mode].svec_alives   [ui8_saClient].push_back (canGetTime());
    return;
  }
}


void checkHandshakingTP()
{
  msgType_t mtype;
  uint8_t ui8_saClient=0xFE; // default to avoid compiler warning. The code below should make sure that it is always initialized properly!
  if ((canGetPgn() == ETP_DATA_TRANSFER_PGN) || (canGetPgn() == TP_DATA_TRANSFER_PGN))
  {
    ui8_saClient = canGetSa();
    mtype = msgTypeDATA;
  }
  else if ((canGetPgn() == ETP_CONN_MANAGE_PGN) || (canGetPgn() == TP_CONN_MANAGE_PGN))
  {
    switch (canGetData (0))
    {
      case 0x10:
      case 0x14: mtype = msgTypeRTS; ui8_saClient = canGetSa(); break;
      case 0x11:
      case 0x15: mtype = msgTypeCTS; ui8_saClient = canGetDa(); break;
      case 0x16: mtype = msgTypeDPO; ui8_saClient = canGetSa(); break;
      case 0x13:
      case 0x17: mtype = msgTypeEOMACK; ui8_saClient = canGetDa();break;
      case 0xFF: mtype = msgTypeCONNABORT; ui8_saClient = canGetSa();
                 // this message is special case! This aborts SA->DA and also DA->SA!
                 struct_messages[4].svec_response [canGetDa()].push_back (mtype);
                 struct_messages[4].svec_alives   [canGetDa()].push_back (canGetTime());
                 break; // CONN ABORT BY SENDER!
      /// @todo Damn, can't we detect ConnAbort by Receiver here, too?
    }
  }
  else return;

  struct_messages[4].svec_response [ui8_saClient].push_back (mtype);
  struct_messages[4].svec_alives   [ui8_saClient].push_back (canGetTime());
}


void setup()
{
  struct_messages [0].name = "VT Server<->Client Periodic Alive";
  struct_messages [0].alivePeriod = 1000;

  struct_messages [1].name = "TC Server<->Client Periodic Alive";
  struct_messages [1].alivePeriod = 2000;

  struct_messages [2].name = "NOT Acknowledge (NACK)";
  struct_messages [2].alivePeriod = 0; // means NO alivePeriod!

  struct_messages [3].name = "VT Command-Response times";
  struct_messages [3].alivePeriod = -1500; // means Handshaking!

  struct_messages [4].name = "(E)TP-communication times";
  struct_messages [4].alivePeriod = -500; // means Handshaking! (damn, we have mixed periods here...)
}

int main (int argc, char** argv)
{
  cerr << "ISOBUS-Logalizer (c) in 2007 by Martin Wodok, OSB AG." << endl << endl;

  if (argc < 2)
    exit_with_usage(argv[0]);

  setup();
  ifs_in.open (argv[1]);
  if (!ifs_in.is_open()) exit_with_error("Couldn't open file");

  if (argc >= 3)
    logType = (logType_t) atoi (argv[2]);
  else // default to can_server
    logType = logTypeCanServer;

  while (!ifs_in.eof())
  {
    if (parseLogLine())
    {
      checkAlives();
      checkSingles();
      checkHandshakingsVtCommands();
      checkHandshakingTP();
    }
  }

  ifs_in.close();
  std::cerr << "Alriiiiite... printing out the results..." << endl;

  for (int i=0; i<256; i++)
  {
    for (unsigned int loop_msg=0; loop_msg<(sizeof(struct_messages)/sizeof(msgType_s)); loop_msg++)
    {
      const int32_t ci32_alivePeriod = struct_messages[loop_msg].alivePeriod;
      if (struct_messages[loop_msg].svec_alives[i].size()>0)
      {
        if (ci32_alivePeriod > 0)
        { // we have a periodic event!
          std::cout << endl << "ISOBUS node with SA="<<hex<<i<<dec<<" had the following alive-times for ["<<struct_messages[loop_msg].name<<"] with alive-periods of "<<struct_messages[loop_msg].alivePeriod<<" ms:"<<endl;
        }
        else if (ci32_alivePeriod < 0)
        { // we have a handshaking event!
          std::cout << endl << "ISOBUS node with SA="<<hex<<i<<dec<<" had the following alive-times for ["<<struct_messages[loop_msg].name<<"] with alive-periods of "<<(-struct_messages[loop_msg].alivePeriod)<<" ms:"<<endl;
        }
        else
        { // sinlge events!! "== 0"
          std::cout << endl << "ISOBUS node with SA="<<hex<<i<<dec<<" sent out ["<<struct_messages[loop_msg].name<<"] at the following times:"<<endl;
        }
        vector<msgType_t>::iterator type_iter=struct_messages[loop_msg].svec_response[i].begin();
        for (vector<uint64_t>::iterator iter=struct_messages[loop_msg].svec_alives[i].begin();
             iter != struct_messages[loop_msg].svec_alives[i].end();
             iter++)
        {
          cout << setfill (' ');
          cout << "absolute time: "<<setw(10)<<(*iter/1000)<<"."<<setw(3)<<setfill('0')<<(*iter%1000)<<setfill(' ');
          if (iter != struct_messages[loop_msg].svec_alives[i].begin())
          {
            const uint64_t cui32_delta = ( *(iter) - *(iter-1) );
            cout<< "  relative time: "<<setw(10)<<cui32_delta;

            if (ci32_alivePeriod > 0)
            { // print out the alivePeriod-deviation!
              cout<<" deviation: ";
              int deviation = int ((float (int32_t (cui32_delta)-ci32_alivePeriod) / ci32_alivePeriod) * 100);
              uint8_t deviation_bias = (deviation > 0) ? '+' : '-';
              deviation = (deviation < 0) ? -deviation : deviation;
              if (deviation > 100)
              {
                std::cout << "EXTREME DEVIATION(!!) OF " << setw(10) << deviation << "0";
              }
              else
              {
                while (deviation > 10)
                {
                  cout << deviation_bias;
                  deviation -= 10;
                }
              }
            }
            else if (ci32_alivePeriod < 0)
            { // Handshaking
              if (type_iter == struct_messages[loop_msg].svec_response[i].end()) exit_with_error("No direction/msgType set but is expected. System failure.");
              int32_t i32_alivePeriodSpecial;
              switch (*type_iter)
              {
                case msgTypeResponse: cout << " Response  "; i32_alivePeriodSpecial = -ci32_alivePeriod; break;
                case msgTypeCommand:  cout << " Command   "; i32_alivePeriodSpecial = 0; break; // no timing-requirement here!
                case msgTypeRTS:      cout << " (E)TP-CONN: Request to Send (RTS)         "; i32_alivePeriodSpecial = 0; break; // no timing-requirement here! 
                case msgTypeCTS:      cout << " (E)TP-CONN: Clear to Send (CTS)           "; i32_alivePeriodSpecial = 1250; break;
                case msgTypeDPO:      cout << " (E)TP-CONN: Data Packet Offset (DPO)      "; i32_alivePeriodSpecial = 1250; break; /// @todo set the correct values here!
                case msgTypeEOMACK:   cout << " (E)TP-CONN: End of Message ACK (EoMACK)   "; i32_alivePeriodSpecial = 1250; break;
                case msgTypeDATA:     cout << " (E)TP-DATA                                "; i32_alivePeriodSpecial = 250; break;
                case msgTypeCONNABORT:cout << " (E)TP-CONN: Connection Abort (CONNABORT)  "; i32_alivePeriodSpecial = -1; break; // doesn't come normally!
                default:              cout << " ???                                       "; i32_alivePeriodSpecial = 0; break;
              }
              if ( (*type_iter == msgTypeResponse) && (*(type_iter-1) == msgTypeResponse) ) cout << " - RESPONSE FOLLOWING A RESPONSE!";
              if ( (*type_iter == msgTypeCommand)  && (*(type_iter-1) == msgTypeCommand)  ) cout << " - COMMAND FOLLOWING A COMMAND!";
              if (i32_alivePeriodSpecial > 0)
              { // print out the time it took!
                if (cui32_delta > (unsigned int) (i32_alivePeriodSpecial)) cout << " *** !!! TIMEOUT - Check relative time!!!! ***";
                else
                {
                  int32_t time = (cui32_delta*100) / i32_alivePeriodSpecial;
                  cout <<setw(2)<<time<< " percent of timeout ("<<setw(4)<<i32_alivePeriodSpecial<<"): (one '%' shows 10%) ";
                  while (time > 10)
                  {
                    cout << "%";
                    time -= 10;
                  }
                }
              }
              else if (i32_alivePeriodSpecial < 0)
              { // unsolicited messages (like CONNABORT)
                cout << "*** UNEXPECTED/UNSOLICITED MESSAGE ***";
              }
            }
          }
          cout << endl;
          if (type_iter != struct_messages[loop_msg].svec_response[i].end()) type_iter++;
        }
      }
    }
  }
}

void analyzeFS2Cl(uint8_t SA, uint8_t DA, uint16_t lengthData, uint8_t Data[])
{
  uint32_t i_pathNameLength = 0;
  uint16_t msg_count = 0;
  uint16_t msg_offset = 0;

  cout << "SA: " << setw(2)<<setfill('0') << uint32_t(SA) << " DA: " << setw(2)<<setfill('0') << uint32_t(DA) << " length Data: " << setw(4)<<setfill('0') << uint32_t(lengthData);

  switch (uint32_t(Data[0])) {
    case 0x00: 
      cout << "\tFile Server Status: ";
        (Data[1] & 0x2) ? cout << "Busy writing" : cout << "Not Busy writing";
        (Data[1] & 0x1) ? cout << " Busy reading" : cout << " Not Busy reading";
      cout << " Number of Open Files: " << setw(2) << hex << uint32_t(Data[2]);
      break; //File Server Status

    case 0x01: 

      cout << "\tGet File Server Properties Response Version Number:";
      switch (Data[1]) {
        case 0x00: cout << " Draft version of the Standard"; break;
        case 0x01: cout << " Final Draft version of the Standard"; break;
        case 0x02: cout << " First published version of the Standard"; break;
      }
      cout << " Maximum Number of Simuntaneously Open Files: " << hex << uint32_t(Data[2]);
      cout << " File Server Capabilities : ";
      (Data[3]) ? cout << " File server supports multiple volumes"
                : cout << " File server does not supports multiple volumes";
      break; //Get File Server Properties Response

    case 0x10: 

      cout << "\tGet Current Directory Response TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);
      cout << " Total Space: " << setw(8) << uint32_t((Data[6]<<24) | (Data[5]<<16) | (Data[4]<<8) | Data[3]) << "(* 512 bytes)";
      cout << " Free Space: " << setw(8) << uint32_t((Data[10]<<24) | (Data[9]<<16) | (Data[8]<<8) | Data[7]) << "(* 512 bytes)";
      i_pathNameLength = ((Data[12]<<8) | Data[11]);
      cout << " Path Name Length: " << setw(4) << i_pathNameLength;

      cout << " Path Name: ";
      for (uint16_t i_count = 0; i_count < i_pathNameLength; i_count++)
      {
        cout << uint8_t(Data[13 + i_count]);
      }

      break; //Get Current Directory Response 
    case 0x11: 

      cout << "\tChange Current Directory Response TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);

      break; //Chanege Current Directory Response
    case 0x20:

      cout << "\tOpen File Response TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);
      cout << " Handle: " << setw(2) << hex << uint32_t(Data[3]);
      decodeAttributes(Data[4]);

      break; //Open File Response
    case 0x21: 

      cout << "\tSeek File Response TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);
      cout << " Position : " << setw(8) << uint32_t((Data[7]<<24) | (Data[6]<<16) | (Data[5]<<8) | Data[4]);
      break; //Seek File Response
    case 0x22:

      cout << "\tRead File/Directory Response TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);

      msg_count = uint16_t((Data[4]<<8) | Data[3]);
      msg_offset = 5;

      cout << " Count: " << setw(4) << msg_count << " ";

      //case file-data
      if (lengthData == msg_count + 5) {
        for (uint16_t loop = 0; loop < msg_count; loop++) {
          cout << setw(2) << hex << uint32_t(Data[5 + loop]) << " ";
        }
      }
      //else directory entries
      else {
        for (uint16_t loop = 0; loop < msg_count; loop++) {
          uint8_t filenamelength = uint8_t(Data[msg_offset]);
          if (filenamelength == 0xff)
            break;
          cout << " Filename length: " << uint16_t(filenamelength) << " ";
          cout << " Filename: ";
          uint8_t loopfilename = 0;
          for (; loopfilename < filenamelength; loopfilename++) {
            cout << uint8_t(Data[msg_offset + 1 + loopfilename]);
          }
          decodeAttributes(Data[msg_offset + 1 + loopfilename]);
          decodeDate(uint16_t(Data[msg_offset + 3 + loopfilename] << 8 | uint16_t(Data[msg_offset + 2 + loopfilename])));
          decodeTime(uint16_t(Data[msg_offset + 5 + loopfilename] << 8 | uint16_t(Data[msg_offset + 4 + loopfilename])));
          cout << " FileSize: " << uint32_t(Data[msg_offset + 9 + loopfilename] << 24 |Data[msg_offset + 8 + loopfilename] << 16 |Data[msg_offset + 7 + loopfilename] << 8 | uint16_t(Data[msg_offset + 6 + loopfilename]));
          msg_offset += (filenamelength + 10);
        }
      }

      break; //Read File/Directory Response
    case 0x23: 

      cout << "\tWrite File Response TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);
      cout << " Count: " << setw(4) << uint16_t((Data[4]<<8) | Data[3]);
      break; // Write File Response
    case 0x24: 

      cout << "\tClose File Response TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);
      break; // Close File Response
    case 0x30: 

      cout << "\tMove File Response TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);
      break; // Move File Response 
    case 0x31: 

      cout << "\tDelete File Repsonse TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);
      break; // Delete File Repsonse
    case 0x32: 

      cout << "\tGet File Attribute Response TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);
      decodeAttributes(Data[3]);
      cout << " Size: " << uint32_t(Data[4] | Data[5] << 8 | Data[6] << 16 | Data[7] << 24);
      break; // Get File Attribute Response
    case 0x33: 

      cout << "\tSet File Attribute Response TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);
      break; // Set File Attribute Response
    case 0x34: 

      cout << "\tGet File Date & Time Response TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);
      decodeDate(uint16_t((Data[4]<<8) | Data[3]));
      decodeTime(uint16_t((Data[6]<<8) | Data[5]));
      break; // Get File Date & Time Response
    case 0x40: 

      cout << "\tInitialize Volume Response TAN: " << setw(2) << hex << uint32_t(Data[1]);
      decodeErrorCode(Data[2]);
      decodeAttributes(Data[3]);
      break; // Initialize Volume Response
    default: return;
  }
}

void analyzeRest(uint8_t SA, uint8_t DA, uint16_t lengthData, uint8_t Data[])
{
  cout << endl << endl; 
  cout << "SA: " << setw(2)<<setfill('0') << uint32_t(SA) << " DA: " << setw(2)<<setfill('0') << uint32_t(DA) << " length Data: " << setw(4)<<setfill('0') << uint32_t(lengthData) << " " << endl;
  cout << "For PGN "<<setw(6)<<setfill('0')<<ui32_tpembeddedPgn<< " (";
      interpretePgn (ui32_tpembeddedPgn);
  cout << ")" << endl << "";
  for (int i = 0; i < lengthData; i++)
  {
    cout << setw(2)<<setfill('0') << uint32_t(Data[i]) << " ";
    if (i != 0 && (i + 1) % 8 == 0)
      cout << endl << "";
  }
}

void decodeErrorCode(uint8_t i_errorCode)
{
  switch (uint32_t(i_errorCode)) {
    case 0x00: 
      cout << " Success" ;
      break;
    case 0x01: 
      cout << " Access Denied" ;
      break;
    case 0x02: 
      cout << " Invalid Access" ;
      break;
    case 0x03: 
      cout << " Too many files open" ;
      break;
    case 0x04: 
      cout << " File or path not found" ;
      break;
    case 0x05: 
      cout << " Invalid Handle" ;
      break;
    case 0x06: 
      cout << " Invalid given source name" ;
      break;
    case 0x07: 
      cout << " Invalid given destination name" ;
      break;
    case 0x08: 
      cout << " Out of free space" ;
      break;
    case 0x09: 
      cout << " Failure during write operation" ;
      break;
    case 0x0a: 
      cout << " Volume is possibly not initialized" ;
      break;
    case 0x0b: 
      cout << " Failure during a read operation" ;
      break;
    case 0x0c: 
      cout << " Function not supported" ;
      break;
    case 0x2A: 
      cout << " Invalid request length" ;
      break;
    case 0x2B: 
      cout << " Out of memory" ;
      break;
    case 0x2C: 
      cout << " Any other error" ;
      break;
    case 0x2D: 
      cout << " End of file reached" ;
      break;
    default:
      cout << " Reserved" ;
      break;
  }
}

void decodeAttributes(uint8_t i_attributes)
{
  (i_attributes & 128)? cout << " Volume is case-sensitive"  : cout << " Volume is case-insensitive" ;
  (i_attributes & 64)? cout << " Volume is not removable"  : cout << " Volume is removable" ;
  (i_attributes & 32)? cout << " Volume does support long filenames"  : cout << " Volume does not support long filenames" ;
  (i_attributes & 16)? cout << " Handle specifies a directory"  : cout << " Handle does not specify a directory" ;
  (i_attributes & 8)? cout << " Handle specifies a volume"  : cout << " Handle does not specify a volume" ;
  (i_attributes & 2)? cout << " Hidden attribute is set"  : cout << " Hidden attribute is not set" ;
  (i_attributes & 1)? cout << " Read-only attribute is set"  : cout << " Read-only attribute is not set" ;
}

void decodeDate(uint16_t i_date)
{

  cout << " " << setw(4) << dec << uint32_t(1980 + ((i_date >> 9) & 0x7F)) << "-" << setw(2) << dec << uint32_t((i_date >> 5) & 0xF) << "-" << setw(2) << dec << uint32_t((i_date) & 0x1F);

}

void decodeTime(uint16_t i_time)
{
  cout << " " << setw(2) << dec << uint32_t(((i_time >> 11) & 0x1F)) << "-" << setw(2) << dec << uint32_t(((i_time >> 5) & 0x3F)) << "-" << setw(2) << dec << uint32_t(2 * ((i_time) & 0x1F));
}