#ifndef RS_OBJ_STAT_HPP
#define RS_OBJ_STAT_HPP

#include "objStat.h"

int rsObjStat( rsComm_t *rsComm, dataObjInp_t *dataObjInp, rodsObjStat_t **rodsObjStatOut );
int _rsObjStat( rsComm_t *rsComm, dataObjInp_t *dataObjInp, rodsObjStat_t **rodsObjStatOut );
int _rsObjStat( rsComm_t *rsComm, dataObjInp_t *dataObjInp, rodsObjStat_t **rodsObjStatOut );
int dataObjStat( rsComm_t *rsComm, dataObjInp_t *dataObjInp, rodsObjStat_t **rodsObjStatOut );

#endif
