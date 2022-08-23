/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* reHelpers1.h - common header file for rods server and agents
 */



#ifndef RE_HELPERS1_HPP
#define RE_HELPERS1_HPP

#include "irods/irods_re_structs.hpp"

int computeExpression( char *expr, msParamArray_t *msParamArray, ruleExecInfo_t *rei, int reiSaveFlag, char *res );

#endif	/* RE_HELPERS1_H */
