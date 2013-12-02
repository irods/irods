/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* reVariables.h - header file for Variables that are 'used' when computing the conditional
 * expressions in a rule by the the rule engine module
 */
#ifndef RE_VARIABLES_H
#define RE_VARIABLES_H

#include "rodsUser.h"
#include "rods.h"
#include "rcGlobalExtern.h"
#include "reDefines.h"
#include "objInfo.h"
#include "regExpMatch.h"

/*************
typedef struct {
  char varName[NAME_LEN];
  char action[NAME_LEN];
  char *var2CMap;
} rulevardef_t;


rulevardef_t RuleVarTable[] = {
  {RESC_NAME_KW, "resc_modify", (char *) "rei->roi->rescName"},
  {OBJ_PATH_KW, "", (char *) "rei->doi->objPath"},
  {RESC_NAME_KW, "", (char *) "rei->doi->rescName"},
  {DEST_RESC_NAME_KW,  "", (char *) "rei->doi->destRescName"},
  {BACKUP_RESC_NAME_KW, "", (char *) "rei->doi->backupRescName"},
  {DATA_TYPE_KW, "", (char *) "rei->doi->dataType"},
  {DATA_SIZE_KW, "", (char *) "rei->doi->dataSize"},
  {CHKSUM_KW, "", (char *) "rei->doi->chksum"},
  {VERSION_KW, "", (char *) "rei->doi->version"},
  {FILE_PATH_KW, "", (char *) "rei->doi->filePath"},
  {REPL_NUM_KW, "", (char *) "rei->doi->replNum"},
  {REPL_STATUS_KW, "", (char *) "rei->doi->replStatus"},
  {WRITE_FLAG_KW, "", (char *) "rei->doi->writeFlag"},
  {DATA_OWNER_KW, "", (char *) "rei->doi->dataOwnerName"},
  {DATA_OWNER_ZONE_KW, "", (char *) "rei->doi->dataOwnerZone"},
  {DATA_EXPIRY_KW, "", (char *) "rei->doi-dataExpiry"},
  {DATA_COMMENTS_KW, "", (char *) "rei->doi->dataComments"},
  {DATA_CREATE_KW, "", (char *) "rei->doi-dataCreate"},
  {DATA_MODIFY_KW, "", (char *) "rei->doi-dataModify"},
  {DATA_ACCESS_KW, "", (char *) "rei->doi->dataAccess"},
  {DATA_ACCESS_INX_KW, "", (char *) "rei->doi->dataAccessInx"},
  {DATA_ID_KW, "", (char *) "rei->doi->dataId"},
  {COLL_ID_KW, "", (char *) "rei->doi->collId"},
  {RESC_GROUP_NAME_KW, "", (char *) "rei->doi->rescGroupName"},
  {STATUS_STRING_KW, "", (char *) "rei->doi->statusString"},
  {DATA_MAP_ID_KW, "", (char *) "rei->doi->dataMapId"},

  {USER_NAME_CLIENT_KW, "", (char *) "rei->uoic->userName"},
  {RODS_ZONE_CLIENT_KW, "", (char *) "rei->uoic->rodsZone"},
  {USER_TYPE_CLIENT_KW, "", (char *) "rei->uoic->userType"},
  {HOST_CLIENT_KW, "", (char *) "rei->uoic->authInfo->host"},
  {CLIENT_ADDR_KW, "", (char *) "rei->rsComm->clientAddr"},
  {AUTH_STR_CLIENT_KW, "", (char *) "rei->uoic->authInfo->authStr"},
  {USER_AUTH_SCHEME_CLIENT_KW, "", (char *) "rei->uoic->authInfo->authScheme"},
  {USER_INFO_CLIENT_KW, "", (char *) "rei->uoic->userOtherInfo->userInfo"},
  {USER_COMMENT_CLIENT_KW, "", (char *) "rei->uoic->userOtherInfo->userComments"},
  {USER_CREATE_CLIENT_KW, "", (char *) "rei->uoic-userOtherInfo->userCreate"},
  {USER_MODIFY_CLIENT_KW, "", (char *) "rei->uoic-userOtherInfo->userModify"},

  {USER_NAME_PROXY_KW, "", (char *) "rei->uoip->userName"},
  {RODS_ZONE_PROXY_KW, "", (char *) "rei->uoip->rodsZone"},
  {USER_TYPE_PROXY_KW, "", (char *) "rei->uoip->userType"},
  {HOST_PROXY_KW, "", (char *) "rei->uoip->authInfo->host"},
  {AUTH_STR_PROXY_KW, "", (char *) "rei->uoip->authInfo->authStr"},
  {USER_AUTH_SCHEME_PROXY_KW, "", (char *) "rei->uoip->authInfo->authScheme"},
  {USER_INFO_PROXY_KW, "", (char *) "rei->uoip->userOtherInfo->userInfo"},
  {USER_COMMENT_PROXY_KW, "", (char *) "rei->uoip->userOtherInfo->userComments"},
  {USER_CREATE_PROXY_KW, "", (char *) "rei->uoip->userOtherInfo->userCreate"},
  {USER_MODIFY_PROXY_KW, "", (char *) "rei->uoip->userOtherInfo->userModify"},

  {COLL_NAME_KW, "", (char *) "rei->coi->collName"},
  {COLL_PARENT_NAME_KW, "", (char *) "rei->coi->collParentName"},
  {COLL_OWNER_NAME_KW, "", (char *) "rei->coi->collOwnerName"},
  {COLL_EXPIRY_KW, "", (char *) "rei->coi-collExpiry"},
  {COLL_COMMENTS_KW, "", (char *) "rei->coi->collComments"},
  {COLL_CREATE_KW, "", (char *) "rei->coi-collCreate"},
  {COLL_MODIFY_KW, "", (char *) "rei->coi-collModify"},
  {COLL_ACCESS_KW, "", (char *) "rei->coi->collAccess"},
  {COLL_ACCESS_INX_KW, "", (char *) "rei->coi->collAccessInx"},
  {COLL_MAP_ID_KW, "", (char *) "rei->coi->collMapId"},
  {COLL_INHERITANCE_KW, "", (char *) "rei->coi->collInheritance"},

  {RESC_ZONE_KW, "", (char *) "rei->roi->zoneName"},
  {RESC_LOC_KW, "", (char *) "rei->roi->rescLoc"},
  {RESC_TYPE_KW, "", (char *) "rei->roi->rescType"},
  {RESC_TYPE_INX_KW, "", (char *) "rei->roi->rescTypeInx"},
  {RESC_CLASS_KW, "", (char *) "rei->roi->rescClass"},
  {RESC_CLASS_INX_KW, "", (char *) "rei->roi->rescClassInx"},
  {RESC_VAULT_PATH_KW, "", (char *) "rei->roi->rescVaultPath"},
  {NUM_OPEN_PORTS_KW, "", (char *) "rei->roi->numOpenPorts"},
  {PARA_OPR_KW, "", (char *) "rei->roi->paraOpr"},
  {RESC_ID_KW, "", (char *) "rei->roi->rescId"},
  {GATEWAY_ADDR_KW, "", (char *) "rei->roi->gateWayAddr"},
  {RESC_MAX_OBJ_SIZE_KW, "", (char *) "rei->roi->rescMaxObjSize"},
  {FREE_SPACE_KW, "", (char *) "rei->roi->freeSpace"},
  {FREE_SPACE_TIME_KW, "", (char *) "rei->roi->long freeSpaceTime"},
  {FREE_SPACE_TIMESTAMP_KW, "", (char *) "rei->roi-freeSpaceTimeStamp"},
  {RESC_INFO_KW, "", (char *) "rei->roi->rescInfo"},
  {RESC_COMMENTS_KW, "", (char *) "rei->roi->rescComments"},
  {RESC_CREATE_KW, "", (char *) "rei->roi-rescCreate"},
  {RESC_MODIFY_KW, "", (char *) "rei->roi-rescModify"}
};

int NumOfRuleVar = sizeof(RuleVarTable) / sizeof(rulevardef_t);
********************/
#endif	/* RE_VARIABLES_H */

