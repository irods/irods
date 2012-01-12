/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rodsPackTable.h - The common rods pack Instruction table
 */

#ifndef RODS_PACK_TABLE_H
#define RODS_PACK_TABLE_H

#include "rods.h"
#include "packStruct.h"
#include "rodsPackInstruct.h"
#include "rodsGenQuery.h"
#include "reGlobalsExtern.h"
#include "apiHeaderAll.h"

#define UNKNOWN_SIZE	-1

packType_t packTypeTable[] = {
        {"char", PACK_CHAR_TYPE, sizeof (char)},
        {"bin", PACK_BIN_TYPE, sizeof (char)},
        {"str", PACK_STR_TYPE, sizeof (char)},
        {"piStr", PACK_PI_STR_TYPE, sizeof (char)},   /* str containing pi */
        {"int", PACK_INT_TYPE, 4},
        {"double", PACK_DOUBLE_TYPE, 8},
        {"struct", PACK_STRUCT_TYPE, UNKNOWN_SIZE},
        {"?", PACK_DEPENDENT_TYPE, UNKNOWN_SIZE},
        {"%", PACK_INT_DEPENDENT_TYPE, UNKNOWN_SIZE},
};

int NumOfPackTypes = (sizeof (packTypeTable) / sizeof (packType_t));
 
packConstantArray_t PackConstantTable[] = {
        {"HEADER_TYPE_LEN", HEADER_TYPE_LEN},
        {"NAME_LEN", NAME_LEN},
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


packInstructArray_t RodsPackTable[] = {
	{"STR_PI", STR_PI},
	{"IRODS_STR_PI", IRODS_STR_PI},
	{"STR_PTR_PI", STR_PTR_PI},
	{"INT_PI", INT_PI},
	{"DOUBLE_PI", DOUBLE_PI},
	{"BUF_LEN_PI", BUF_LEN_PI},
	{"MsgHeader_PI", MsgHeader_PI},
	{"StartupPack_PI", StartupPack_PI},
	{"Version_PI", Version_PI},
	{"RErrMsg_PI", RErrMsg_PI},
	{"RError_PI", RError_PI},
	{"RHostAddr_PI", RHostAddr_PI},
	{"RODS_STAT_T_PI", RODS_STAT_T_PI},
	{"RODS_DIRENT_T_PI", RODS_DIRENT_T_PI},
	{"KeyValPair_PI", KeyValPair_PI},
	{"InxIvalPair_PI", InxIvalPair_PI},
	{"InxValPair_PI", InxValPair_PI},
	{"PortList_PI", PortList_PI},
	{"PortalOprOut_PI", PortalOprOut_PI},
	{"PortList_PI", PortList_PI},
	{"DataOprInp_PI", DataOprInp_PI},
	{"GenQueryInp_PI", GenQueryInp_PI},
	{"SqlResult_PI", SqlResult_PI},
	{"GenQueryOut_PI", GenQueryOut_PI},
	{"DataObjInfo_PI", DataObjInfo_PI},
	{"TransStat_PI", TransStat_PI},
	{"TransferStat_PI", TransferStat_PI},
	{"RescGrpInfo_PI", RescGrpInfo_PI},
	{"AuthInfo_PI", AuthInfo_PI},
	{"UserOtherInfo_PI", UserOtherInfo_PI},
	{"UserInfo_PI", UserInfo_PI},
	{"CollInfo_PI", CollInfo_PI},
	{"Rei_PI", Rei_PI},
	{"ReArg_PI", ReArg_PI},
	{"ReiAndArg_PI", ReiAndArg_PI},
	{"BytesBuf_PI", BytesBuf_PI},
	{"BinBytesBuf_PI", BinBytesBuf_PI},
	{"MsParam_PI", MsParam_PI},
	{"MsParamArray_PI", MsParamArray_PI},
	{"TagStruct_PI", TagStruct_PI},
	{"RodsObjStat_PI", RodsObjStat_PI},
	{"ReconnMsg_PI", ReconnMsg_PI},
	{"VaultPathPolicy_PI", VaultPathPolicy_PI},
	{"IntArray_PI", IntArray_PI},
	{"SpecColl_PI", SpecColl_PI},
	{"SubFile_PI", SubFile_PI},
	{"XmsgTicketInfo_PI", XmsgTicketInfo_PI},
	{"SendXmsgInfo_PI", SendXmsgInfo_PI},
	{"RcvXmsgInp_PI", RcvXmsgInp_PI},
	{"RcvXmsgOut_PI", RcvXmsgOut_PI},
	/* HDF5 PI */
        {"h5File_PI", h5File_PI},
        {"h5error_PI", h5error_PI},
        {"h5Group_PI", h5Group_PI},
        {"h5Attribute_PI", h5Attribute_PI},
        {"h5Dataset_PI", h5Dataset_PI},
        {"h5Datatype_PI", h5Datatype_PI},
        {"h5Dataspace_PI", h5Dataspace_PI},
	/* end of HDF5 */ 
        {"CollEnt_PI", CollEnt_PI},
        {"CollOprStat_PI", CollOprStat_PI},
        {"RuleStruct_PI", RuleStruct_PI},
	{"DVMapStruct_PI", DVMapStruct_PI},
	{"FNMapStruct_PI", FNMapStruct_PI},
        {"MsrvcStruct_PI", MsrvcStruct_PI},
        {"DataSeg_PI", DataSeg_PI},
        {"FileRestartInfo_PI", FileRestartInfo_PI},
        {PACK_TABLE_END_PI, (char *) NULL},
};

#endif	/* RODS_PACK_TABLE_H */
