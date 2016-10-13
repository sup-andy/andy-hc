
#ifndef _ZWAVE_ASSOCIATION_H_
#define _ZWAVE_ASSOCIATION_H_
//#pragma pack(1)
#include "zwave_api.h"
#include "zwave_appl.h"
#include "zwave_serialapi.h"
#include "zwave_security_aes.h"

extern ZW_NODES_ASSOCIATION *zw_nodes_association;

extern ZW_STATUS ZW_AssociationSupportedGroupingsGet(WORD associatedID, BYTE *pSupportedGroupings);
extern ZW_STATUS ZW_AssociationGet(WORD associatedID, BYTE grouping, BYTE *pNodeID, BYTE *pNodeNum);
extern ZW_STATUS ZW_AssociationSet(WORD associatedID, BYTE grouping, BYTE DesNodeID);
extern ZW_STATUS ZW_AssociationRemove(WORD associatedID, BYTE grouping, BYTE DesNodeID);
extern ZW_STATUS ZW_AssociationSpecificGroupGet(WORD associatedID, BYTE *pGroup);
extern ZW_STATUS ZW_AssociationRecordsSupportedGet(WORD associatedID, BYTE *pMaxCommandLength, BYTE *pVAndC, BYTE *pConfigurableSupported, WORD *pFreeRecords, WORD *pMaxRecords);
extern ZW_STATUS ZW_AssociationConfigurationGet(WORD associatedID, BYTE grouping, BYTE nodeID);
extern ZW_STATUS ZW_AssociationConfigurationSet(WORD associatedID, BYTE grouping, BYTE nodeID, BYTE cmdLength, BYTE *pCmd);




#endif

