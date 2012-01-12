
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef HDF5_MS_H
#define HDF5_MS_H

#include "rodsClient.h"
#include "apiHeaderAll.h"
#include "h5Object.h"
#include "h5File.h"
#include "h5Group.h"
#include "h5Dataset.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Server side microservices and routines :
 *     process request from the client and send result to the client
 */
int
msiH5File_open (msParam_t *inpH5FileParam, msParam_t *inpFlagParam,
msParam_t *outH5FileParam, ruleExecInfo_t *rei);
int
msiH5File_close (msParam_t *inpH5FileParam, msParam_t *outParam,
ruleExecInfo_t *rei);
int
msiH5Dataset_read (msParam_t *inpH5DatasetParam, msParam_t *outH5DatasetParam,
ruleExecInfo_t *rei);
int
msiH5Dataset_read_attribute (msParam_t *inpH5DatasetParam, 
msParam_t *outH5DatasetParam, ruleExecInfo_t *rei);
int
msiH5Group_read_attribute (msParam_t *inpH5GroupParam,
msParam_t *outH5GroupParam, ruleExecInfo_t *rei);
int
getL1descInxByFid (int fid);

#ifdef __cplusplus
}
#endif

#endif


