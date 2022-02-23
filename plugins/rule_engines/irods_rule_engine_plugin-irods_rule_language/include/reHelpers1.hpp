/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* reHelpers1.h - common header file for rods server and agents
 */



#ifndef RE_HELPERS1_HPP
#define RE_HELPERS1_HPP

#include "reGlobalsExtern.hpp"
#include "irods/irods_re_structs.hpp"

int
checkRuleCondition( char *action, char *incond,
                    char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc, ruleExecInfo_t *rei,
                    int reiSaveFlag );
int
checkRuleConditionNew( char *action, char *incond,
                       msParamArray_t *inMsParamArray, ruleExecInfo_t *rei, int reiSaveFlag );
int computeExpression( char *expr, msParamArray_t *msParamArray, ruleExecInfo_t *rei, int reiSaveFlag, char *res );
int
replaceVariables( char *action, char *inStr,
                  char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc, ruleExecInfo_t *rei );
int
replaceVariablesNew( char *action, char *inStr, msParamArray_t *inMsParamArray,
                     ruleExecInfo_t *rei );
int
replaceVariablesAndMsParams( char *action, char *inStr, msParamArray_t *inMsParamArray, ruleExecInfo_t *rei );
int
replaceMsParams( char *inStr, msParamArray_t *inMsParamArray );
int
replaceDollarParam( char *action, char *dPtr, int len,
                    char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
                    ruleExecInfo_t *rei );
int
reREMatch( char *pat, char *str );

int _computeExpression( char *expr1, char *expr2, char *oper1, ruleExecInfo_t *rei,
                        int reiSaveFlag , char *res );
int convertArgWithVariableBinding( char *inS, char **outS, msParamArray_t *inMsParamArray,
                                   ruleExecInfo_t *rei );
#endif	/* RE_HELPERS1_H */
