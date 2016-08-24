#ifndef RS_BULK_DATA_OBJ_REG_HPP
#define RS_BULK_DATA_OBJ_REG_HPP

int rsBulkDataObjReg( rsComm_t *rsComm, genQueryOut_t *bulkDataObjRegInp, genQueryOut_t **bulkDataObjRegOut );
int _rsBulkDataObjReg( rsComm_t *rsComm, genQueryOut_t *bulkDataObjRegInp, genQueryOut_t **bulkDataObjRegOut );
int modDataObjSizeMeta( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, char *strDataSize );
int svrRegReplByDataObjInfo( rsComm_t *rsComm, dataObjInfo_t *destDataObjInfo );

#endif
