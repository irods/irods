/* For copyright information please refer to files in the COPYRIGHT directory
 */


#ifndef RECONSTANTS_H
#define RECONSTANTS_H
#include "debug.h"

#define MAX_PARAMS_LEN 100
#define MAX_RULE_LEN (1024 * 64)
#define MAX_NUM_DISJUNCTS 100

#ifdef DEBUG
/* parser error -1203000 */
#ifndef PARSER_ERROR
#define PARSER_ERROR -1201000
#define UNPARSED_SUFFIX -1202000
#define POINTER_ERROR -1203000
/* runtime error -1205000 */
#define RUNTIME_ERROR -1205000
#define DIVISION_BY_ZERO -1206000
#define BUFFER_OVERFLOW -1207000
#define UNSUPPORTED_OP_OR_TYPE -1208000
#define UNSUPPORTED_SESSION_VAR -1209000
#define UNABLE_TO_WRITE_LOCAL_VAR -1210000
#define UNABLE_TO_READ_LOCAL_VAR -1211000
#define UNABLE_TO_WRITE_SESSION_VAR -1212000
#define UNABLE_TO_READ_SESSION_VAR -1213000
#define UNABLE_TO_WRITE_VAR -1214000
#define UNABLE_TO_READ_VAR -1215000
#define PATTERN_NOT_MATCHED -1216000
#define STRING_OVERFLOW -1217000
/* system error -1220000 */
#define UNKNOWN_ERROR -1220000
#define OUT_OF_MEMORY -1221000
#define SHM_UNLINK_ERROR -1222000
#define FILE_STAT_ERROR -1223000
/* type error -1230000 */
#define TYPE_ERROR -1230000
#define FUNCTION_REDEFINITION -1231000
#define DYNAMIC_TYPE_ERROR -1232000
#endif
#endif

#define DATETIME_MS_T "DATETIME_MS_T"

#define LIST "[]"
#define TUPLE "<>"
#define APPLICATION "()"
#define META_DATA "@()"
#define AVU "avu"
#define ST_TUPLE "()"
#define FUNC "->"
#define ERR_MSG_SEP "=========="

/* todo change text of dynamically allocated array */
#define MAX_TOKEN_TEXT_LEN 1023

/* #define PARSER_LAZY 0 */
#define MAX_FUNC_PARAMS 20
#define MAX_NUM_RULES 50000
#define CORE_RULE_INDEX_OFF 30000
#define APP_RULE_INDEX_OFF 10000

#define MAX_PREC 20
#define MIN_PREC 0

#define POINTER_BUF_SIZE (16*1024)

#define RULE_ENGINE_INIT_CACHE 0
#define RULE_ENGINE_TRY_CACHE 1
#define RULE_ENGINE_NO_CACHE 2
#define RULE_ENGINE_REFRESH_CACHE 3

#endif
