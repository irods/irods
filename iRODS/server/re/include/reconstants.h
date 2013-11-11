/* For copyright information please refer to files in the COPYRIGHT directory
 */


#ifndef RECONSTANTS_H
#define RECONSTANTS_H

#define MAX_PARAMS_LEN 100
#define MAX_RULE_LEN (1024 * 64)
#define MAX_NUM_DISJUNCTS 100

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
