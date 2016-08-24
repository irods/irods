#ifndef RS_STRUCT_FILE_EXT_AND_REG_HPP
#define RS_STRUCT_FILE_EXT_AND_REG_HPP

#include "rcConnect.h"
#include "rodsGenQuery.h"
#include "rodsType.h"
#include "objStat.h"
#include "bulkDataObjPut.h"
#include "structFileExtAndReg.h"

int rsStructFileExtAndReg( rsComm_t *rsComm, structFileExtAndRegInp_t *structFileExtAndRegInp );
int chkCollForExtAndReg( rsComm_t *rsComm, char *collection, rodsObjStat_t **rodsObjStatOut );
int regUnbunSubfiles( rsComm_t *rsComm, const char *_resc_name, const char* rescHier, char *collection, char *phyBunDir, int flags, genQueryOut_t *attriArray );
int regSubfile( rsComm_t *rsComm, const char *_resc_name, const char* rescHier, char *subObjPath, char *subfilePath, rodsLong_t dataSize, int flags );
int addRenamedPhyFile( char *subObjPath, char *oldFileName, char *newFileName, renamedPhyFiles_t *renamedPhyFiles );
int postProcRenamedPhyFiles( renamedPhyFiles_t *renamedPhyFiles, int regStatus );
int cleanupBulkRegFiles( rsComm_t *rsComm, genQueryOut_t *bulkDataObjRegInp );
int postProcBulkPut( rsComm_t *rsComm, genQueryOut_t *bulkDataObjRegInp, genQueryOut_t *bulkDataObjRegOut );

#endif
