#include "UMTSStick.h"

const UMTSSwitchingInfo UMTSwitchingTable[UMTS_SWITCHING_COUNT] = {

/*
struct UMTSSwitchingInfo
{
  uint16_t cdfsVid;
  uint16_t cdfsPid;
  uint16_t serialVid;
  uint16_t serialPidList[16];
  byte targetClass;
  bool huaweiPacket;
  byte cdfsPacket[31];
};
*/

//Huawei E220, E230, E270, E870
{ 0x12d1, 0x1003, 0, {0}, 0xFF, true, {0} },

//Huawei E1550, E270+
{ 0x12d1, 0x1446, 0x12d1, {0x1001, 0x1406, 0x140c, 0x14ac/*, 0x1003*/}, 0, false, { 0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78, 0,    0, 0, 0, 0,    0, 0, 0x11, 0x06, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }

};
