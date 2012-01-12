/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* xmlMS.h - header file for xmlMS.c
 */

#ifndef XMLMS_H
#define XMLMS_H

#include "apiHeaderAll.h"
#include "objMetaOpr.h"
#include "miscUtil.h"


int msiLoadMetadataFromXml(msParam_t *targetObj, msParam_t *xmlObj, ruleExecInfo_t *rei);
int msiXmlDocSchemaValidate(msParam_t *xmlObj, msParam_t *xsdObj, msParam_t *status, ruleExecInfo_t *rei);


#endif	/* XMLMS_H */

