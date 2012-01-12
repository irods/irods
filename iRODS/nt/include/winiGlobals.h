#ifndef __WINI_OBJECT_GLOBALS_H__
#define __WINI_OBJECT_GLOBALS_H__

#define c_plus_plus	1

#define NULL 0

#define FED_Catalog 1

//comment this out for now when converting to WINI
//#include "scommands.h"

#define WINI_NONE		0
#define WINI_DATASET	1
#define WINI_COLLECTION	2
#define WINI_METADATA	4
#define WINI_ACCESS		8
#define WINI_RESOURCE	16
#define WINI_QUERY		32
#define WINI_DOMAIN		64	
#define WINI_USER		128
#define WINI_QUERY		256
#define WINI_ZONE		512
#define WINI_CATALOG	1024
#define WINI_SET		2048
#define WINI_ALL		4095

#define WINI_OP_NONE				0
#define WINI_OP_IS_TRANSFER_OP	1
#define WINI_OP_COLLECTION		3	/*2 + 1*/
#define WINI_OP_CONTAINER		4
#define WINI_OP_DATASET			9	/*8 + 1*/
#define WINI_OP_DOMAIN			16
#define WINI_OP_QUERY			32
#define WINI_OP_RESOURCE			64
#define WINI_OP_ZONE				128
#define WINI_OP_Catalog				256
#define WINI_OP_ALL				2047

#define WINI_NO_OVERWRITE	0
#define WINI_OVERWRITE		1
#define WINI_OVERWRITE_ALL	3	//includes OVERWRITE

static char* op_map[] = {"AND", "OR", "=", "<>", ">","<", ">=", "<=", "between", "not between", "like", "not like", "sounds like", "sounds not like"};


//#define WINI_DATASET 1
//#define WINI_COLLECTION 2
//#define WINI_METADATA 3
//#define WINI_QUERY 4
//#define WINI_QUERY2 5
//#define WINI_SORT 6
//#define WINI_RESOURCE 7
//#define WINI_CONTAINER 8
//#define WINI_USER 9
//#define WINI_DOMAIN 10
//#define WINI_GROUP 11
//#define WINI_ACCESS 12







#define MAX_ROWS 400

//this block of error codes should be defined in mdas_errno.h
#define WINI_OK 0
#define WINI_ERROR_NOT_FILLED -7000
#define WINI_GENERAL_ERROR -7001
#define WINI_ERROR -7001
#define WINI_ERROR_INVALID_PARAMETER -7002
#define WINI_ERROR_INVALID_BINDING -7005
#define WINI_ERROR_NOT_CHILD -7003
#define WINI_ERROR_BUSY -7004
//-7005 used
#define WINI_ERROR_TYPE_NOT_SUPPORTED -7006
#define WINI_ERROR_LOGIN_FAILED -7007
#define WINI_ERROR_NOT_SUPPORTED -7008
#define WINI_ERROR_CHILD_NOT_FOUND -7009
#define WINI_ERROR_DOES_NOT_EXIST -7010
#define WINI_ERROR_NO_HOME_COLLECTION -7011
#define WINI_ERROR_SOURCE_EQUALS_TARGET -7012
#define WINI_ERROR_UNCLE_NOT_FOUND -7013
#define WINI_ERROR_QUERY_DELETE_FORBIDDEN -7014
#define WINI_ERROR_NOT_IMPLEMENTED -7015

#define WINI_MD_AND 0
#define WINI_MD_OR 1
#define WINI_MD_EQUAL 2
#define WINI_MD_NOT_EQUAL 3
#define WINI_MD_GREATER_THAN 4
#define WINI_MD_LESS_THAN 5
#define WINI_MD_GREATER_OR_EQUAL 6
#define WINI_MD_LESS_OR_EQUAL 7
#define WINI_MD_BETWEEN 8
#define WINI_MD_NOT_BETWEEN 9
#define WINI_MD_LIKE 10
#define WINI_MD_NOT_LIKE 11
#define WINI_MD_SOUNDS_LIKE 12
#define WINI_MD_SOUNDS_NOT_LIKE 13
#define WINI_MD_SORT 14

#define SMART_FREE(variable) \
	if(variable){ \
		free(variable); \
variable = NULL;} \

#define SMART_DELETE(variable) \
	if(variable){ \
		delete variable; \
		variable = NULL;} \

#define SMART_DELETE_ARRAY(variable) \
	if(variable){ \
		delete [] variable; \
		variable = NULL;} \

#define LOG(MESSAGE) \
	m_file.open("c:\\soblog.txt", ios::out); \
	m_file << MESSAGE; \
	m_file.close(); \



#endif __WINI_OBJECT_GLOBALS_H__