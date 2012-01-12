/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* xsltMS.h - header file for xsltMS.c
 */

#ifndef XSLTMS_H
#define XSLTMS_H

#include "apiHeaderAll.h"
#include "objMetaOpr.h"
#include "miscUtil.h"


int msiXsltApply(msParam_t *xsltObj, msParam_t *xmlObj, msParam_t *msParamOut, ruleExecInfo_t *rei);


#endif	/* XSLTMS_H */

