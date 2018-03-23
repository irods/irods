#ifndef RS_STRUCT_FILE_EXT_AND_REG_HPP
#define RS_STRUCT_FILE_EXT_AND_REG_HPP

#include "rcConnect.h"
#include "objStat.h"
#include "structFileExtAndReg.h"

int rsStructFileExtAndReg(
    rsComm_t *rsComm,
    structFileExtAndRegInp_t *structFileExtAndRegInp );

int chkCollForExtAndReg(
    rsComm_t *rsComm,
    char *collection,
    rodsObjStat_t **rodsObjStatOut );

#endif
