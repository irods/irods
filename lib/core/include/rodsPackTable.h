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
    {"STR_PI", STR_PI, nullptr},
    {"IRODS_STR_PI", IRODS_STR_PI, nullptr},
    {"STR_PTR_PI", STR_PTR_PI, nullptr},
    {"INT_PI", INT_PI, nullptr},
    {"CHAR_PI", CHAR_PI, nullptr},
    {"DOUBLE_PI", DOUBLE_PI, nullptr},
    {"FLOAT_PI", INT_PI, nullptr},           /* pack as if it is INT_PI */
    {"BOOL_PI", INT_PI, nullptr},            /* pack as if it is INT_PI */
    {"BUF_LEN_PI", BUF_LEN_PI, nullptr},
    {"INT16_PI", INT16_PI, nullptr},
    {"MsgHeader_PI", MsgHeader_PI, nullptr},
    {"StartupPack_PI", StartupPack_PI, nullptr},
    {"Version_PI", Version_PI, nullptr},
    {"RErrMsg_PI", RErrMsg_PI, nullptr},
    {"RError_PI", RError_PI, nullptr},
    {"RHostAddr_PI", RHostAddr_PI, nullptr},
    {"RODS_STAT_T_PI", RODS_STAT_T_PI, nullptr},
    {"RODS_DIRENT_T_PI", RODS_DIRENT_T_PI, nullptr},
    {"KeyValPair_PI", KeyValPair_PI, nullptr},
    {"InxIvalPair_PI", InxIvalPair_PI, nullptr},
    {"InxValPair_PI", InxValPair_PI, nullptr},
    {"PortList_PI", PortList_PI, nullptr},
    {"PortalOprOut_PI", PortalOprOut_PI, nullptr},
    {"PortList_PI", PortList_PI, nullptr},
    {"DataOprInp_PI", DataOprInp_PI, nullptr},
    {"GenQueryInp_PI", GenQueryInp_PI, nullptr},
    {"SqlResult_PI", SqlResult_PI, nullptr},
    {"GenQueryOut_PI", GenQueryOut_PI, nullptr},
    {"DataObjInfo_PI", DataObjInfo_PI, nullptr},
    {"TransStat_PI", TransStat_PI, nullptr},
    {"TransferStat_PI", TransferStat_PI, nullptr},
    {"AuthInfo_PI", AuthInfo_PI, nullptr},
    {"UserOtherInfo_PI", UserOtherInfo_PI, nullptr},
    {"UserInfo_PI", UserInfo_PI, nullptr},
    {"CollInfo_PI", CollInfo_PI, nullptr},
    {"Rei_PI", Rei_PI, nullptr},
    {"ReArg_PI", ReArg_PI, nullptr},
    {"ReiAndArg_PI", ReiAndArg_PI, nullptr},
    {"BytesBuf_PI", BytesBuf_PI, nullptr},
    {"charDataArray_PI", charDataArray_PI, nullptr},
    {"strDataArray_PI", strDataArray_PI, nullptr},
    {"intDataArray_PI", intDataArray_PI, nullptr},
    {"int16DataArray_PI", int16DataArray_PI, nullptr},
    {"int64DataArray_PI", int64DataArray_PI, nullptr},
    {"BinBytesBuf_PI", BinBytesBuf_PI, nullptr},
    {"MsParam_PI", MsParam_PI, nullptr},
    {"MsParamArray_PI", MsParamArray_PI, nullptr},
    {"TagStruct_PI", TagStruct_PI, nullptr},
    {"RodsObjStat_PI", RodsObjStat_PI, nullptr},
    {"ReconnMsg_PI", ReconnMsg_PI, nullptr},
    {"VaultPathPolicy_PI", VaultPathPolicy_PI, nullptr},
    {"IntArray_PI", IntArray_PI, nullptr},
    {"SpecColl_PI", SpecColl_PI, nullptr},
    {"SubFile_PI", SubFile_PI, nullptr},
    {"XmsgTicketInfo_PI", XmsgTicketInfo_PI, nullptr},
    {"SendXmsgInfo_PI", SendXmsgInfo_PI, nullptr},
    {"RcvXmsgInp_PI", RcvXmsgInp_PI, nullptr},
    {"RcvXmsgOut_PI", RcvXmsgOut_PI, nullptr},
    /* HDF5 PI */
    {"h5File_PI", h5File_PI, nullptr},
    {"h5error_PI", h5error_PI, nullptr},
    {"h5Group_PI", h5Group_PI, nullptr},
    {"h5Attribute_PI", h5Attribute_PI, nullptr},
    {"h5Dataset_PI", h5Dataset_PI, nullptr},
    {"h5Datatype_PI", h5Datatype_PI, nullptr},
    {"h5Dataspace_PI", h5Dataspace_PI, nullptr},
    /* end of HDF5 */
    {"CollEnt_PI", CollEnt_PI, nullptr},
    {"CollOprStat_PI", CollOprStat_PI, nullptr},
    {"RuleStruct_PI", RuleStruct_PI, nullptr},
    {"DVMapStruct_PI", DVMapStruct_PI, nullptr},
    {"FNMapStruct_PI", FNMapStruct_PI, nullptr},
    {"MsrvcStruct_PI", MsrvcStruct_PI, nullptr},
    {"DataSeg_PI", DataSeg_PI, nullptr},
    {"FileRestartInfo_PI", FileRestartInfo_PI, nullptr},
    {"CS_NEG_PI", CS_NEG_PI, nullptr},
    {"StrArray_PI", StrArray_PI, nullptr},
    {PACK_TABLE_END_PI, ( char * ) nullptr, nullptr},
};

#endif	// RODS_PACK_TABLE_H__
