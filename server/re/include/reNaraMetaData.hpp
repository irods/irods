/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* reNaraMetaData.h - header file for reNaraMetaData.c
 */



#ifndef RE_NARA_META_DATA_HPP
#define RE_NARA_META_DATA_HPP

#include "rods.h"
#include "objMetaOpr.hpp"
#include "dataObjRepl.h"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"

#define NARA_META_DATA_FILE	"naraMetaData.txt"
int
msiExtractNaraMetadata( ruleExecInfo_t *rei );

#endif	/* RE_NARA_META_DATA_H */
