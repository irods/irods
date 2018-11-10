/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rodsPackTable.h - The common rods pack Instruction table
 */

#ifndef RODS_PACK_TABLE_H__
#define RODS_PACK_TABLE_H__

#include "rods.h"
#include "packStruct.h"
#include "rodsPackInstruct.h"
#include "reDefines.h"
#include "authenticate.h"
#include "rcGlobalExtern.h"

#define UNKNOWN_SIZE	-1

packType_t packTypeTable[] = {
    {"char", PACK_CHAR_TYPE, sizeof( char )},
    {"bin", PACK_BIN_TYPE, sizeof( char )},
    {"str", PACK_STR_TYPE, sizeof( char )},
    {"piStr", PACK_PI_STR_TYPE, sizeof( char )},  /* str containing pi */
    {"int", PACK_INT_TYPE, 4},
    {"double", PACK_DOUBLE_TYPE, 8},
    {"struct", PACK_STRUCT_TYPE, UNKNOWN_SIZE},
    {"?", PACK_DEPENDENT_TYPE, UNKNOWN_SIZE},
    {"%", PACK_INT_DEPENDENT_TYPE, UNKNOWN_SIZE},
    {"int16", PACK_INT16_TYPE, 2},
};

int NumOfPackTypes = ( sizeof( packTypeTable ) / sizeof( packType_t ) );

packConstant_t PackConstantTable[] = {
    {"HEADER_TYPE_LEN", HEADER_TYPE_LEN},
    {"NAME_LEN", NAME_LEN},
    {"CHKSUM_LEN", CHKSUM_LEN},
    {"LONG_NAME_LEN", LONG_NAME_LEN},
    {"MAX_NAME_LEN", MAX_NAME_LEN},
    {"SHORT_STR_LEN", SHORT_STR_LEN},
    {"TIME_LEN", TIME_LEN},
    {"DIR_LEN", DIR_LEN},
    {"ERR_MSG_LEN", ERR_MSG_LEN},
    {"MAX_SQL_ATTR", MAX_SQL_ATTR},
    {"RULE_SET_DEF_LENGTH", RULE_SET_DEF_LENGTH},
    {"META_STR_LEN", META_STR_LEN},
    {"CHALLENGE_LEN", CHALLENGE_LEN},
    {"RESPONSE_LEN", RESPONSE_LEN},
    {"MAX_PASSWORD_LEN", MAX_PASSWORD_LEN},
    /* HDF5 constant */
    {"MAX_ERROR_SIZE", 80},
    {"OBJID_DIM", 2},
    {"H5S_MAX_RANK", 32},
    {"H5DATASPACE_MAX_RANK", 32},
    {"HUGE_NAME_LEN", HUGE_NAME_LEN},
    {"MAX_NUM_CONFIG_TRAN_THR", MAX_NUM_CONFIG_TRAN_THR},
    /* end of HDF5 */
    {PACK_TABLE_END_PI, 0},
};


const packInstruct_t RodsPackTable[] = {
    {"STR_PI", STR_PI, NULL},
    {"IRODS_STR_PI", IRODS_STR_PI, NULL},
    {"STR_PTR_PI", STR_PTR_PI, NULL},
    {"INT_PI", INT_PI, NULL},
    {"CHAR_PI", CHAR_PI, NULL},
    {"DOUBLE_PI", DOUBLE_PI, NULL},
    {"FLOAT_PI", INT_PI, NULL},           /* pack as if it is INT_PI */
    {"BOOL_PI", INT_PI, NULL},            /* pack as if it is INT_PI */
    {"BUF_LEN_PI", BUF_LEN_PI, NULL},
    {"INT16_PI", INT16_PI, NULL},
    {"MsgHeader_PI", MsgHeader_PI, NULL},
    {"StartupPack_PI", StartupPack_PI, NULL},
    {"Version_PI", Version_PI, NULL},
    {"RErrMsg_PI", RErrMsg_PI, NULL},
    {"RError_PI", RError_PI, NULL},
    {"RHostAddr_PI", RHostAddr_PI, NULL},
    {"RODS_STAT_T_PI", RODS_STAT_T_PI, NULL},
    {"RODS_DIRENT_T_PI", RODS_DIRENT_T_PI, NULL},
    {"KeyValPair_PI", KeyValPair_PI, NULL},
    {"InxIvalPair_PI", InxIvalPair_PI, NULL},
    {"InxValPair_PI", InxValPair_PI, NULL},
    {"PortList_PI", PortList_PI, NULL},
    {"PortalOprOut_PI", PortalOprOut_PI, NULL},
    {"PortList_PI", PortList_PI, NULL},
    {"DataOprInp_PI", DataOprInp_PI, NULL},
    {"GenQueryInp_PI", GenQueryInp_PI, NULL},
    {"SqlResult_PI", SqlResult_PI, NULL},
    {"GenQueryOut_PI", GenQueryOut_PI, NULL},
    {"DataObjInfo_PI", DataObjInfo_PI, NULL},
    {"TransStat_PI", TransStat_PI, NULL},
    {"TransferStat_PI", TransferStat_PI, NULL},
    {"AuthInfo_PI", AuthInfo_PI, NULL},
    {"UserOtherInfo_PI", UserOtherInfo_PI, NULL},
    {"UserInfo_PI", UserInfo_PI, NULL},
    {"CollInfo_PI", CollInfo_PI, NULL},
    {"Rei_PI", Rei_PI, NULL},
    {"ReArg_PI", ReArg_PI, NULL},
    {"ReiAndArg_PI", ReiAndArg_PI, NULL},
    {"BytesBuf_PI", BytesBuf_PI, NULL},
    {"charDataArray_PI", charDataArray_PI, NULL},
    {"strDataArray_PI", strDataArray_PI, NULL},
    {"intDataArray_PI", intDataArray_PI, NULL},
    {"int16DataArray_PI", int16DataArray_PI, NULL},
    {"int64DataArray_PI", int64DataArray_PI, NULL},
    {"BinBytesBuf_PI", BinBytesBuf_PI, NULL},
    {"MsParam_PI", MsParam_PI, NULL},
    {"MsParamArray_PI", MsParamArray_PI, NULL},
    {"TagStruct_PI", TagStruct_PI, NULL},
    {"RodsObjStat_PI", RodsObjStat_PI, NULL},
    {"ReconnMsg_PI", ReconnMsg_PI, NULL},
    {"VaultPathPolicy_PI", VaultPathPolicy_PI, NULL},
    {"IntArray_PI", IntArray_PI, NULL},
    {"SpecColl_PI", SpecColl_PI, NULL},
    {"SubFile_PI", SubFile_PI, NULL},
    /* HDF5 PI */
    {"h5File_PI", h5File_PI, NULL},
    {"h5error_PI", h5error_PI, NULL},
    {"h5Group_PI", h5Group_PI, NULL},
    {"h5Attribute_PI", h5Attribute_PI, NULL},
    {"h5Dataset_PI", h5Dataset_PI, NULL},
    {"h5Datatype_PI", h5Datatype_PI, NULL},
    {"h5Dataspace_PI", h5Dataspace_PI, NULL},
    /* end of HDF5 */
    {"CollEnt_PI", CollEnt_PI, NULL},
    {"CollOprStat_PI", CollOprStat_PI, NULL},
    {"RuleStruct_PI", RuleStruct_PI, NULL},
    {"DVMapStruct_PI", DVMapStruct_PI, NULL},
    {"FNMapStruct_PI", FNMapStruct_PI, NULL},
    {"MsrvcStruct_PI", MsrvcStruct_PI, NULL},
    {"DataSeg_PI", DataSeg_PI, NULL},
    {"FileRestartInfo_PI", FileRestartInfo_PI, NULL},
    {"CS_NEG_PI", CS_NEG_PI, NULL},
    {"StrArray_PI", StrArray_PI, NULL},
    {PACK_TABLE_END_PI, ( char * ) NULL, NULL},
};

#endif	// RODS_PACK_TABLE_H__
