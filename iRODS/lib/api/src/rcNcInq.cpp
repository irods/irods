/**
 * @file  rcNcInq.c
 *
 */

/* This is script-generated code.  */ 
/* See ncInq.h for a description of this API call.*/

#include "ncInq.h"

/**
 * \fn rcNcInq (rcComm_t *conn, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut)
 *
 * \brief General netcdf inquiry (equivalent to nc_inq + nc_inq_format).
 *          Get all info including attributes, dimensions and variables
 *          assocaiated with a NETCDF file will a single call.
 *
 * \user client
 *
 * \category NETCDF operations
 *
 * \since 3.1
 *
 * \author  Mike Wan
 * \date    2012
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * nc_inq an opened data object and print the header info:
 * \n ncOpenInp_t ncOpenInp;
 * \n ncInqInp_t ncInqInp;
 * \n int ncid = 0;
* \n int status;
 * \n bzero (&ncOpenInp, sizeof (ncOpenInp));
 * \n rstrcpy (ncOpenInp.objPath, "/myZone/home/john/myfile.nc", MAX_NAME_LEN);
 * \n ncOpenInp.mode = NC_NOWRITE;
 * \n status = rcNcOpen (conn, &ncOpenInp, &ncid);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 * \n   bzero (&ncInqInp, sizeof (ncInqInp));
 * \n    ncInqInp.ncid = ncid;
 * \n   ncInqInp.paramType = NC_ALL_TYPE;
 * \n    ncInqInp.flags = NC_ALL_FLAG;
 * \n    status = rcNcInq (conn, &ncInqInp, &ncInqOut);
 * \n .... handle the error
 * \n    if (status < 0) {
 * \n    }
 * \n status = prNcHeader (conn, ncid, False, ncInqOut);
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] ncInqInp - Elements of ncInqInp_t used :
 *    \li int \b paramType - what to inquire - valid values are defined in ncInq.h - NC_VAR_TYPE, NC_DIM_TYPE, NC_ATT_TYPE and NC_ALL_TYPE.
 *    \li int \b ncid - the ncid from ncNcOpen.
 *    \li int \b myid - the id (from rcNcInqId) to inquire. Meaning only if NC_ALL_TYPE == 0.
 *    \li char \b name[MAX_NAME_LEN] - the name of the item to inquire. Either  myid or name, but not both, can be input. Meaning only if NC_ALL_TYPE == 0.
 * \param[out] ncInqOut - a ncInqOut_t. Elements of ncInqOut_t:
 *    \li int \b ndims - number of dimensions
 *    \li ncGenDimOut_t *dim - an array with ndims length of ncGenDimOut_t struct
 *    \li int \b ngatts - number of global attributes
 *    \li ncGenAttOut_t *gatt - an array with ngatts length of ncGenAttOut_t struct
 *    \li int \b nvars - number of variables 
 *    \li ncGenAttOut_t *var - an array with nvars length of ncGenVarOut_t struct
 *    \li int \b unlimdimid - the dimid of the unlimited dimension
 *    \li int \b format - NC_FORMAT_CLASSIC, NC_FORMAT_64BIT, NC_FORMAT_NETCDF4 or NC_FORMAT_NETCDF4_CLASSIC
 *
 * \return integer
 * \retval status of the call. success if greater or equal 0. error if negative.

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcNcInq (rcComm_t *conn, ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut)
{
    int status;
    status = procApiRequest (conn, NC_INQ_AN, ncInqInp, NULL, 
        (void **) ncInqOut, NULL);

    return (status);
}

int
initNcInqOut (int ndims, int nvars, int ngatts, int unlimdimid, int format,
ncInqOut_t **ncInqOut)
{
    *ncInqOut = (ncInqOut_t *) calloc (1, sizeof (ncInqOut_t));
    (*ncInqOut)->ndims = ndims;
    (*ncInqOut)->nvars = nvars;
    (*ncInqOut)->ngatts = ngatts;
    (*ncInqOut)->unlimdimid = unlimdimid;
    (*ncInqOut)->format = format;

    if (ndims > 0)
        (*ncInqOut)->dim = (ncGenDimOut_t *) 
	  calloc (ndims, sizeof (ncGenDimOut_t));
    if (nvars > 0) 
        (*ncInqOut)->var = (ncGenVarOut_t *) 
	  calloc (nvars, sizeof (ncGenVarOut_t));
    if (ngatts > 0)
        (*ncInqOut)->gatt = (ncGenAttOut_t *) 
	  calloc (ngatts, sizeof (ncGenAttOut_t));

    return 0;
}

int
freeNcInqOut (ncInqOut_t **ncInqOut)
{
    int i;

    if (ncInqOut == NULL || *ncInqOut == NULL) return USER__NULL_INPUT_ERR;

    if ((*ncInqOut)->dim != NULL) free ((*ncInqOut)->dim);
    if ((*ncInqOut)->gatt != NULL) free ((*ncInqOut)->gatt);
    if ((*ncInqOut)->var != NULL) {
	for (i = 0; i < (*ncInqOut)->nvars; i++) {
	    if ((*ncInqOut)->var[i].att != NULL) {
#if 0	/* this can cause core dump */
		clearNcGetVarOut (&(*ncInqOut)->var[i].att->value);
		free ((*ncInqOut)->var[i].att);
#endif
	    }
	    if ((*ncInqOut)->var[i].dimId != NULL) 
	        free ((*ncInqOut)->var[i].dimId);
	}
	free ((*ncInqOut)->var);
    }
    free (*ncInqOut);
    *ncInqOut = NULL;

    return 0;
}

int
prFirstNcLine (char *objPath)
{
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];

    if (objPath == NULL || splitPathByKey (objPath, myDir, myFile, '/') < 0) {
        printf ("netcdf UNKNOWN_FILE {\n");
    } else {
        int len = strlen (myFile);
        char *myptr = myFile + len - 3;
        if (strcmp (myptr, ".nc") == 0) *myptr = '\0';
        printf ("netcdf %s {\n", myFile);
    }
    return 0;
}

int
prNcHeader (rcComm_t *conn, int ncid, int noattr, ncInqOut_t *ncInqOut)
{
    int i, j, dimId, status;
    char tempStr[NAME_LEN];
    void *bufPtr;
#if 0
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];

    if (fileName == NULL || splitPathByKey (fileName, myDir, myFile, '/') < 0) {
	printf ("netcdf UNKNOWN_FILE {\n");
    } else {
	int len = strlen (myFile);
	char *myptr = myFile + len - 3;
	if (strcmp (myptr, ".nc") == 0) *myptr = '\0'; 
	printf ("netcdf %s {\n", myFile);
    }
#endif

    /* attrbutes */
    if (noattr == False) {
        for (i = 0; i < ncInqOut->ngatts; i++) {
            bufPtr = ncInqOut->gatt[i].value.dataArray->buf;
            printf ("   %s = \n", ncInqOut->gatt[i].name);
	    if (ncInqOut->gatt[i].dataType == NC_CHAR) {
                /* assume it is a string */
                /* printf ("%s ;\n", (char *) bufPtr); */
                if (printNice ((char *) bufPtr, "      ", 72) < 0) 
                    printf ("     %s", (char *) bufPtr);
            } else {
                ncValueToStr (ncInqOut->gatt[i].dataType, &bufPtr, tempStr);
                if (printNice (tempStr, "      ", 72) < 0)
                    printf ("     %s", (char *) bufPtr);
            }
            printf (";\n");
        }
    }

    /* dimensions */
    if (ncInqOut->ndims <= 0 || ncInqOut->dim == NULL) 
	return USER__NULL_INPUT_ERR;
    printf ("dimensions:\n");
    for (i = 0; i < ncInqOut->ndims; i++) {
	if (ncInqOut->unlimdimid == ncInqOut->dim[i].id) {
	    /* unlimited */
	    printf ("   %s = UNLIMITED ; // (%lld currently)\n", 
	      ncInqOut->dim[i].name, ncInqOut->dim[i].arrayLen);
	} else { 
	    printf ("   %s = %lld ;\n", 
	      ncInqOut->dim[i].name, ncInqOut->dim[i].arrayLen);
	}
    }
    /* variables */
    if (ncInqOut->nvars <= 0 || ncInqOut->var == NULL)
        return USER__NULL_INPUT_ERR;
    printf ("variables:\n");
    for (i = 0; i < ncInqOut->nvars; i++) {
	status = getNcTypeStr (ncInqOut->var[i].dataType, tempStr);
	if (status < 0) return status;
        printf ("   %s %s(", tempStr, ncInqOut->var[i].name);
	for (j = 0; j < ncInqOut->var[i].nvdims - 1; j++) {
	    dimId = ncInqOut->var[i].dimId[j];
	    printf ("%s, ",  ncInqOut->dim[dimId].name);
	}
	/* last one */
	if (ncInqOut->var[i].nvdims > 0) {
             dimId = ncInqOut->var[i].dimId[j];
             printf ("%s) ;\n", ncInqOut->dim[dimId].name);
        }
	/* print the attributes */
        if (noattr == False) {
	    for (j = 0; j < ncInqOut->var[i].natts; j++) {
	        ncGenAttOut_t *att = &ncInqOut->var[i].att[j];
                  printf ("     %s:%s =\n",   
	          ncInqOut->var[i].name, att->name);
	        bufPtr = att->value.dataArray->buf;
                if (att->dataType == NC_CHAR) {
                    /* assume it is a string */
                    /* printf ("%s ;\n", (char *) bufPtr); */
                    if (printNice ((char *) bufPtr, "         ", 70) < 0)
                        printf ("     %s", (char *) bufPtr);
                } else {
	            ncValueToStr (att->dataType, &bufPtr, tempStr);
	            /* printf ("%s ;\n", tempStr); */
                    if (printNice (tempStr, "         ", 70) < 0) 
                        printf ("     %s", (char *) bufPtr);
                }
                printf (";\n");
            }
        }
    }
    return 0;
}

int
prNcDimVar (rcComm_t *conn, char *fileName, int ncid, int printAsciTime,
ncInqOut_t *ncInqOut)
{
    int i, j, status = 0;

    /* dimensions */
    if (ncInqOut->ndims <= 0 || ncInqOut->dim == NULL)
        return USER__NULL_INPUT_ERR;
    printf ("dimensions:\n");
    for (i = 0; i < ncInqOut->ndims; i++) {
        if (ncInqOut->unlimdimid == ncInqOut->dim[i].id) {
            /* unlimited */
            printf ("    %s = UNLIMITED ; // (%lld currently)\n",
              ncInqOut->dim[i].name, ncInqOut->dim[i].arrayLen);
        } else {
            printf ("    %s = %lld ;\n",
              ncInqOut->dim[i].name, ncInqOut->dim[i].arrayLen);
        }
        /* get the dim variables */
        for (j = 0; j < ncInqOut->nvars; j++) {
            if (strcmp (ncInqOut->dim[i].name, ncInqOut->var[j].name) == 0) {
                break;
            }
        }
        if (j >= ncInqOut->nvars) {
            /* not found. tabledap allows this */
            continue;
#if 0
            rodsLogError (LOG_ERROR, status,
              "prNcDimVar: unmatched dim var  %s", ncInqOut->dim[i].id);
            return NETCDF_DIM_MISMATCH_ERR;
#endif
        }
        status = prSingleDimVar (conn, ncid, j, 10, printAsciTime, ncInqOut);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "prNcDimVar: prSingleDimVar error for %s",
              ncInqOut->var[j].name);
            return status;
        }
    }
    return status;
}

int
prSingleDimVar (rcComm_t *conn, int ncid, int varInx, 
int itemsPerLine, int printAsciTime, ncInqOut_t *ncInqOut)
{
    int j, status;
    rodsLong_t start[NC_MAX_DIMS], stride[NC_MAX_DIMS], count[NC_MAX_DIMS];
    ncGetVarOut_t *ncGetVarOut = NULL;
    int lastDimLen;
    char tempStr[NAME_LEN];
    void *bufPtr;
    int outCnt = 0;
    int itemsInLine = 0;

    status = getSingleNcVarData (conn, ncid, varInx, ncInqOut, NULL, 
      &ncGetVarOut, start, stride, count);
#if 0
    for (j = 0; j < ncInqOut->var[varInx].nvdims; j++) {
        int dimId = ncInqOut->var[varInx].dimId[j];
        start[j] = 0;
        if (dumpVarLen > 0 && ncInqOut->dim[dimId].arrayLen > dumpVarLen) {
            /* If it is NC_CHAR, it could be a str */
            if (ncInqOut->var[varInx].dataType == NC_CHAR &&
              j ==  ncInqOut->var[varInx].nvdims - 1) {
                count[j] = ncInqOut->dim[dimId].arrayLen;
            } else {
                count[j] = dumpVarLen;
            }
        } else {
            count[j] = ncInqOut->dim[dimId].arrayLen;
        }
        stride[j] = 1;
    }
    bzero (&ncGetVarInp, sizeof (ncGetVarInp));
    ncGetVarInp.dataType = ncInqOut->var[varInx].dataType;
    ncGetVarInp.ncid = ncid;
    ncGetVarInp.varid =  ncInqOut->var[varInx].id;
    ncGetVarInp.ndim =  ncInqOut->var[varInx].nvdims;
    ncGetVarInp.start = start;
    ncGetVarInp.count = count;
    ncGetVarInp.stride = stride;

    status = rcNcGetVarsByType (conn, &ncGetVarInp, &ncGetVarOut);
#endif

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "dumpNcInqOut: rcNcGetVarsByType error for %s",
          ncInqOut->var[varInx].name);
          return status;
    }
    /* print it */
    lastDimLen = count[ncInqOut->var[varInx].nvdims - 1];
    bufPtr = ncGetVarOut->dataArray->buf;
    bzero (tempStr, sizeof (tempStr));
    if (ncInqOut->var[varInx].dataType == NC_CHAR) {
        int nextLastDimLen;
        if (ncInqOut->var[varInx].nvdims >= 2) {
            nextLastDimLen = count[ncInqOut->var[varInx].nvdims - 2];
        } else {
            nextLastDimLen = 0;
        }
        for (j = 0; j < ncGetVarOut->dataArray->len; j+=lastDimLen) {
            /* treat it as strings */
            if (j + lastDimLen >= ncGetVarOut->dataArray->len - 1) {
                printf ("%s ;\n", (char *) bufPtr);
            } else if (outCnt >= nextLastDimLen) {
                /* reset */
                printf ("%s,\n  ", (char *) bufPtr);
                outCnt = 0;
            } else {
                printf ("%s, ", (char *) bufPtr);
            }
        }
    } else {
        for (j = 0; j < ncGetVarOut->dataArray->len; j++) {
            ncValueToStr (ncInqOut->var[varInx].dataType, &bufPtr, tempStr);
            outCnt++;
           if (printAsciTime == True && 
              strcasecmp (ncInqOut->var[varInx].name, "time") == 0) {
                /* asci time */
                time_t mytime =atoi (tempStr);
                timeToAsci (mytime, tempStr);
#if 0
                struct tm *mytm = gmtime (&mytime);
                if (mytm != NULL) {
                    snprintf (tempStr, NAME_LEN, 
                      "%04d-%02d-%02dT%02d:%02d:%02dZ",
                      1900+mytm->tm_year, mytm->tm_mon + 1, mytm->tm_mday,
                      mytm->tm_hour, mytm->tm_min, mytm->tm_sec);
                }
#endif
            }
            if (j >= ncGetVarOut->dataArray->len - 1) {
                printf ("%s ;\n", tempStr);
            } else if (itemsPerLine > 0) {
                int numbLine = outCnt / itemsPerLine;
                if (itemsInLine == 0) {
                    printf ("(%d - %d)  ", numbLine * itemsPerLine, 
                      numbLine * itemsPerLine + itemsPerLine -1);
                }
                itemsInLine++;
                if (itemsInLine >= itemsPerLine) {
                    printf ("%s,\n", tempStr);
                    itemsInLine = 0;
                } else {
                    printf ("%s, ", tempStr);
                }
            } else if (outCnt >= lastDimLen) {
                /* reset */
                printf ("%s,\n  ", tempStr);
                outCnt = 0;
            } else {
                printf ("%s, ", tempStr);
            }
        }
    }

    return status;
}

/* dumpVarLen 0 mean dump all. > 0 means dump the specified len */
int
dumpNcInqOut (rcComm_t *conn, char *fileName, int ncid, int dumpVarLen,
ncInqOut_t *ncInqOut)
{
    int status;

    prFirstNcLine (fileName);
    status = prNcHeader (conn, ncid, False, ncInqOut);
    if (status < 0) return status;

    if (dumpVarLen > 0) {
        int i;
        ncVarSubset_t ncVarSubset;

        bzero (&ncVarSubset, sizeof (ncVarSubset));
        ncVarSubset.numSubset = ncInqOut->ndims;
        for (i = 0; i < ncInqOut->ndims; i++) {
            rstrcpy (ncVarSubset.ncSubset[i].subsetVarName, 
              ncInqOut->dim[i].name, LONG_NAME_LEN);
            ncVarSubset.ncSubset[i].start = 0;
            ncVarSubset.ncSubset[i].stride = 1;
            if (dumpVarLen > ncInqOut->dim[i].arrayLen) {
                ncVarSubset.ncSubset[i].end = ncInqOut->dim[i].arrayLen - 1;
            } else {
                ncVarSubset.ncSubset[i].end = dumpVarLen -1;
            }
        }
        status = prNcVarData (conn, fileName, ncid, False, ncInqOut, &ncVarSubset);
    } else {
        status = prNcVarData (conn, fileName, ncid, False, ncInqOut, NULL);
    }
    printf ("}\n");
    return status;
}

int
prNcVarData (rcComm_t *conn, char *fileName, int ncid, int printAsciTime,
ncInqOut_t *ncInqOut, ncVarSubset_t *ncVarSubset)
{
    int i, j, status;
    char tempStr[NAME_LEN];
    rodsLong_t start[NC_MAX_DIMS], stride[NC_MAX_DIMS], count[NC_MAX_DIMS];
    ncGetVarOut_t *ncGetVarOut = NULL;
    void *bufPtr;

    /* data */
    printf ("data:\n\n");
    for (i = 0; i < ncInqOut->nvars; i++) {
        if (ncVarSubset->numVar > 1 || (ncVarSubset->numVar == 1 && 
          strcmp (&ncVarSubset->varName[0][LONG_NAME_LEN], "all") != 0)) {
            /* only do those that match */
            for (j = 0; j < ncVarSubset->numVar; j++) {
                if (strcmp (&ncVarSubset->varName[j][LONG_NAME_LEN], 
                  ncInqOut->var[i].name) == 0) break;
            }
            if (j >= ncVarSubset->numVar) continue;    /* no match */
        }
	printf (" %s = ", ncInqOut->var[i].name);
	if (ncInqOut->var[i].nvdims > 1) printf ("\n  ");
        status = getSingleNcVarData (conn, ncid, i, ncInqOut, ncVarSubset,
          &ncGetVarOut, start, stride, count);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "dumpNcInqOut: rcNcGetVarsByType error for %s", 
	      ncInqOut->var[i].name);
              /* don't exit yet. This could be caused by tabledap not having
               * the variable */
              printf (" ;\n");
              continue;
        } else {
	    /* print it */
	    int outCnt = 0;
	    int lastDimLen = count[ncInqOut->var[i].nvdims - 1];
	    bufPtr = ncGetVarOut->dataArray->buf;
            bzero (tempStr, sizeof (tempStr));
            if (ncInqOut->var[i].dataType == NC_CHAR) {
                int nextLastDimLen;
                if (ncInqOut->var[i].nvdims >= 2) {
                    nextLastDimLen = count[ncInqOut->var[i].nvdims - 2];
                } else {
                    nextLastDimLen = 0;
                }
	        for (j = 0; j < ncGetVarOut->dataArray->len; j+=lastDimLen) {
                    /* treat it as strings */
                    if (j + lastDimLen >= ncGetVarOut->dataArray->len - 1) {
                        printf ("%s ;\n", (char *) bufPtr);
                    } else if (outCnt >= nextLastDimLen) {
                        /* reset */
                        printf ("%s,\n  ", (char *) bufPtr);
                        outCnt = 0;
                    } else {
                        printf ("%s, ", (char *) bufPtr);
                    }
                }
            } else {
                for (j = 0; j < ncGetVarOut->dataArray->len; j++) {
                    ncValueToStr (ncInqOut->var[i].dataType, &bufPtr, tempStr);
                    if (printAsciTime == True &&
                      strcasecmp (ncInqOut->var[i].name, "time") == 0) { 
                        /* asci time */
                        time_t mytime =atoi (tempStr);
                        timeToAsci (mytime, tempStr);
#if 0
                        struct tm *mytm = gmtime (&mytime);
                        if (mytm != NULL) {
                            snprintf (tempStr, NAME_LEN,
                              "%04d-%02d-%02dT%02d:%02d:%02dZ",
                              1900+mytm->tm_year, mytm->tm_mon + 1, 
                              mytm->tm_mday,
                              mytm->tm_hour, mytm->tm_min, mytm->tm_sec);
                        }
#endif
                    }
		    outCnt++;
		    if (j >= ncGetVarOut->dataArray->len - 1) {
		        printf ("%s ;\n", tempStr);
		    } else if (outCnt >= lastDimLen) {
		        /* reset */
                        printf ("%s,\n  ", tempStr);
		        outCnt = 0;
		    } else {
                        printf ("%s, ", tempStr);
		    }
	        }
            }
	    freeNcGetVarOut (&ncGetVarOut);
	}
    }
    return 0;
}

int
getSingleNcVarData (rcComm_t *conn, int ncid, int varInx, ncInqOut_t *ncInqOut,
ncVarSubset_t *ncVarSubset, ncGetVarOut_t **ncGetVarOut, rodsLong_t *start, 
rodsLong_t *stride, rodsLong_t *count)
{
    int j, k, status;
    ncGetVarInp_t ncGetVarInp;

    for (j = 0; j < ncInqOut->var[varInx].nvdims; j++) {
        int dimId = ncInqOut->var[varInx].dimId[j];
        int doSubset = False;
        if (ncVarSubset != NULL && ncVarSubset->numSubset > 0) {
            for (k = 0; k < ncVarSubset->numSubset; k++) {
                if (strcmp (ncInqOut->dim[dimId].name,
                  ncVarSubset->ncSubset[k].subsetVarName) == 0) {
                    doSubset = True;
                    break;
                }
            }
        }
        if (doSubset == True) {
            if (ncVarSubset->ncSubset[k].start >=
              ncInqOut->dim[dimId].arrayLen ||
              ncVarSubset->ncSubset[k].end >=
              ncInqOut->dim[dimId].arrayLen ||
              ncVarSubset->ncSubset[k].start >
              ncVarSubset->ncSubset[k].end) {
                rodsLog (LOG_ERROR, 
                 "getSingleNcVarData:start %d or end %d for %s outOfRange %lld",
                 ncVarSubset->ncSubset[k].start,
                 ncVarSubset->ncSubset[k].end,
                 ncVarSubset->ncSubset[k].subsetVarName,
                 ncInqOut->dim[dimId].arrayLen);
                return NETCDF_DIM_MISMATCH_ERR;
            }
            start[j] = ncVarSubset->ncSubset[k].start;
            stride[j] = ncVarSubset->ncSubset[k].stride;
            count[j] = ncVarSubset->ncSubset[k].end -
              ncVarSubset->ncSubset[k].start + 1;
        } else {
            start[j] = 0;
            count[j] = ncInqOut->dim[dimId].arrayLen;
            stride[j] = 1;
        }
    }
    bzero (&ncGetVarInp, sizeof (ncGetVarInp));
    ncGetVarInp.dataType = ncInqOut->var[varInx].dataType;
    ncGetVarInp.ncid = ncid;
    ncGetVarInp.varid =  ncInqOut->var[varInx].id;
    ncGetVarInp.ndim =  ncInqOut->var[varInx].nvdims;
    ncGetVarInp.start = start;
    ncGetVarInp.count = count;
    ncGetVarInp.stride = stride;

    if (conn == NULL) {
        /* local call */
#ifdef NETCDF_API
        status = _rsNcGetVarsByType (ncid, &ncGetVarInp, ncGetVarOut);
#else
        status = NETCDF_BUILD_WITH_NETCDF_API_NEEDED;
#endif
    } else {
        status = rcNcGetVarsByType (conn, &ncGetVarInp, ncGetVarOut);
    }

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "getSingleNcVarData: rcNcGetVarsByType error for %s",
          ncInqOut->var[varInx].name);
    }
    return status;
}

int
getNcTypeStr (int dataType, char *outString)
{
    switch (dataType) {
	case NC_CHAR:
	    rstrcpy (outString, "char", NAME_LEN);
	    break;
	case NC_BYTE:
	    rstrcpy (outString, "byte", NAME_LEN);
	    break;
	case NC_UBYTE:
	    rstrcpy (outString, "ubyte", NAME_LEN);
	    break;
        case NC_SHORT:
            rstrcpy (outString, "short", NAME_LEN);
            break;
	case NC_STRING:
	    rstrcpy (outString, "string", NAME_LEN);
	    break;
	case NC_INT:
	    rstrcpy (outString, "int", NAME_LEN);
	    break;
	case NC_UINT:
	    rstrcpy (outString, "uint", NAME_LEN);
	    break;
	case NC_INT64:
	    rstrcpy (outString, "longlong", NAME_LEN);
	    break;
	case NC_UINT64:
	    rstrcpy (outString, "ulonglong", NAME_LEN);
	    break;
	case NC_FLOAT:
	    rstrcpy (outString, "float", NAME_LEN);
	    break;
	case NC_DOUBLE:
	    rstrcpy (outString, "double", NAME_LEN);
	    break;
      default:
        rodsLog (LOG_ERROR,
          "getNcTypeStr: Unknow dataType %d", dataType);
        return (NETCDF_INVALID_DATA_TYPE);
    }
    return 0;
}

int
ncValueToStr (int dataType, void **invalue, char *outString)
{
    void *value = *invalue;
    char **ptr = (char **) invalue;
    short myshort;

    switch (dataType) {
	case NC_CHAR:
            *outString = *(char *) value;
             *ptr+= sizeof (char);
            break;
	case NC_STRING:
	    snprintf (outString, NAME_LEN, "\"%s\"", (char *) value);
	    break;
        case NC_BYTE:
        case NC_UBYTE:
	    snprintf (outString, NAME_LEN, "%x", *(*ptr));
	    *ptr+= sizeof (char);	/* advance pointer */
	    break;
        case NC_SHORT:
            myshort = *(short int*) value;
	    snprintf (outString, NAME_LEN, "%hi", myshort);
	    *ptr+= sizeof (short);	/* advance pointer */
	    break;
        case NC_USHORT:
            myshort = *(short int*) value;
            snprintf (outString, NAME_LEN, "%hu", myshort);
            *ptr+= sizeof (short);      /* advance pointer */
            break;
	case NC_INT:
	    snprintf (outString, NAME_LEN, "%d", *(int *) value);
	    *ptr+= sizeof (int);	/* advance pointer */
	    break;
	case NC_UINT:
	    snprintf (outString, NAME_LEN, "%u", *(unsigned int *) value);
	    *ptr+= sizeof (int);	/* advance pointer */
	    break;
	case NC_INT64:
	    snprintf (outString, NAME_LEN, "%lld", *(rodsLong_t *) value);
	    *ptr+= sizeof (rodsLong_t);	/* advance pointer */
	    break;
	case NC_UINT64:
	    snprintf (outString, NAME_LEN, "%llu", *(rodsULong_t *) value);
	    *ptr+= sizeof (rodsULong_t);	/* advance pointer */
	    break;
	case NC_FLOAT:
	    snprintf (outString, NAME_LEN, "%.2f", *(float *) value);
	    *ptr+= sizeof (float);	/* advance pointer */
	    break;
	case NC_DOUBLE:
	    snprintf (outString, NAME_LEN, "%lf", *(double *) value);
	    *ptr+= sizeof (double);	/* advance pointer */
	    break;
      default:
        rodsLog (LOG_ERROR,
          "getNcTypeStr: Unknow dataType %d", dataType);
        return (NETCDF_INVALID_DATA_TYPE);
    }
    return 0;
}

#ifdef NETCDF_API
int
dumpNcInqOutToNcFile (rcComm_t *conn, int srcNcid, int noattrFlag,
ncInqOut_t *ncInqOut, char *outFileName)
{
    int i, j, dimId, status;
    int ncid, cmode;
    rodsLong_t start[NC_MAX_DIMS], stride[NC_MAX_DIMS], count[NC_MAX_DIMS];
    size_t lstart[NC_MAX_DIMS], lcount[NC_MAX_DIMS];
    ptrdiff_t lstride[NC_MAX_DIMS];
    void *bufPtr;
    int dimIdArray[NC_MAX_DIMS];

    cmode = ncFormatToCmode (ncInqOut->format);
    status = nc_create (outFileName, cmode, &ncid);
    if (status != NC_NOERR) {
        rodsLog (LOG_ERROR,
          "dumpNcInqOutToNcFile: nc_create error.  %s ", nc_strerror(status));
        status = NETCDF_CREATE_ERR - status;
        return status;
    }

    /* attrbutes */
    if (noattrFlag == False) {
        for (i = 0; i < ncInqOut->ngatts; i++) {
            bufPtr = ncInqOut->gatt[i].value.dataArray->buf;
            status = nc_put_att (ncid, NC_GLOBAL, ncInqOut->gatt[i].name,
              ncInqOut->gatt[i].dataType, ncInqOut->gatt[i].length, bufPtr);
            if (status != NC_NOERR) {
                rodsLog (LOG_ERROR,
                  "dumpNcInqOutToNcFile: nc_put_att error.  %s ", 
                  nc_strerror(status));
                status = NETCDF_PUT_ATT_ERR - status;
                closeAndRmNeFile (ncid, outFileName);
                return status;
            }
        }
    }
    /* dimensions */
    if (ncInqOut->ndims <= 0 || ncInqOut->dim == NULL)
        return USER__NULL_INPUT_ERR;
    for (i = 0; i < ncInqOut->ndims; i++) {
        if (ncInqOut->unlimdimid == ncInqOut->dim[i].id) {
            /* unlimited */
            status = nc_def_dim (ncid,  ncInqOut->dim[i].name, 
              NC_UNLIMITED, &ncInqOut->dim[i].myint);
        } else {
            status = nc_def_dim (ncid,  ncInqOut->dim[i].name, 
              ncInqOut->dim[i].arrayLen, &ncInqOut->dim[i].myint);
        }
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "dumpNcInqOutToNcFile: nc_def_dim error.  %s ", 
              nc_strerror(status));
            status = NETCDF_DEF_DIM_ERR - status;
            closeAndRmNeFile (ncid, outFileName);
            return status;
        }
    }
    /* variables */
    if (ncInqOut->nvars <= 0 || ncInqOut->var == NULL) {
        /* no variables */
        nc_close (ncid);
        return 0;
    }
    for (i = 0; i < ncInqOut->nvars; i++) {
        /* define the variables */
        for (j = 0; j < ncInqOut->var[i].nvdims;  j++) {
            dimId = ncInqOut->var[i].dimId[j];
            dimIdArray[j] = ncInqOut->dim[dimId].myint;
        }
        status = nc_def_var (ncid, ncInqOut->var[i].name, 
          ncInqOut->var[i].dataType, ncInqOut->var[i].nvdims, 
          dimIdArray, &ncInqOut->var[i].myint);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "dumpNcInqOutToNcFile: nc_def_var for %s error.  %s ",
              ncInqOut->var[i].name, nc_strerror(status));
            status = NETCDF_DEF_VAR_ERR - status;
            closeAndRmNeFile (ncid, outFileName);
            return status;
        }
        /* put the variable attributes */
        if (noattrFlag == False) {
            for (j = 0; j < ncInqOut->var[i].natts; j++) {
                ncGenAttOut_t *att = &ncInqOut->var[i].att[j];
                bufPtr = att->value.dataArray->buf;
                status = nc_put_att (ncid, ncInqOut->var[i].myint, att->name,
                  att->dataType, att->length, bufPtr);
                if (status != NC_NOERR) {
                    rodsLog (LOG_ERROR,
                      "dumpNcInqOutToNcFile: nc_put_att for %s error.  %s ",
                      ncInqOut->var[i].name, nc_strerror(status));
                    status = NETCDF_PUT_ATT_ERR - status;
                    closeAndRmNeFile (ncid, outFileName);
                    return status;
                }
            }
        }
    }
    nc_enddef (ncid);
    for (i = 0; i < ncInqOut->nvars; i++) {
        ncGenVarOut_t *var = &ncInqOut->var[i];
        for (j = 0; j < var->nvdims; j++) {
            dimId = var->dimId[j];
            start[j] = 0;
            lstart[j] = 0;
            count[j] = ncInqOut->dim[dimId].arrayLen;
            lcount[j] = ncInqOut->dim[dimId].arrayLen;
            stride[j] = 1;
            lstride[j] = 1;
        }
        status = getAndPutVarToFile (conn, srcNcid, var->id, var->nvdims,
          var->dataType, lstart, lstride, lcount, ncid, var->myint);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "dumpNcInqOutToNcFile: getAndPutVarToFile error for %s",
              ncInqOut->var[i].name);
            closeAndRmNeFile (ncid, outFileName);
            return status;
        }
#if 0
        bzero (&ncGetVarInp, sizeof (ncGetVarInp));
        ncGetVarInp.dataType = var->dataType;
        ncGetVarInp.ncid = srcNcid;
        ncGetVarInp.varid =  var->id;
        ncGetVarInp.ndim =  var->nvdims;
        ncGetVarInp.start = start;
        ncGetVarInp.count = count;
        ncGetVarInp.stride = stride;

        if (conn == NULL) {
            /* local call */
            status = _rsNcGetVarsByType (srcNcid, &ncGetVarInp, &ncGetVarOut);
        } else {
            status = rcNcGetVarsByType (conn, &ncGetVarInp, &ncGetVarOut);
        }
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "dumpNcInqOutToNcFile: rcNcGetVarsByType error for %s",
              ncInqOut->var[i].name);
            closeAndRmNeFile (ncid, outFileName);
            return status;
        }
        status = nc_put_vars (ncid, var->myint, lstart, 
         lcount, lstride, ncGetVarOut->dataArray->buf);
        freeNcGetVarOut (&ncGetVarOut);
        if (status != NC_NOERR) {
            rodsLogError (LOG_ERROR, status,
              "dumpNcInqOutToNcFile: nc_put_vars error for %s    %s",
              ncInqOut->var[i].name, nc_strerror(status));
            closeAndRmNeFile (ncid, outFileName);
            return NETCDF_PUT_VARS_ERR;
        }
#endif
    }
    nc_close (ncid);
    return 0;
}

int
getAndPutVarToFile (rcComm_t *conn, int srcNcid, int srcVarid, int ndim, 
int dataType, size_t *lstart, ptrdiff_t *lstride, size_t *lcount, 
int ncid, int varid)
{
    int i, status;
    rodsLong_t arrayLen = 1;
    size_t dimStep = lcount[0];		/* use dim 0 - time for time series */
    size_t curCount = 0;
    size_t mystart[NC_MAX_DIMS], mycount[NC_MAX_DIMS];
    rodsLong_t start[NC_MAX_DIMS], stride[NC_MAX_DIMS], count[NC_MAX_DIMS];
    ptrdiff_t mystride[NC_MAX_DIMS];
    ncGetVarInp_t ncGetVarInp;
    ncGetVarOut_t *ncGetVarOut = NULL;

    for (i = 0; i < ndim; i++) {
        arrayLen = arrayLen * ((lcount[i] - 1) / lstride[i] + 1);
        mystart[i] = lstart[i];
        mycount[i] = lcount[i];
        mystride[i] = lstride[i];
        start[i] = lstart[i];
        count[i] = lcount[i];
        stride[i] = lstride[i];
        
    }

    if (arrayLen > NC_VAR_TRANS_SZ) {
        int stepSize = arrayLen / dimStep;
        dimStep = NC_VAR_TRANS_SZ / stepSize + 1;
    }
    
    bzero (&ncGetVarInp, sizeof (ncGetVarInp));
    ncGetVarInp.dataType = dataType;
    ncGetVarInp.ncid = srcNcid;
    ncGetVarInp.varid =  srcVarid;
    ncGetVarInp.ndim =  ndim;
    ncGetVarInp.start = start;
    ncGetVarInp.count = count;
    ncGetVarInp.stride = stride;

    while (curCount < lcount[0]) {
        if (curCount + dimStep > lcount[0]) {
            mycount[0] = lcount[0] - curCount;
            count[0] = lcount[0] - curCount;
        } else {
            mycount[0] = dimStep;
            count[0] = dimStep;
        }
        if (conn == NULL) {
            /* local call */
            status = _rsNcGetVarsByType (srcNcid, &ncGetVarInp, &ncGetVarOut);
        } else {
            status = rcNcGetVarsByType (conn, &ncGetVarInp, &ncGetVarOut);
        }
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "getAndPutVarToFile: rcNcGetVarsByType error for varid %d",
              srcVarid);
            return status;
        }
        status = nc_put_vars (ncid, varid, mystart, mycount, mystride, 
          ncGetVarOut->dataArray->buf);
        freeNcGetVarOut (&ncGetVarOut);
        if (status != NC_NOERR) {
            rodsLogError (LOG_ERROR, status,
              "getAndPutVarToFile: nc_put_vars error for varid %d    %s",
              varid, nc_strerror(status));
            return NETCDF_PUT_VARS_ERR;
        }
        curCount += mycount[0];
        mystart[0] += mycount[0];
        start[0] = mystart[0];
    } 
    return 0;
}

int
dumpSubsetToFile (rcComm_t *conn, int srcNcid, int noattrFlag,
ncInqOut_t *ncInqOut, ncVarSubset_t *ncVarSubset, char *outFileName)
{
    int i, j, dimId, nvars, status;
    int ncid, cmode;
    rodsLong_t start[NC_MAX_DIMS], stride[NC_MAX_DIMS], count[NC_MAX_DIMS];
    size_t lstart[NC_MAX_DIMS], lcount[NC_MAX_DIMS];
    ptrdiff_t lstride[NC_MAX_DIMS];
    ncGetVarOut_t *ncGetVarOut = NULL;
    void *bufPtr;
    int dimIdArray[NC_MAX_DIMS];
    ncInqOut_t subsetNcInqOut;

    cmode = ncFormatToCmode (ncInqOut->format);
    status = nc_create (outFileName, cmode, &ncid);
    if (status != NC_NOERR) {
        rodsLog (LOG_ERROR,
          "dumpSubsetToFile: nc_create error.  %s ", nc_strerror(status));
        status = NETCDF_CREATE_ERR + status;
        return status;
    }
    /* attrbutes */
    if (noattrFlag == False) {
        for (i = 0; i < ncInqOut->ngatts; i++) {
            bufPtr = ncInqOut->gatt[i].value.dataArray->buf;
            status = nc_put_att (ncid, NC_GLOBAL, ncInqOut->gatt[i].name,
              ncInqOut->gatt[i].dataType, ncInqOut->gatt[i].length, bufPtr);
            if (status != NC_NOERR) {
                rodsLog (LOG_ERROR,
                  "dumpSubsetToFile: nc_put_att error.  %s ",
                  nc_strerror(status));
                status = NETCDF_PUT_ATT_ERR - status;
                closeAndRmNeFile (ncid, outFileName);
                return status;
            }
        }
    }
    /* dimensions */
    if (ncInqOut->ndims <= 0 || ncInqOut->dim == NULL)
        return USER__NULL_INPUT_ERR;
    bzero (&subsetNcInqOut, sizeof (subsetNcInqOut));
    subsetNcInqOut.ndims = ncInqOut->ndims;
    subsetNcInqOut.format = ncInqOut->format;
    subsetNcInqOut.unlimdimid = -1;
    subsetNcInqOut.dim = (ncGenDimOut_t *)
      calloc (ncInqOut->ndims, sizeof (ncGenDimOut_t));
    for (i = 0; i < ncInqOut->ndims; i++) {
         rstrcpy (subsetNcInqOut.dim[i].name, ncInqOut->dim[i].name,
           LONG_NAME_LEN);
        /* have to use the full arrayLen instead of subsetted length
         * because it will be used in subsetting later */
        subsetNcInqOut.dim[i].arrayLen = ncInqOut->dim[i].arrayLen;
        if (ncInqOut->unlimdimid == ncInqOut->dim[i].id) {
            /* unlimited */
            status = nc_def_dim (ncid,  ncInqOut->dim[i].name,
              NC_UNLIMITED, &subsetNcInqOut.dim[i].id);
            subsetNcInqOut.unlimdimid = subsetNcInqOut.dim[i].id;
        } else {
            int arrayLen;
            for (j = 0; j < ncVarSubset->numSubset; j++) {
                if (strcmp (ncInqOut->dim[i].name,
                  ncVarSubset->ncSubset[j].subsetVarName) == 0) {
                    arrayLen = (ncVarSubset->ncSubset[j].end -
                     ncVarSubset->ncSubset[j].start) /
                     ncVarSubset->ncSubset[j].stride + 1;
                    break;
                }
            }
            if (j >= ncVarSubset->numSubset) 	/* no match */
                arrayLen = ncInqOut->dim[i].arrayLen;
            status = nc_def_dim (ncid,  ncInqOut->dim[i].name,
              arrayLen, &subsetNcInqOut.dim[i].id);
        }
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "dumpSubsetToFile: nc_def_dim error.  %s ",
              nc_strerror(status));
            status = NETCDF_DEF_DIM_ERR - status;
            closeAndRmNeFile (ncid, outFileName);
            return status;
        }
    }
    /* variables */
    if (ncInqOut->nvars <= 0 || ncInqOut->var == NULL) {
        /* no variables */
        nc_close (ncid);
        return 0;
    }
    /* screen the variables */
    subsetNcInqOut.var = (ncGenVarOut_t *)
      calloc (ncInqOut->nvars, sizeof (ncGenVarOut_t));
    nvars = 0;
    /* For subsequent subsetting and writing vars to a netcdf file,
     * subsetNcInqOut.var[i].id contains the var id of the source and
     * subsetNcInqOut.var[i].myint contains the var id of the target 
     */
    for (i = 0; i < ncInqOut->nvars; i++) {
        if (ncVarSubset->numVar == 0 || 
          strcmp ((char *)ncVarSubset->varName, "all") == 0) {
            /* do all var */
            subsetNcInqOut.var[subsetNcInqOut.nvars] = ncInqOut->var[i];
            subsetNcInqOut.nvars++;
            continue;
        }  
        for (j = 0; j < ncVarSubset->numVar; j++) {
            if (strcmp (&ncVarSubset->varName[j][LONG_NAME_LEN],
              ncInqOut->var[i].name) == 0) {
                subsetNcInqOut.var[subsetNcInqOut.nvars] = 
                  ncInqOut->var[i];
                subsetNcInqOut.nvars++;
                break;
            }
        }
    }

    for (i = 0; i < subsetNcInqOut.nvars; i++) {
        /* define the variables */
        for (j = 0; j < subsetNcInqOut.var[i].nvdims;  j++) {
            dimId = subsetNcInqOut.var[i].dimId[j];
            dimIdArray[j] = subsetNcInqOut.dim[dimId].id;
        }
        status = nc_def_var (ncid, subsetNcInqOut.var[i].name,
          subsetNcInqOut.var[i].dataType, subsetNcInqOut.var[i].nvdims,
          dimIdArray, &subsetNcInqOut.var[i].myint);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "dumpSubsetToFile: nc_def_var for %s error.  %s ",
              subsetNcInqOut.var[i].name, nc_strerror(status));
            status = NETCDF_DEF_VAR_ERR - status;
            closeAndRmNeFile (ncid, outFileName);
            return status;
        }
        /* put the variable attributes */
        if (noattrFlag == False) {
            for (j = 0; j < subsetNcInqOut.var[i].natts; j++) {
                ncGenAttOut_t *att = &subsetNcInqOut.var[i].att[j];
                bufPtr = att->value.dataArray->buf;
                status = nc_put_att (ncid, subsetNcInqOut.var[i].myint, att->name,
                  att->dataType, att->length, bufPtr);
                if (status != NC_NOERR) {
                    rodsLog (LOG_ERROR,
                      "dumpSubsetToFile: nc_put_att for %s error.  %s ",
                      subsetNcInqOut.var[i].name, nc_strerror(status));
                    status = NETCDF_PUT_ATT_ERR - status;
                    closeAndRmNeFile (ncid, outFileName);
                    return status;
                }
            }
        }
    }
    nc_enddef (ncid);

    for (i = 0; i < subsetNcInqOut.nvars; i++) {
        status = getSingleNcVarData (conn, srcNcid, i, &subsetNcInqOut,
          ncVarSubset, &ncGetVarOut, start, stride, count);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "dumpSubsetToFile: rcNcGetVarsByType error for %s",
              subsetNcInqOut.var[i].name);
            closeAndRmNeFile (ncid, outFileName);
            return status;
        }
        for (j = 0; j < subsetNcInqOut.var[i].nvdims; j++) {
            lstart[j] = (size_t) 0;
            lstride[j] = (ptrdiff_t) 1;
            lcount[j] = (size_t) count[j];
        }
        status = nc_put_vars (ncid, subsetNcInqOut.var[i].myint, lstart,
         lcount, lstride, ncGetVarOut->dataArray->buf);
        freeNcGetVarOut (&ncGetVarOut);
        if (status != NC_NOERR) {
            rodsLogError (LOG_ERROR, status,
              "dumpSubsetToFile: nc_put_vars error for %s    %s",
              subsetNcInqOut.var[i].name, nc_strerror(status));
            closeAndRmNeFile (ncid, outFileName);
            return NETCDF_PUT_VARS_ERR;
        }
    }
    if (subsetNcInqOut.dim != NULL) free (subsetNcInqOut.dim);
    if (subsetNcInqOut.var != NULL) free (subsetNcInqOut.var);
    nc_close (ncid);
    return 0;
}
#endif

int
ncFormatToCmode (int format)
{
    int cmode;

    switch (format) {
      case NC_FORMAT_CLASSIC:
#ifdef NETCDF4_API	/* force it to NC_NETCDF4 */
        cmode = NC_NETCDF4;
#else
        cmode = NC_CLASSIC_MODEL;
#endif
        break;
      case NC_FORMAT_64BIT:
        cmode = NC_64BIT_OFFSET;
        break;
      case NC_FORMAT_NETCDF4: 
        cmode = NC_NETCDF4;
        break;
      case NC_FORMAT_NETCDF4_CLASSIC:
        cmode = NC_NETCDF4|NC_CLASSIC_MODEL;
        break;
      default:
        rodsLog (LOG_ERROR,
          "ncFormatToCmode: Unknow format %d, use NC_CLASSIC_MODEL", format);
        cmode = NC_CLASSIC_MODEL;
    }
    return cmode;
}

#ifdef NETCDF_API
int
closeAndRmNeFile (int ncid, char *outFileName)
{
    nc_close (ncid);
    unlink (outFileName);
    return 0;
}
#endif

int
printNice (char *str, char *margin, int charPerLine)
{
    char tmpStr[META_STR_LEN];
    char *tmpPtr = tmpStr;
    int len = strlen (str);
    int c;

    if (len > META_STR_LEN) return USER_STRLEN_TOOLONG;

    rstrcpy (tmpStr, str, META_STR_LEN);
    while (len > 0) {
        if (len > charPerLine) {
            char *tmpPtr1 = tmpPtr;
            c = *(tmpPtr + charPerLine);
            *(tmpPtr + charPerLine) = '\0';	
            /* take out any \n */
            while (*tmpPtr1 != '\0') {
                /* if (*tmpPtr1 == '\n') {  */
                if (isspace (*tmpPtr1)) {
                    *tmpPtr1 = ' ';
                }
                tmpPtr1++;
            }
            printf ("%s%s\n", margin, tmpPtr);
            *(tmpPtr + charPerLine) = c;
            tmpPtr += charPerLine;
            len -= charPerLine;
        } else {
            printf ("%s%s", margin, tmpPtr);
            break;
        }
    }
    return 0;
}

/* parseNcVarSubset - ncVarSubset->subsetVarName is expected to be in the
 * form varName[start:stride:end]. Parse for integer value of start, stride
 * and end.
 */
int
parseNcSubset (ncSubset_t *ncSubset)
{
    char *endPtr, *tmpPtr1, *tmpPtr2;

    if ((endPtr = strchr (ncSubset->subsetVarName, '[')) == NULL) {
        rodsLog (LOG_ERROR,
          "parseNcSubset: subset input %s format error", 
          ncSubset->subsetVarName);
        return USER_INPUT_FORMAT_ERR;
    }
 
    tmpPtr1 = endPtr + 1;
    if ((tmpPtr2 = strchr (tmpPtr1, '%')) == NULL || !isdigit (*tmpPtr1)) {
        rodsLog (LOG_ERROR,
          "parseNcSubset: subset input %s format error", 
          ncSubset->subsetVarName);
        return USER_INPUT_FORMAT_ERR;
    }
    *tmpPtr2 = '\0';
    ncSubset->start = atoi (tmpPtr1);
    rstrcpy (ncSubset->startStr, tmpPtr1, NAME_LEN);
    *tmpPtr2 = '%';
    tmpPtr1 = tmpPtr2 + 1;
    if ((tmpPtr2 = strchr (tmpPtr1, '%')) == NULL || !isdigit (*tmpPtr1)) {
        rodsLog (LOG_ERROR,
          "parseNcSubset: subset input %s format error",   
          ncSubset->subsetVarName);
        return USER_INPUT_FORMAT_ERR;
    }
    *tmpPtr2 = '\0';
    ncSubset->stride = atoi (tmpPtr1);
    *tmpPtr2 = '%';
    tmpPtr1 = tmpPtr2 + 1;
    if ((tmpPtr2 = strchr (tmpPtr1, ']')) == NULL || !isdigit (*tmpPtr1)) {
        rodsLog (LOG_ERROR,
          "parseNcSubset: subset input %s format error",
          ncSubset->subsetVarName);
        return USER_INPUT_FORMAT_ERR;
    }
    *tmpPtr2 = '\0';
    ncSubset->end = atoi (tmpPtr1);
    rstrcpy (ncSubset->endStr, tmpPtr1, NAME_LEN);
    *endPtr = '\0'; 	/* truncate the name */
    return 0;
}

int
parseVarStrForSubset (char *varStr, ncVarSubset_t *ncVarSubset)
{
    int i = 0;
    int inLen = strlen (varStr);
    char *inPtr = varStr;

    while (getNextEleInStr (&inPtr, &ncVarSubset->varName[i][LONG_NAME_LEN],
      &inLen, LONG_NAME_LEN) > 0) {
        ncVarSubset->numVar++;
        i++;
        if (ncVarSubset->numVar >= MAX_NUM_VAR) break;
    }
    return 0;
}

int
parseSubsetStr (char *subsetStr, ncVarSubset_t *ncVarSubset)
{
    int status;
    int i = 0;
    int inLen = strlen (subsetStr);
    char *inPtr = subsetStr;

    while (getNextEleInStr (&inPtr,
     ncVarSubset->ncSubset[i].subsetVarName,
     &inLen, LONG_NAME_LEN) > 0) {
        status = parseNcSubset (&ncVarSubset->ncSubset[i]);
        if (status < 0) return status;
        ncVarSubset->numSubset++;
        i++;
        if (ncVarSubset->numSubset >= MAX_NUM_VAR) break;
    }
    return 0;
}

int
timeToAsci (time_t mytime, char *asciTime)
{
    struct tm *mytm = localtime (&mytime);
    if (mytm != NULL) {
        snprintf (asciTime, NAME_LEN,
          "%04d-%02d-%02dT%02d:%02d:%02d",
           1900+mytm->tm_year, mytm->tm_mon + 1, mytm->tm_mday,
           mytm->tm_hour, mytm->tm_min, mytm->tm_sec);
    } else {
        *asciTime = '\0';
    }
    return 0;
}

int
asciToTime (char *asciTime, time_t *mytime)
{
    struct tm mytm, *tmptm;
    int status;
    time_t thistm = time (0);

    if (strchr (asciTime, 'T') == NULL) {
        /* assume it is UTC time */
        *mytime = atoi (asciTime);
        return 0;
    }
    bzero (&mytm, sizeof (mytm));
    status = sscanf (asciTime, "%04d-%02d-%02dT%02d:%02d:%02d",
      &mytm.tm_year, &mytm.tm_mon, &mytm.tm_mday, 
      &mytm.tm_hour, &mytm.tm_min, &mytm.tm_sec);

    if (status != 6) {
        rodsLog (LOG_ERROR,
          "asciToTime: Time format error for %s, must be like %s", asciTime,
          "1970-01-01T03:21:48");
        return USER_INPUT_FORMAT_ERR;
    }
    /* get the tm_isdst of local time */
    tmptm = localtime (&thistm);

    mytm.tm_year -= 1900;
    mytm.tm_mon -= 1;
    mytm.tm_isdst = tmptm->tm_isdst;

    *mytime = mktime (&mytm);

    return 0;
}
        
int
resolveSubsetVar (rcComm_t *conn, int ncid, ncInqOut_t *ncInqOut,
ncVarSubset_t *ncVarSubset)
{
    int i, j, k, status;
    rodsLong_t start[NC_MAX_DIMS], stride[NC_MAX_DIMS], count[NC_MAX_DIMS];
    ncGetVarOut_t *ncGetVarOut = NULL;
    char *bufPtr;
    rodsLong_t longStart, longEnd, mylong;
    double doubleStart, doubleEnd, mydouble;
    time_t startTime, endTime;
    int isInt;

    for (j = 0; j < ncVarSubset->numSubset; j++) {
        for (i = 0; i < ncInqOut->nvars; i++) {
            if (strcmp (ncVarSubset->ncSubset[j].subsetVarName,
                  ncInqOut->var[i].name) == 0) break;
        }
        if (i >= ncInqOut->nvars) {
            rodsLog (LOG_ERROR,
              "resolveSubsetVar: unmatch subset dim %s",
              ncVarSubset->ncSubset[j].subsetVarName);
            return NETCDF_DIM_MISMATCH_ERR;
        }
        if (strcasecmp (ncInqOut->var[i].name, "time") == 0) {
            asciToTime (ncVarSubset->ncSubset[j].startStr, &startTime);
            asciToTime (ncVarSubset->ncSubset[j].endStr, &endTime);
        }
        switch (ncInqOut->var[i].dataType) {
          case NC_SHORT:
          case NC_USHORT:
          case NC_INT:
          case NC_UINT:
          case NC_INT64:
          case NC_UINT64:
            isInt = True;
            if (strcasecmp (ncInqOut->var[i].name, "time") == 0) {
                longStart = startTime;
                longEnd = endTime;
            } else {
                longStart = atoll (ncVarSubset->ncSubset[j].startStr);
                longEnd = atoll (ncVarSubset->ncSubset[j].endStr);
            }
            break;
          case NC_FLOAT:
          case NC_DOUBLE:
            isInt = False;
            if (strcasecmp (ncInqOut->var[i].name, "time") == 0) {
                doubleStart = startTime;
                doubleEnd = endTime;
            } else {
                doubleStart = atof (ncVarSubset->ncSubset[j].startStr);
                doubleEnd = atof (ncVarSubset->ncSubset[j].endStr);
            }
            break;
          default:
            rodsLog (LOG_ERROR,
              "resolveSubsetVar: Unknow dim dataType %d",
              ncInqOut->var[i].dataType);
            return (NETCDF_INVALID_DATA_TYPE);
        }
        status = getSingleNcVarData (conn, ncid, i, ncInqOut, NULL,
          &ncGetVarOut, start, stride, count);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "resolveSubsetVar: rcNcGetVarsByType error for %s",
              ncInqOut->var[i].name);
            return status;
        }
        bufPtr = (char *) ncGetVarOut->dataArray->buf;
        ncVarSubset->ncSubset[j].start = ncVarSubset->ncSubset[j].end = -1;
        for (k = 0; k < ncGetVarOut->dataArray->len; k++) {
            switch (ncInqOut->var[i].dataType) {
              case NC_SHORT:
                mylong = *((short *) bufPtr);
                bufPtr += sizeof (short);
                break;
              case NC_USHORT:
                mylong = *((unsigned short *) bufPtr);
                bufPtr += sizeof (short);
                break;
              case NC_INT:
                mylong = *((int *) bufPtr);
                bufPtr += sizeof (int);
                break;
              case NC_UINT:
                mylong = *((unsigned int *) bufPtr); 
                bufPtr += sizeof (int);
                break;
              case NC_INT64:
                mylong = *((rodsLong_t *) bufPtr);
                bufPtr += sizeof (rodsLong_t);
                break;
              case NC_UINT64:
                mylong = *((rodsULong_t *) bufPtr);
                bufPtr += sizeof (rodsLong_t);
                break;
              case NC_FLOAT:
                mydouble = *((float *) bufPtr);
                bufPtr += sizeof (float);
                break;
              case NC_DOUBLE:
                mydouble = *((double *) bufPtr);
                bufPtr += sizeof (double);
                break;
              default:
                rodsLog (LOG_ERROR,
                  "resolveSubsetVar: Unknow dim dataType %d", 
                  ncInqOut->var[i].dataType);
                return (NETCDF_INVALID_DATA_TYPE);
            }
            if (ncVarSubset->ncSubset[j].start == -1) {
                if (isInt == True) {
                    if (mylong >= longStart) 
                      ncVarSubset->ncSubset[j].start = k;
                } else {
                    if (mydouble >= doubleStart) 
                      ncVarSubset->ncSubset[j].start = k;
                }
            }
            if (isInt == True) {
                if (mylong <= longEnd) {
                    ncVarSubset->ncSubset[j].end = k;
                } else {
                    break;
                }
            } else {
                if (mydouble <= doubleEnd) {
                  ncVarSubset->ncSubset[j].end = k;
                } else {
                    break;
                }
            }
        }
        freeNcGetVarOut (&ncGetVarOut);
    }
    return 0;
}
#ifdef NETCDF_API
int
ncInq (ncInqInp_t *ncInqInp, ncInqOut_t **ncInqOut)
{
    int ndims, nvars, ngatts, unlimdimid, format;
    int dimType, attType, varType, allFlag;
    int status, i;
    ncGenDimOut_t *dim;
    ncGenVarOut_t *var;
    ncGenAttOut_t *gatt;
    size_t mylong = 0;
    int intArray[NC_MAX_VAR_DIMS];
    int ncid = ncInqInp->ncid;

    *ncInqOut = NULL;

    status = nc_inq (ncid, &ndims, &nvars, &ngatts, &unlimdimid);
    if (status != NC_NOERR) {
        rodsLog (LOG_ERROR,
          "_rsNcInq: nc_inq error.  %s ", nc_strerror(status));
        status = NETCDF_INQ_ERR + status;
        return status;
    }

    if (ncInqInp->paramType == 0) ncInqInp->paramType = NC_ALL_TYPE;
    if ((ncInqInp->paramType & NC_DIM_TYPE) == 0) {
        dimType = ndims = 0;
    } else {
        dimType = 1;
    }
    if ((ncInqInp->paramType & NC_ATT_TYPE) == 0) {
        attType = ngatts = 0;
    } else {
        attType = 1;
    }
    if ((ncInqInp->paramType & NC_VAR_TYPE) == 0) {
        varType = nvars = 0;
    } else {
        varType = 1;
    }

    if ((varType + attType + dimType) > 1) {
        /* inq more than 1 type, ignore name and myid and inq all items of each
         * type */
        allFlag = NC_ALL_FLAG;
    } else {
        allFlag = ncInqInp->flags & NC_ALL_FLAG;
    }

    if (allFlag == 0) {
        /* indiviudal query. get only a single result */
        if (ndims > 0) ndims = 1;
        else if (ngatts > 0) ngatts = 1;
        else if (nvars > 0) nvars = 1;
    }

    status = nc_inq_format (ncid, &format);
    if (status != NC_NOERR) {
        rodsLog (LOG_ERROR,
          "_rsNcInq: nc_inq_format error.  %s ", nc_strerror(status));
        status = NETCDF_INQ_FORMAT_ERR + status;
        return status;
    }
    initNcInqOut (ndims, nvars, ngatts, unlimdimid, format, ncInqOut);

    /* inq dimension */
    dim = (*ncInqOut)->dim;
    for (i = 0; i < ndims; i++) {
        if (allFlag != 0) {
            /* inq all dim */
            dim[i].id = i;
            status = nc_inq_dim (ncid, i, dim[i].name, &mylong);
        } else {
            if (*ncInqInp->name != '\0') {
                /* inq by name */
                status = nc_inq_dimid (ncid, ncInqInp->name, &dim[i].id);
                if (status != NC_NOERR) {
                    rodsLog (LOG_ERROR,
                      "_rsNcInq: nc_inq_dimid error for %s.  %s ",
                      ncInqInp->name, nc_strerror(status));
                    status = NETCDF_INQ_ID_ERR + status;
                    freeNcInqOut (ncInqOut);
                    return status;
                }
            } else {
                dim[i].id = ncInqInp->myid;
            }
            status =  nc_inq_dim (ncid, dim[i].id, dim[i].name, &mylong);
        }
        if (status == NC_NOERR) {
             dim[i].arrayLen = mylong;
        } else {
            rodsLog (LOG_ERROR,
              "_rsNcInq: nc_inq_dim error.  %s ", nc_strerror(status));
            status = NETCDF_INQ_DIM_ERR + status;
            freeNcInqOut (ncInqOut);
            return status;
        }
    }

    /* inq variables */
    var = (*ncInqOut)->var;
    for (i = 0; i < nvars; i++) {
        if (allFlag != 0) {
            var[i].id = i;
        } else {
            if (*ncInqInp->name != '\0') {
                /* inq by name */
                status = nc_inq_varid (ncid, ncInqInp->name, &var[i].id);
                if (status != NC_NOERR) {
                    rodsLog (LOG_ERROR,
                      "_rsNcInq: nc_inq_varid error for %s.  %s ",
                      ncInqInp->name, nc_strerror(status));
                    status = NETCDF_INQ_ID_ERR + status;
                    freeNcInqOut (ncInqOut);
                    return status;
                }
            } else {
                var[i].id = ncInqInp->myid;
            }
        }
        status = nc_inq_var (ncid, var[i].id, var[i].name, &var[i].dataType,
          &var[i].nvdims, intArray, &var[i].natts);
        if (status == NC_NOERR) {
            /* fill in att */
            if (var[i].natts > 0) {
                var[i].att = (ncGenAttOut_t *)
                  calloc (var[i].natts, sizeof (ncGenAttOut_t));
                status = inqAtt (ncid, i, var[i].natts, NULL, 0, NC_ALL_FLAG,
                  var[i].att);
                if (status < 0) {
                    freeNcInqOut (ncInqOut);
                    return status;
                }
            }
            /* fill in dimId */
            if (var[i].nvdims > 0) {
                int j;
                var[i].dimId = (int *) calloc (var[i].nvdims, sizeof (int));
                for (j = 0; j < var[i].nvdims; j++) {
                    var[i].dimId[j] = intArray[j];
                }
            }
        } else {
            rodsLog (LOG_ERROR,
              "_rsNcInq: nc_inq_var error.  %s ", nc_strerror(status));
            status = NETCDF_INQ_VARS_ERR + status;
            freeNcInqOut (ncInqOut);
            return status;
        }
    }

    /* inq attributes */
    gatt = (*ncInqOut)->gatt;
    status = inqAtt (ncid, NC_GLOBAL, ngatts, ncInqInp->name, ncInqInp->myid,
      allFlag, gatt);

    return status;
}
 
int
inqAtt (int ncid, int varid, int natt, char *name, int id, int allFlag,
ncGenAttOut_t *attOut)
{
    int status, i;
    nc_type dataType;
    size_t length;


    if (natt <= 0) return 0;

    if (attOut == NULL) return USER__NULL_INPUT_ERR;

    for (i = 0; i < natt; i++) {
        if (allFlag != 0) {
            attOut[i].id = i;
            status = nc_inq_attname (ncid, varid, i, attOut[i].name);
        } else {
            if (*name != '\0') {
                /* inq by name */
                rstrcpy (attOut[i].name, name,  NAME_LEN);
                status = NC_NOERR;
            } else {
                /* inq by id */
                attOut[i].id = id;
                status = nc_inq_attname (ncid, varid, id, attOut[i].name);
            }  
        }
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "inqAtt: nc_inq_attname error for ncid %d, varid %d, %s",
              ncid, varid, nc_strerror(status));
            status = NETCDF_INQ_ATT_ERR + status;
            free (attOut);
            return status;
        }
        status = nc_inq_att (ncid, varid, attOut[i].name, &dataType, &length);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "inqAtt: nc_inq_att error for ncid %d, varid %d, %s",
              ncid, varid, nc_strerror(status));
            status = NETCDF_INQ_ATT_ERR + status;
            free (attOut);
            return status;
        }  
        status = getAttValue (ncid, varid, attOut[i].name, dataType, length,
          &attOut[i].value);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "inqAtt: getAttValue error for ncid %d, varid %d", ncid, varid);
            free (attOut);
            return status;
        } 
        attOut[i].dataType = dataType;
        attOut[i].length = length;
        attOut[i].id = i;
    }

    return 0;
}

int
getAttValue (int ncid, int varid, char *name, int dataType, int length,
ncGetVarOut_t *value)
{
    int status;

    (value)->dataArray = (dataArray_t *) calloc (1, sizeof (dataArray_t));
    (value)->dataArray->len = length;
    (value)->dataArray->type = dataType;
    switch (dataType) {
      case NC_CHAR:
        value->dataArray->buf = calloc (length + 1, sizeof (char));
        rstrcpy (value->dataType_PI, "charDataArray_PI", NAME_LEN);
        status = nc_get_att_text (ncid, varid, name,
          (char *) (value)->dataArray->buf);
        (value)->dataArray->len = length + 1;
        break;
      case NC_BYTE:
      case NC_UBYTE:
        value->dataArray->buf = calloc (length, sizeof (char));
        rstrcpy (value->dataType_PI, "charDataArray_PI", NAME_LEN);
        status = nc_get_att_uchar (ncid, varid, name,
          (unsigned char *) (value)->dataArray->buf);
        break;
      case NC_SHORT:
        /* use int because we can't pack short yet. */
        (value)->dataArray->buf = calloc (length, sizeof (short));
        rstrcpy ((value)->dataType_PI, "int16DataArray_PI", NAME_LEN);
        status = nc_get_att_short (ncid, varid, name,
          (short *) (value)->dataArray->buf);
        break;
      case NC_USHORT:
        /* use uint because we can't pack short yet. */
        (value)->dataArray->buf = calloc (length, sizeof (short));
        rstrcpy ((value)->dataType_PI, "int16DataArray_PI", NAME_LEN);
        status = nc_get_att_ushort (ncid, varid, name,
          (unsigned short *) (value)->dataArray->buf);
        break;
      case NC_STRING:
        (value)->dataArray->buf = calloc (length + 1, sizeof (char *));
        rstrcpy ((value)->dataType_PI, "strDataArray_PI", NAME_LEN);
        status = nc_get_att_string (ncid, varid, name,
          (char **) (value)->dataArray->buf);
        break;
      case NC_INT:
       (value)->dataArray->buf = calloc (length, sizeof (int));
        rstrcpy ((value)->dataType_PI, "intDataArray_PI", NAME_LEN);
        status = nc_get_att_int (ncid, varid, name,
          (int *) (value)->dataArray->buf);
        break;
      case NC_UINT:
       (value)->dataArray->buf = calloc (length, sizeof (unsigned int));
        rstrcpy ((value)->dataType_PI, "intDataArray_PI", NAME_LEN);
        status = nc_get_att_uint (ncid, varid, name,
          (unsigned int *) (value)->dataArray->buf);
        break;
      case NC_INT64:
        (value)->dataArray->buf = calloc (length, sizeof (long long));
        rstrcpy ((value)->dataType_PI, "int64DataArray_PI", NAME_LEN);
        status = nc_get_att_longlong (ncid, varid, name,
          (long long *) (value)->dataArray->buf);
        break;
      case NC_UINT64:
        (value)->dataArray->buf = calloc (length, sizeof (unsigned long long));
        rstrcpy ((value)->dataType_PI, "int64DataArray_PI", NAME_LEN);
        status = nc_get_att_ulonglong (ncid, varid, name,
          (unsigned long long *) (value)->dataArray->buf);
        break;
      case NC_FLOAT:
        (value)->dataArray->buf = calloc (length, sizeof (float));
        rstrcpy ((value)->dataType_PI, "intDataArray_PI", NAME_LEN);
        status = nc_get_att_float (ncid, varid, name,
          (float *) (value)->dataArray->buf);
        break;
      case NC_DOUBLE:
        (value)->dataArray->buf = calloc (length, sizeof (double));
        rstrcpy ((value)->dataType_PI, "int64DataArray_PI", NAME_LEN);
        status = nc_get_att_double (ncid, varid, name,
          (double *) (value)->dataArray->buf);
        break;
      default:
        rodsLog (LOG_ERROR,
          "getAttValue: Unknow dataType %d", dataType);
        return (NETCDF_INVALID_DATA_TYPE);
    }

    if (status != NC_NOERR) {
        clearNcGetVarOut (value);
        rodsLog (LOG_ERROR,
          "getAttValue:  nc_get_att err varid %d dataType %d. %s ",
          varid, dataType, nc_strerror(status));
        status = NETCDF_GET_ATT_ERR + status;
    }
    return status;
}

unsigned int
getNcIntVar (int ncid, int varid, int dataType, rodsLong_t inx)
{
    size_t start[1], count[1];
    short int myshort;
    int myint;
    rodsLong_t mylong;
    float myfloat;
    double mydouble;
    unsigned int retint;
    int status;


    start[0] = inx;
    count[0] = 1;

    if (dataType == NC_SHORT || dataType == NC_USHORT) {
        status = nc_get_vara (ncid, varid, start, count, (void *) &myshort);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "getNcIntVar: nc_get_vara error, status = %d, %s",
              status, nc_strerror(status));
            return NETCDF_GET_VARS_ERR - status;
        }
        retint = (unsigned int) myshort;
    } else if (dataType == NC_INT || dataType == NC_UINT) {
        status = nc_get_vara (ncid, varid, start, count, (void *) &myint);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "getNcIntVar: nc_get_vara error, status = %d, %s",
              status, nc_strerror(status));
            return NETCDF_GET_VARS_ERR - status;
        }
        retint = (unsigned int) myint;
    } else if (dataType == NC_INT64 || dataType == NC_UINT64) {
        status = nc_get_vara (ncid, varid, start, count, (void *) &mylong);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "getNcIntVar: nc_get_vara error, status = %d, %s",
              status, nc_strerror(status));
            return NETCDF_GET_VARS_ERR - status;
        }
        retint = (unsigned int) mylong;
   } else if (dataType == NC_FLOAT) {
        status = nc_get_vara (ncid, varid, start, count, (void *) &myfloat);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "getNcIntVar: nc_get_vara error, status = %d, %s",
              status, nc_strerror(status));
            return NETCDF_GET_VARS_ERR - status;
        }
        retint = (unsigned int) myfloat;
    } else if (dataType == NC_DOUBLE) {
        status = nc_get_vara (ncid, varid, start, count, (void *) &mydouble);
        if (status != NC_NOERR) {
            rodsLog (LOG_ERROR,
              "getNcIntVar: nc_get_vara error, status = %d, %s",
              status, nc_strerror(status));
            return NETCDF_GET_VARS_ERR - status;
        }
        retint = (unsigned int) mydouble;
    } else {
        rodsLog (LOG_ERROR,
          "getNcIntVar: Unsupported dataType %d", dataType);
        return (NETCDF_INVALID_DATA_TYPE);
    }

    return retint;
}
#endif	/* NETCDF_API */

int
getTimeInxInVar (ncInqOut_t *ncInqOut, int varid)
{
    int i;
    int timeDimId = -1;
    int varInx = -1;

    for (i = 0; i < ncInqOut->ndims; i++) {
        if (strcasecmp (ncInqOut->dim[i].name, "time") == 0) {
            timeDimId = i;
            break;
        }
    }
    if (timeDimId < 0) return NETCDF_AGG_ELE_FILE_NO_TIME_DIM;
    for (i = 0; i < ncInqOut->nvars; i++) {
        if (ncInqOut->var[i].id == varid) {
            varInx = i;
            break;
        }
    }
    if (varInx < 0) return NETCDF_DEF_VAR_ERR;
    /* try to fine the time range */
    for (i = 0; i < ncInqOut->var[varInx].nvdims; i++) {
        if (ncInqOut->var[varInx].dimId[i] == timeDimId) {
            return i;
        }
    }
    return NETCDF_AGG_ELE_FILE_NO_TIME_DIM; 
}

unsigned int
ncValueToInt (int dataType, void **invalue)
{
    void *value = *invalue;
    char **ptr = (char **) invalue;
    unsigned short myshort;
    unsigned int myInt;
    rodsLong_t myLong;
    float myFloat;
    double myDouble;

    switch (dataType) {
        case NC_SHORT:
        case NC_USHORT:
            myshort = *(short int*) value;
            myInt = myshort;
	    *ptr+= sizeof (short);	/* advance pointer */
	    break;
	case NC_INT:
	case NC_UINT:
            myInt = *(unsigned int*) value;
	    *ptr+= sizeof (int);	/* advance pointer */
	    break;
	case NC_INT64:
	case NC_UINT64:
            myLong = *(rodsLong_t *) value;
            myInt = myLong;
	    *ptr+= sizeof (rodsLong_t);	/* advance pointer */
	    break;
	case NC_FLOAT:
            myFloat = *(float *) value;
            myInt = myFloat;
	    *ptr+= sizeof (float);	/* advance pointer */
	    break;
	case NC_DOUBLE:
            myDouble = *(double *) value;
            myInt = myDouble;
	    *ptr+= sizeof (double);	/* advance pointer */
	    break;
      default:
        rodsLog (LOG_ERROR,
          "ncValueToInt: Unknow dataType %d for time", dataType);
        return (NETCDF_INVALID_DATA_TYPE);
    }
    return myInt;
}

rodsLong_t
getTimeStepSize (ncInqOut_t *ncInqOut)
{
    int timeDimInx;
    int i, j;
    rodsLong_t  totalSize = 0;

    for (timeDimInx = 0; timeDimInx < ncInqOut->ndims; timeDimInx++) {
        if (strcasecmp (ncInqOut->dim[timeDimInx].name, "time") == 0) break;
    }
    if (timeDimInx >= ncInqOut->ndims) {
        /* no match */
        rodsLog (LOG_ERROR,
          "_rsNcArchTimeSeries: 'time' dim does not exist");
        return NETCDF_DIM_MISMATCH_ERR;
    }

    for (i = 0; i < ncInqOut->nvars; i++) {
        int varSize = getDataTypeSize (ncInqOut->var[i].dataType);
        for (j = 0; j < ncInqOut->var[i].nvdims; j++) {
            int dimId = ncInqOut->var[i].dimId[j];
            /* skip time dim */
            if (dimId == timeDimInx) continue;
            varSize *= ncInqOut->dim[dimId].arrayLen;
        }
        totalSize += varSize;
    }
    return totalSize;
}
    
