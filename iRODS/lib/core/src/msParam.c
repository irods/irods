/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* msParam.c - function for handling msParam_t.
 */

#include "msParam.h"
#include "apiHeaderAll.h"
#include "modDataObjMeta.h"
#include "rcGlobalExtern.h"

/* addMsParam - This is for backward compatibility only.
 *  addMsParamToArray should be used for all new functions
 */

int
addMsParam (msParamArray_t *msParamArray, char *label,
char *type, void *inOutStruct, bytesBuf_t *inpOutBuf)
{
    int status = addMsParamToArray (msParamArray, label, type,
      inOutStruct, inpOutBuf, 0);

    return (status);
}

int
addIntParamToArray (msParamArray_t *msParamArray, char *label, int inpInt)
{
    int *myInt;
    int status;

    myInt = (int *)malloc (sizeof (int));
    *myInt = inpInt;
    status = addMsParamToArray (msParamArray, label, INT_MS_T, myInt, NULL, 0);
    return status;
}

/* addMsParamToArray - Add a msParam_t to the msParamArray.
 * Input char *label - an element of the msParam_t. This input must be 
 *            non null.
 *       char *type - can be NULL 
 *       void *inOutStruct - can be NULL;
 *       bytesBuf_t *inpOutBuf - can be NULL
 *	 int replFlag - label and type will be automatically replicated
 *         (strdup). If replFlag == 0, only the pointers of inOutStruct
 *         and inpOutBuf will be passed. If replFlag == 1, the inOutStruct
 *         and inpOutBuf will be replicated.
 */

int
addMsParamToArray (msParamArray_t *msParamArray, char *label, 
char *type, void *inOutStruct, bytesBuf_t *inpOutBuf, int replFlag)
{
    msParam_t **newParam;
    int len, newLen;
    int i;

    if (msParamArray == NULL || label == NULL) {
	rodsLog (LOG_ERROR, 
	  "addMsParam: NULL msParamArray or label input");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    len = msParamArray->len;

    for (i = 0; i < len; i++) {
	if (msParamArray->msParam[i]->label == NULL) 
	    continue;
	if (strcmp (msParamArray->msParam[i]->label, label) == 0) {
	  /*** RAJA ADDED Jan 28 2010 to make it not given an error ***/
	  if (!strcmp(msParamArray->msParam[i]->type,STR_MS_T) &&
	      !strcmp(type,STR_MS_T) &&
	      !strcmp( (char *) inOutStruct, (char *) msParamArray->msParam[i]->inOutStruct)) {
	    return(0);
	  }
	  /*** RAJA ADDED Jan 28 2010 to make it not given an error ***/
	    rodsLog (LOG_ERROR,
	      "addMsParam: Two params have the same label %s", label);
	    if (!strcmp(msParamArray->msParam[i]->type,STR_MS_T))
	      rodsLog (LOG_ERROR,
		       "addMsParam: old string value = %s\n", (char *) msParamArray->msParam[i]->inOutStruct);
	    else
	      rodsLog (LOG_ERROR,
		       "addMsParam: old param is of type: %s\n", msParamArray->msParam[i]->type);
	    if (!strcmp(type,STR_MS_T))
	      rodsLog (LOG_ERROR,
		       "addMsParam: new string value = %s\n", (char *) inOutStruct);
	    else
	      rodsLog (LOG_ERROR,
		       "addMsParam: new param is of type: %s\n", type);
	    return (USER_PARAM_LABEL_ERR);
	}
    } 

    if ((msParamArray->len % PTR_ARRAY_MALLOC_LEN) == 0) {
        newLen = msParamArray->len + PTR_ARRAY_MALLOC_LEN;
        newParam = (msParam_t **) malloc (newLen * sizeof (newParam));
        memset (newParam, 0, newLen * sizeof (newParam));
        for (i = 0; i < len; i++) {
	    newParam[i] = msParamArray->msParam[i];
        }
        if (msParamArray->msParam != NULL)
            free (msParamArray->msParam);
        msParamArray->msParam = newParam;
    }

    msParamArray->msParam[len] = (msParam_t *) malloc (sizeof (msParam_t));
    memset (msParamArray->msParam[len], 0, sizeof (msParam_t));
    if (replFlag == 0) {
        fillMsParam (msParamArray->msParam[len], label, type, inOutStruct,
          inpOutBuf);
    } else {
	msParam_t inMsParam;
	inMsParam.label = label;
	inMsParam.type = type;
	inMsParam.inOutStruct = inOutStruct;
	inMsParam.inpOutBuf = inpOutBuf;
	replMsParam (&inMsParam, msParamArray->msParam[len]);
    }
    msParamArray->len++;

    return (0);
}

int
replMsParamArray (msParamArray_t *msParamArray, 
msParamArray_t *outMsParamArray) 
{
    int newLen;
    int i;
    int status = 0;

    memset (outMsParamArray, 0, sizeof (msParamArray_t));
 
    newLen = (msParamArray->len / PTR_ARRAY_MALLOC_LEN + 1) * 
      PTR_ARRAY_MALLOC_LEN;

    outMsParamArray->msParam = 
      (msParam_t **) malloc (newLen * sizeof (outMsParamArray->msParam));
    memset (outMsParamArray->msParam, 0, 
      newLen * sizeof (outMsParamArray->msParam));

    outMsParamArray->len = msParamArray->len;
    for (i = 0; i < msParamArray->len; i++) {
	msParam_t *outMsParam;

	outMsParam = outMsParamArray->msParam[i] = 
	  (msParam_t *) malloc (sizeof (msParam_t));
	memset (outMsParamArray->msParam[i], 0, sizeof (msParam_t));
	status = replMsParam (msParamArray->msParam[i], 
	  outMsParamArray->msParam[i]);
    }
    return (status);
}

int
replMsParam (msParam_t *msParam, msParam_t *outMsParam)
{
    char *label, *type;
    void *inOutStruct;
    bytesBuf_t *inpOutBuf;
    int status;

    label = msParam->label;
    type = msParam->type;
    inOutStruct = msParam->inOutStruct;
    inpOutBuf = msParam->inpOutBuf;

    if (label != NULL) {
        outMsParam->label = strdup (label);
    }

    if (type != NULL) {
        outMsParam->type = strdup (type);
    }

    status = replInOutStruct (inOutStruct, &outMsParam->inOutStruct, type);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
           "replMsParamArray: replInOutStruct error for type %s", type);
	return status;
    }

    if (inpOutBuf != NULL && inpOutBuf->len > 0) {
        outMsParam->inpOutBuf = (bytesBuf_t *) malloc (sizeof (bytesBuf_t));
        outMsParam->inpOutBuf->len = inpOutBuf->len;
        outMsParam->inpOutBuf->buf = malloc (inpOutBuf->len + 100);
        memcpy (outMsParam->inpOutBuf->buf, inpOutBuf->buf, inpOutBuf->len);
    }
    return (0);
}

int
replInOutStruct (void *inStruct, void **outStruct, char *type)
{
    int status;

    if (outStruct == NULL) {
	return SYS_INTERNAL_NULL_INPUT_ERR;
    } else {
	*outStruct = NULL;
    }

    if (inStruct != NULL && type != NULL) {
        if (strcmp (type, STR_MS_T) == 0) {
            *outStruct = (void *) strdup ((char *)inStruct);
        } else {
            bytesBuf_t *packedResult;
            status = packStruct (inStruct, &packedResult, type,
              NULL, 0, NATIVE_PROT);
            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                   "replInOutStruct: packStruct error for type %s", type);
                return (status);
            }
            status = unpackStruct (packedResult->buf,
              (void **) outStruct, type, NULL, NATIVE_PROT);
            freeBBuf (packedResult);
            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                   "replInOutStruct: unpackStruct error for type %s",
                  type);
                return (status);
            }
        }
    }
    return (0);
}

int
fillMsParam (msParam_t *msParam, char *label,
char *type, void *inOutStruct, bytesBuf_t *inpOutBuf)
{
    if (label != NULL) {
        msParam->label = strdup (label);
    }

    if (type != NULL) {
        msParam->type = strdup (type);
    }
    /*** RAJA Changed JUn 27, 2007 from msParam->inOutStruct != NULL to inOutStruct != NULL ***/
    if (inOutStruct != NULL && msParam->type != NULL &&
      strcmp (msParam->type, STR_MS_T) == 0) {
	msParam->inOutStruct = (void *) strdup ((char *)inOutStruct);
    } else {
        msParam->inOutStruct = inOutStruct;
    }
    msParam->inpOutBuf = inpOutBuf;

    return (0);
}

int 
fillIntInMsParam (msParam_t *msParam, int myInt)
{
    int *myInOutStruct;

    if (msParam != NULL) {
        myInOutStruct = (int*)malloc (sizeof (int));
        *myInOutStruct = myInt;
        fillMsParam (msParam, NULL, INT_MS_T, myInOutStruct, NULL);
    }
    return (0);
}

int
fillStrInMsParam (msParam_t *msParam, char *myStr)
{

    if (msParam != NULL) {
	/* XXXX memory leak as fillMsParam will strdup too */
#if 0
        char *myInOutStruct;
        myInOutStruct = strdup (myStr);
        fillMsParam (msParam, NULL, STR_MS_T, myInOutStruct, NULL);
#else
        fillMsParam (msParam, NULL, STR_MS_T, myStr, NULL);
#endif
    }
    return (0);
}

int
fillBufLenInMsParam (msParam_t *msParam, int myInt, bytesBuf_t *bytesBuf)
{
    int *myInOutStruct;

    if (msParam != NULL) {
        myInOutStruct = (int*)malloc (sizeof (int));
        *myInOutStruct = myInt;
        fillMsParam (msParam, NULL, BUF_LEN_MS_T, myInOutStruct, bytesBuf);
    }
    return (0);
}


int
printMsParam (msParamArray_t *outParamArray) 
{

  char buf[10000];
  int i,j;
  msParam_t *msParam;

  if (outParamArray == NULL) {
    return (0);
  }

  for (i = 0; i < outParamArray->len; i++) {
    msParam = outParamArray->msParam[i];
    j = writeMsParam(buf, 10000,  msParam);
    if (j < 0)
      return(j);
    printf("%s", buf);
  }
  return(0);
}


int
writeMsParam (char *buf, int len, msParam_t *msParam) 
{
    int j;
    keyValPair_t *kVPairs;
    tagStruct_t *tagValues;

    buf[0]= '\0';


    if (msParam->label != NULL && 
	msParam->type != NULL && 
	msParam->inOutStruct != NULL) {
      if (strcmp (msParam->type, STR_MS_T) == 0)
	snprintf(&buf[strlen(buf)], len - strlen(buf), "%s: %s\n",msParam->label, (char *) msParam->inOutStruct);
      else  if (strcmp (msParam->type, INT_MS_T) == 0)
	snprintf(&buf[strlen(buf)], len - strlen(buf), "%s: %i\n",msParam->label,*(int *) msParam->inOutStruct);
      else if (strcmp (msParam->type, KeyValPair_MS_T) == 0 ) {
	kVPairs = (keyValPair_t *)msParam->inOutStruct;
	snprintf(&buf[strlen(buf)], len - strlen(buf), "KVpairs %s: %i\n", msParam->label, kVPairs->len);
	for (j = 0; j < kVPairs->len; j++) {
	  snprintf(&buf[strlen(buf)], len - strlen(buf), "       %s = %s\n",kVPairs->keyWord[j],
		  kVPairs->value[j]);
	}
      }
      else if (strcmp (msParam->type, TagStruct_MS_T) == 0 ) {
	tagValues = (tagStruct_t *) msParam->inOutStruct;
	snprintf(&buf[strlen(buf)], len - strlen(buf), "Tags %s: %i\n", msParam->label, tagValues->len);
	for (j = 0; j < tagValues->len; j++) {
	  snprintf(&buf[strlen(buf)], len - strlen(buf), "       AttName = %s\n", tagValues->keyWord[j]);
	  snprintf(&buf[strlen(buf)], len - strlen(buf), "       PreTag  = %s\n", tagValues->preTag[j]);
	  snprintf(&buf[strlen(buf)], len - strlen(buf), "       PostTag = %s\n", tagValues->postTag[j]);
	}
      }
      else if (strcmp (msParam->type, ExecCmdOut_MS_T) == 0) {
	execCmdOut_t *execCmdOut;
	execCmdOut = (execCmdOut_t *) msParam->inOutStruct;
	if (execCmdOut->stdoutBuf.buf != NULL) 
	  snprintf(&buf[strlen(buf)], len - strlen(buf), "STDOUT = %s",(char *) execCmdOut->stdoutBuf.buf);
	if (execCmdOut->stderrBuf.buf != NULL) 
	  snprintf(&buf[strlen(buf)], len - strlen(buf), "STRERR = %s", (char *) execCmdOut->stderrBuf.buf);
      }
    }

    /**** RAJA MOdified to simplify June 1 2010 ****/
#if 0
    msParam = outParamArray->msParam[i];
    
    snprintf(&buf[strlen(buf)], len - strlen(buf), "  output index: %d\n", i);
    if (msParam->label != NULL) {
      snprintf(&buf[strlen(buf)], len - strlen(buf), "    label: %s\n", msParam->label); 
    }
    if (msParam->type != NULL) {
      snprintf(&buf[strlen(buf)], len - strlen(buf), "    type: %s\n", msParam->type); 
    }
    if (msParam->inOutStruct != NULL) {
      if (strcmp (msParam->type, STR_MS_T) == 0) {
	/* print the string */
	snprintf(&buf[strlen(buf)], len - strlen(buf), "    str content: %s\n", (char *) msParam->inOutStruct);  
      } else if (strcmp (msParam->type, INT_MS_T) == 0) {
	/* print the int */
	snprintf(&buf[strlen(buf)], len - strlen(buf), "    int content: %d\n", *(int *) msParam->inOutStruct);
      } else if (strcmp (msParam->type, KeyValPair_MS_T) == 0 ) {
	kVPairs = (keyValPair_t *)msParam->inOutStruct;
	snprintf(&buf[strlen(buf)], len - strlen(buf), "      num of kv pairs: %i\n", kVPairs->len);
	for (j = 0; j < kVPairs->len; j++) {
	  snprintf(&buf[strlen(buf)], len - strlen(buf), "       %s = %s\n",kVPairs->keyWord[j],
		  kVPairs->value[j]);
	}
      } else if (strcmp (msParam->type, TagStruct_MS_T) == 0 ) {
	tagValues = (tagStruct_t *) msParam->inOutStruct;
	snprintf(&buf[strlen(buf)], len - strlen(buf), "      num of tags : %i\n", tagValues->len);
	for (j = 0; j < tagValues->len; j++) {
	  snprintf(&buf[strlen(buf)], len - strlen(buf), "       AttName = %s\n", tagValues->keyWord[j]);
	  snprintf(&buf[strlen(buf)], len - strlen(buf), "       PreTag  = %s\n", tagValues->preTag[j]);
	  snprintf(&buf[strlen(buf)], len - strlen(buf), "       PostTag = %s\n", tagValues->postTag[j]);
	}
      } else if (strcmp (msParam->type, ExecCmdOut_MS_T) == 0) {
	execCmdOut_t *execCmdOut;
	execCmdOut = (execCmdOut_t *) msParam->inOutStruct;
	snprintf(&buf[strlen(buf)], len - strlen(buf), "       status = %d\n", execCmdOut->status);
	if (execCmdOut->stdoutBuf.buf != NULL) {
	  snprintf(&buf[strlen(buf)], len - strlen(buf), "       stdout = %s", 
		  (char *) execCmdOut->stdoutBuf.buf);
	}
	if (execCmdOut->stderrBuf.buf != NULL) {
	  snprintf(&buf[strlen(buf)], len - strlen(buf), "       strerr = %s", 
		   (char *) execCmdOut->stderrBuf.buf);
	}
      }
    }
#endif
    if (msParam->inpOutBuf != NULL) {
      snprintf(&buf[strlen(buf)], len - strlen(buf), "    outBuf: buf length = %d\n", msParam->inpOutBuf->len);
    }
    
    return (0);
}


msParam_t *
getMsParamByLabel (msParamArray_t *msParamArray, char *label)
{
    int i;

    if (msParamArray == NULL || msParamArray->msParam == NULL ||label == NULL) {
	return NULL;
    }

    for (i = 0; i < msParamArray->len; i++) {
        if (strcmp (msParamArray->msParam[i]->label, label) == 0) {
            return (msParamArray->msParam[i]);
        }
    }
    return (NULL);
}

msParam_t *
getMsParamByType (msParamArray_t *msParamArray, char *type)
{
    int i;

    if (msParamArray == NULL || msParamArray->msParam == NULL || type == NULL) {
        return NULL;
    }

    for (i = 0; i < msParamArray->len; i++) {
        if (strcmp (msParamArray->msParam[i]->type, type) == 0) {
            return (msParamArray->msParam[i]);
        }
    }
    return (NULL);
}

void 
*getMspInOutStructByLabel (msParamArray_t *msParamArray, char *label) 
{
    int i;  

    if (msParamArray == NULL || label == NULL) { 
        return NULL;
    }

    for (i = 0; i < msParamArray->len; i++) {
        if (strcmp (msParamArray->msParam[i]->label, label) == 0) { 
            return (msParamArray->msParam[i]->inOutStruct);
        }
    }
    return (NULL); 
}

int
rmMsParamByLabel (msParamArray_t *msParamArray, char *label, int freeStruct)
{
    int i, j;

    if (msParamArray == NULL || label == NULL) {
        return 0;
    }

    for (i = 0; i < msParamArray->len; i++) {
        if (strcmp (msParamArray->msParam[i]->label, label) == 0) {
	    clearMsParam (msParamArray->msParam[i], freeStruct);
	    free (msParamArray->msParam[i]);
	    /* move the rest up */
	    for (j = i + 1; j < msParamArray->len; j++) {
		msParamArray->msParam[j - 1] = msParamArray->msParam[j];
	    }
	    msParamArray->len --;
	    break;
        }
    }
    return (0);
}

int
clearMsParamArray (msParamArray_t *msParamArray, int freeStruct)
{
    int i;

    if (msParamArray == NULL) {
        return 0;
    }

    for (i = 0; i < msParamArray->len; i++) {
        clearMsParam (msParamArray->msParam[i], freeStruct);
        free (msParamArray->msParam[i]);
    }

    if (msParamArray->len > 0 && msParamArray->msParam != NULL) {
	free (msParamArray->msParam);
	memset (msParamArray, 0, sizeof (msParamArray_t));
    }

    return (0);
}

int
clearMsParam (msParam_t *msParam, int freeStruct)
{
    if (msParam == NULL)
	return (0);

    if (msParam->label != NULL)
	free (msParam->label);
    /* XXXXXX - need to free their internal struct too */
    /* free STR_MS_T too 7/8/10 mw */
#if 0
    if (msParam->inOutStruct != NULL && freeStruct > 0) { 
#endif
    if (msParam->inOutStruct != NULL && (freeStruct > 0 || 
      (msParam->type != NULL && strcmp (msParam->type, STR_MS_T) == 0))) {
	free (msParam->inOutStruct);
    }
    if (msParam->type != NULL)
        free (msParam->type);

    memset (msParam, 0, sizeof (msParam_t));
    return (0);
}


/* clears everything but the label */
int
resetMsParam(msParam_t *msParam)
{
	if (msParam == NULL) {
		return (0);
	}
		
	if (msParam->type != NULL) {
		free (msParam->type);
	}
	
	if (msParam->inOutStruct != NULL) {
		free (msParam->inOutStruct);
	}

	if (msParam->inpOutBuf != NULL) {
		freeBBuf (msParam->inpOutBuf);
	}
	
	return (0);
}


int
trimMsParamArray (msParamArray_t *msParamArray, char *outParamDesc)
{
    strArray_t strArray;
    int status; 
    int i, j, k;
    char *value;
    msParam_t **msParam;

    if (msParamArray == NULL) {
        return 0;
    }

    memset (&strArray, 0, sizeof (strArray));

    if (outParamDesc != NULL && strlen (outParamDesc) > 0) {
        status = parseMultiStr (outParamDesc, &strArray);
        if (status < 0) {
	    rodsLog (LOG_ERROR,
	      "trimMsParamArray: parseMultiStr error, status = %d", status);
	}
    }

    msParam = msParamArray->msParam;
    value = strArray.value;

    if (strArray.len == 1 && strcmp (value, ALL_MS_PARAM_KW) == 0) {
	/* retain all msParam */
	return 0;
    }

    i = 0;
    while (i < msParamArray->len) {
	int match;
	int nullType;

	match = 0;
	nullType = 0;
	if (msParam[i]->type == NULL || strlen (msParam[i]->type) == 0) {
	    nullType = 1;
	} else { 
            for (k = 0; k < strArray.len; k++) {
	        if (strcmp (&value[k * strArray.size], msParam[i]->label) 
		  == 0) {
		    match = 1;
		    break;
	        }
	    }
        }
	if (match == 0 || nullType == 1) {
            if (nullType != 1)
	      clearMsParam (msParamArray->msParam[i], 1);
	    free (msParamArray->msParam[i]);
            /* move the rest up */
            for (j = i + 1; j < msParamArray->len; j++) {
                msParamArray->msParam[j - 1] = msParamArray->msParam[j];
            }
            msParamArray->len --;
	} else {
	    i++;
	}
    }
    if (value != NULL)
        free (value);

    return (0);
}

/* parseMspForDataObjInp - This is a rather convoluted subroutine because
 * it tries to parse DataObjInp that have different types of msParam
 * in inpParam and different modes of output.
 *
 * If outputToCache == 0 and inpParam is DataObjInp_MS_T, *outDataObjInp
 *    will be set to the pointer given by inpParam->inOutStruct.
 * If inpParam is STR_MS_T or KeyValPair_MS_T, regardles of the value of
 *    outputToCache, the dataObjInpCache will be used to contain the output 
 *    if it is not NULL. Otherwise, one will be malloc'ed (be sure to free
 *    it after your are done). 
 * If outputToCache == 1, the dataObjInpCache will be used to contain the 
 *    output if it is not NULL. Otherwise, one will be malloc'ed (be sure to 
 *    free it after your are done).
 */

int
parseMspForDataObjInp (msParam_t *inpParam, dataObjInp_t *dataObjInpCache, 
dataObjInp_t **outDataObjInp, int outputToCache)
{
    *outDataObjInp = NULL;

    if (inpParam == NULL) {
        rodsLog (LOG_ERROR,
          "parseMspForDataObjInp: input inpParam is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if (strcmp (inpParam->type, STR_MS_T) == 0) {
        /* str input */
        if (dataObjInpCache == NULL) {
            dataObjInpCache = (dataObjInp_t *)malloc (sizeof (dataObjInp_t));
        }
        memset (dataObjInpCache, 0, sizeof (dataObjInp_t));
	*outDataObjInp = dataObjInpCache;
	if (strcmp ((char *) inpParam->inOutStruct, "null") != 0) {
            rstrcpy (dataObjInpCache->objPath, (char*)inpParam->inOutStruct, 
	      MAX_NAME_LEN);
	}
        return (0);
    } else if (strcmp (inpParam->type, DataObjInp_MS_T) == 0) {
	if (outputToCache == 1) {
	    dataObjInp_t *tmpDataObjInp;
	    tmpDataObjInp = (dataObjInp_t *)inpParam->inOutStruct;
            if (dataObjInpCache == NULL) {
                dataObjInpCache = (dataObjInp_t *)malloc (sizeof (dataObjInp_t));
            }
	    *dataObjInpCache = *tmpDataObjInp;
	    /* zero out the condition of the original because it has been
	     * moved */
	    memset (&tmpDataObjInp->condInput, 0, sizeof (keyValPair_t));
	    *outDataObjInp = dataObjInpCache;
	} else {
            *outDataObjInp = (dataObjInp_t *) inpParam->inOutStruct;
	}
	return (0);
    } else if (strcmp (inpParam->type, KeyValPair_MS_T) == 0) {
        /* key-val pair input needs ketwords "DATA_NAME" and  "COLL_NAME" */
        char *dVal, *cVal;
        keyValPair_t *kW;
        kW = (keyValPair_t *)inpParam->inOutStruct;
        if ((dVal = getValByKey (kW,"DATA_NAME")) == NULL)
	    return(USER_PARAM_TYPE_ERR);
        if ((cVal = getValByKey (kW,"COLL_NAME")) == NULL)
	    return(USER_PARAM_TYPE_ERR);

        if (dataObjInpCache == NULL) {
            dataObjInpCache = (dataObjInp_t *)malloc (sizeof (dataObjInp_t));
        }

        memset (dataObjInpCache, 0, sizeof (dataObjInp_t));
        snprintf (dataObjInpCache->objPath, MAX_NAME_LEN, "%s/%s", cVal,dVal);
        *outDataObjInp = dataObjInpCache;
        return(0);
    } else {
        rodsLog (LOG_ERROR,
          "parseMspForDataObjInp: Unsupported input Param1 type %s",
          inpParam->type);
        return (USER_PARAM_TYPE_ERR);
    }
}

/* parseMspForCollInp - see the explanation given for parseMspForDataObjInp
 */

int
parseMspForCollInp (msParam_t *inpParam, collInp_t *collInpCache, 
collInp_t **outCollInp, int outputToCache)
{
    *outCollInp = NULL;

    if (inpParam == NULL) {
        rodsLog (LOG_ERROR,
          "parseMspForCollInp: input inpParam is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if (strcmp (inpParam->type, STR_MS_T) == 0) {
        /* str input */
        if (collInpCache == NULL) {
            collInpCache = (collInp_t *)malloc (sizeof (collInp_t));
        }
        memset (collInpCache, 0, sizeof (collInp_t));
	*outCollInp = collInpCache;
	if (strcmp ((char *) inpParam->inOutStruct, "null") != 0) {
            rstrcpy (collInpCache->collName, (char*)inpParam->inOutStruct, 
	      MAX_NAME_LEN);
	}
        return (0);
    } else if (strcmp (inpParam->type, CollInp_MS_T) == 0) {
	if (outputToCache == 1) {
	    collInp_t *tmpCollInp;
	    tmpCollInp = (collInp_t *)inpParam->inOutStruct;
            if (collInpCache == NULL) {
                collInpCache = (collInp_t *)malloc (sizeof (collInp_t));
            }
	    *collInpCache = *tmpCollInp;
	    /* zero out the condition of the original because it has been
	     * moved */
	    memset (&tmpCollInp->condInput, 0, sizeof (keyValPair_t));
	    *outCollInp = collInpCache;
	} else {
            *outCollInp = (collInp_t *) inpParam->inOutStruct;
	}
	return (0);
    } else {
        rodsLog (LOG_ERROR,
          "parseMspForCollInp: Unsupported input Param1 type %s",
          inpParam->type);
        return (USER_PARAM_TYPE_ERR);
    }
}

int
parseMspForDataObjCopyInp (msParam_t *inpParam, 
dataObjCopyInp_t *dataObjCopyInpCache, dataObjCopyInp_t **outDataObjCopyInp)
{
    *outDataObjCopyInp = NULL;
    if (inpParam == NULL) {
        rodsLog (LOG_ERROR,
          "parseMspForDataObjCopyInp: input inpParam is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if (strcmp (inpParam->type, STR_MS_T) == 0) {
        /* str input */
        memset (dataObjCopyInpCache, 0, sizeof (dataObjCopyInp_t));
        rstrcpy (dataObjCopyInpCache->srcDataObjInp.objPath,
          (char *) inpParam->inOutStruct, MAX_NAME_LEN);
        *outDataObjCopyInp = dataObjCopyInpCache;
    } else if (strcmp (inpParam->type, DataObjCopyInp_MS_T) == 0) {
        *outDataObjCopyInp = (dataObjCopyInp_t*)inpParam->inOutStruct;
    } else if (strcmp (inpParam->type, DataObjInp_MS_T) == 0) {
        memset (dataObjCopyInpCache, 0, sizeof (dataObjCopyInp_t));
        dataObjCopyInpCache->srcDataObjInp =
         *(dataObjInp_t *)inpParam->inOutStruct;
        *outDataObjCopyInp = dataObjCopyInpCache;
    } else if (strcmp (inpParam->type, KeyValPair_MS_T) == 0) {
      /* key-val pair input needs ketwords "DATA_NAME" and  "COLL_NAME" */
      char *dVal, *cVal;
      keyValPair_t *kW;
      kW = (keyValPair_t *)inpParam->inOutStruct;
      if ((dVal = getValByKey (kW,"DATA_NAME")) == NULL)
	return(USER_PARAM_TYPE_ERR);
      if ((cVal = getValByKey (kW,"COLL_NAME")) == NULL)
	return(USER_PARAM_TYPE_ERR);
      memset (dataObjCopyInpCache, 0, sizeof (dataObjCopyInp_t));
      snprintf (dataObjCopyInpCache->srcDataObjInp.objPath, MAX_NAME_LEN, "%s/%s", cVal,dVal);
      *outDataObjCopyInp = dataObjCopyInpCache;
      return(0);
    } else {
        rodsLog (LOG_ERROR,
          "parseMspForDataObjCopyInp: Unsupported input Param1 type %s",
          inpParam->type);
        return (USER_PARAM_TYPE_ERR);
    }
    return (0);
}

/* parseMspForCondInp - Take the inpParam->inOutStruct and use that as the
 * value of the keyVal pair. The KW of the keyVal pair is given in condKw.
 */
 
int
parseMspForCondInp (msParam_t *inpParam, keyValPair_t *condInput, 
char *condKw)
{
    if (inpParam != NULL) {
        if (strcmp (inpParam->type, STR_MS_T) == 0) {
            /* str input */
            if (strcmp ((char *) inpParam->inOutStruct, "null") != 0) {
                addKeyVal (condInput, condKw,
                  (char *) inpParam->inOutStruct);
            }
        } else {
            rodsLog (LOG_ERROR, 
              "parseMspForCondInp: Unsupported input Param type %s",
              inpParam->type);
            return (USER_PARAM_TYPE_ERR);
        }
    }
    return (0);
}
 
/* parseMspForCondKw - Take the KW from inpParam->inOutStruct and add that
 * to condInput. The value of the keyVal  pair is assumed to be "".
 */
 
int
parseMspForCondKw (msParam_t *inpParam, keyValPair_t *condInput)
{
   if (inpParam != NULL) {
        if (strcmp (inpParam->type, STR_MS_T) == 0) {
            /* str input */
            if (strcmp ((char *) inpParam->inOutStruct, "null") != 0 &&
	      strlen ((const char*)inpParam->inOutStruct) > 0) {
                addKeyVal (condInput, (char *) inpParam->inOutStruct, "");
            }
        } else {
            rodsLog (LOG_ERROR,
              "parseMspForCondKw: Unsupported input Param type %s",
              inpParam->type);
            return (USER_PARAM_TYPE_ERR);
        }
    }
    return (0);
}

int
parseMspForPhyPathReg (msParam_t *inpParam, keyValPair_t *condInput)
{
    char *tmpStr;

    if (inpParam != NULL) {
        if (strcmp (inpParam->type, STR_MS_T) == 0) {
	    tmpStr = (char *) inpParam->inOutStruct;
            /* str input */
            if (tmpStr != NULL && strlen (tmpStr) > 0 &&
	      strcmp (tmpStr, "null") != 0) {
		if (strcmp (tmpStr, COLLECTION_KW) == 0) {
                    addKeyVal (condInput, COLLECTION_KW, "");
		} else if (strcmp (tmpStr, MOUNT_POINT_STR) == 0) {
            	    addKeyVal (condInput, COLLECTION_TYPE_KW, MOUNT_POINT_STR);
		} else if (strcmp (tmpStr, LINK_POINT_STR) == 0) {
            	    addKeyVal (condInput, COLLECTION_TYPE_KW, LINK_POINT_STR);
		}
            }
        } else {
            rodsLog (LOG_ERROR,
              "parseMspForCondKw: Unsupported input Param type %s",
              inpParam->type);
            return (USER_PARAM_TYPE_ERR);
        }
    }
    return (0);
}

int
parseMspForPosInt (msParam_t *inpParam) 
{
    int myInt;

    if (strcmp (inpParam->type, STR_MS_T) == 0) {
        /* str input */
	if (strcmp ((char *) inpParam->inOutStruct, "null") == 0) {
	    return (SYS_NULL_INPUT);
	}
	myInt = atoi ((const char*)inpParam->inOutStruct);
    } else if (strcmp (inpParam->type, INT_MS_T) == 0 || 
      strcmp (inpParam->type, BUF_LEN_MS_T) == 0) {
        myInt = *(int *)inpParam->inOutStruct;
    } else {
        rodsLog (LOG_ERROR, 
          "parseMspForPosInt: Unsupported input Param type %s",
          inpParam->type);
        return (USER_PARAM_TYPE_ERR);
    }
    if (myInt < 0) {
        rodsLog (LOG_DEBUG, 
          "parseMspForPosInt: parsed int %d is negative", myInt);
    }
    return (myInt);
}

char *
parseMspForStr (msParam_t *inpParam)
{
    if (inpParam == NULL || inpParam->inOutStruct == NULL) {
	return (NULL);
    }

    if (strcmp (inpParam->type, STR_MS_T) != 0) {
	rodsLog (LOG_ERROR,
	  "parseMspForStr: inpParam type %s is not STR_MS_T",
	  inpParam->type);
    }

    if (strcmp ((char *) inpParam->inOutStruct, "null") == 0) {
	return (NULL);
    }

    return (char *)(inpParam->inOutStruct);
}

int
parseMspForExecCmdInp (msParam_t *inpParam, 
execCmd_t *execCmdInpCache, execCmd_t **ouExecCmdInp)
{
    *ouExecCmdInp = NULL;
    if (inpParam == NULL) {
        rodsLog (LOG_ERROR,
          "parseMspForExecCmdInp: input inpParam is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if (strcmp (inpParam->type, STR_MS_T) == 0) {
        /* str input */
        memset (execCmdInpCache, 0, sizeof (execCmd_t));
        rstrcpy (execCmdInpCache->cmd,
          (char *) inpParam->inOutStruct, LONG_NAME_LEN);
        *ouExecCmdInp = execCmdInpCache;
    } else if (strcmp (inpParam->type, ExecCmd_MS_T) == 0) {
        *ouExecCmdInp = (execCmd_t*)inpParam->inOutStruct;
    } else {
        rodsLog (LOG_ERROR,
          "parseMspForExecCmdInp: Unsupported input Param1 type %s",
          inpParam->type);
        return (USER_PARAM_TYPE_ERR);
    }
    return (0);
}
int
getStdoutInExecCmdOut (msParam_t *inpExecCmdOut, char **outStr)
{
    execCmdOut_t *execCmdOut;

    if (inpExecCmdOut == NULL) {
        rodsLog (LOG_ERROR,
          "getStdoutInExecCmdOut input inpParam is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if (strcmp (inpExecCmdOut->type, ExecCmdOut_MS_T) == 0) {
	execCmdOut = (execCmdOut_t *) inpExecCmdOut->inOutStruct;
	if (execCmdOut == NULL) return SYS_INTERNAL_NULL_INPUT_ERR;
	*outStr = (char *)execCmdOut->stdoutBuf.buf;
	return 0;
    } else {
        rodsLog (LOG_ERROR,
          "getStdoutInExecCmdOut: Unsupported input Param type %s",
          inpExecCmdOut->type);
        return (USER_PARAM_TYPE_ERR);
    }
}

int
getStderrInExecCmdOut (msParam_t *inpExecCmdOut, char **outStr)
{
    execCmdOut_t *execCmdOut;

    if (inpExecCmdOut == NULL) {
        rodsLog (LOG_ERROR,
          "getStderrInExecCmdOut input inpParam is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if (strcmp (inpExecCmdOut->type, ExecCmdOut_MS_T) == 0) {
        execCmdOut = (execCmdOut_t *) inpExecCmdOut->inOutStruct;
        if (execCmdOut == NULL) return SYS_INTERNAL_NULL_INPUT_ERR;
        *outStr = (char *)execCmdOut->stderrBuf.buf;
        return 0;
    } else {
        rodsLog (LOG_ERROR,
          "getStderrInExecCmdOut: Unsupported input Param type %s",
          inpExecCmdOut->type);
        return (USER_PARAM_TYPE_ERR);
    }
}

int
initParsedMsKeyValStr (char *inpStr, parsedMsKeyValStr_t *parsedMsKeyValStr)
{
    if (inpStr == NULL || parsedMsKeyValStr == NULL) {
        rodsLog (LOG_ERROR,
          "initParsedMsKeyValStr: input inpStr or parsedMsKeyValStr is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    bzero (parsedMsKeyValStr, sizeof (parsedMsKeyValStr_t));
    parsedMsKeyValStr->inpStr = parsedMsKeyValStr->curPtr = strdup (inpStr);
    parsedMsKeyValStr->endPtr = parsedMsKeyValStr->inpStr + 
      strlen (parsedMsKeyValStr->inpStr);

    return 0;
}

int
clearParsedMsKeyValStr (parsedMsKeyValStr_t *parsedMsKeyValStr)
{
    if (parsedMsKeyValStr == NULL) {
        rodsLog (LOG_ERROR,
          "clearParsedMsKeyValStr: input parsedMsKeyValStr is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    if (parsedMsKeyValStr->inpStr != NULL)
        free (parsedMsKeyValStr->inpStr);

    bzero (parsedMsKeyValStr, sizeof (parsedMsKeyValStr_t));

    return 0;
}

/* getNextKeyValFromMsKeyValStr - parse the inpStr for keyWd value pair.
 * The str is expected to have the format keyWd=value separated by
 * 4 "+" char. e.g. keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 * If the char "=" is not present, the entire string is assumed to
 * be value with a NULL value for kwPtr.
 */
int
getNextKeyValFromMsKeyValStr (parsedMsKeyValStr_t *parsedMsKeyValStr)
{
    char *tmpEndPtr;
    char *equalPtr;

    if (parsedMsKeyValStr == NULL) {
        rodsLog (LOG_ERROR,
          "getNextKeyValFromMsKeyValStr: input parsedMsKeyValStr is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    if (parsedMsKeyValStr->curPtr >= parsedMsKeyValStr->endPtr)
	return NO_MORE_RESULT;

    if ((tmpEndPtr = strstr (parsedMsKeyValStr->curPtr, MS_INP_SEP_STR)) != 
      NULL) {
	/* NULL terminate the str we are trying to parse */
	*tmpEndPtr = '\0';
    } else {
	tmpEndPtr = parsedMsKeyValStr->endPtr;
    }

    if (strcmp (parsedMsKeyValStr->curPtr, MS_NULL_STR) == 0)
	return NO_MORE_RESULT;

    if ((equalPtr = strstr (parsedMsKeyValStr->curPtr, "=")) != NULL) {
	*equalPtr = '\0';
	parsedMsKeyValStr->kwPtr = parsedMsKeyValStr->curPtr;
	if (equalPtr + 1 == tmpEndPtr) {
	    /* pair has no value */
	    parsedMsKeyValStr->valPtr = equalPtr;
	} else {
	    parsedMsKeyValStr->valPtr = equalPtr + 1;
	}
    } else {
        parsedMsKeyValStr->kwPtr = NULL;
        parsedMsKeyValStr->valPtr = parsedMsKeyValStr->curPtr;
    }

    /* advance curPtr */
    parsedMsKeyValStr->curPtr = tmpEndPtr + strlen (MS_INP_SEP_STR);

    return 0;
}

/* parseMsKeyValStrForDataObjInp - parse the msKeyValStr for keyWd value pair
 * and put the kw/value pairs in dataObjInp->condInput.
 * The msKeyValStr is expected to have the format keyWd=value separated by
 * 4 "+" char. e.g. keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 * If the char "=" is not present, the entire string is assumed to
 * be value and hintForMissingKw will provide the missing keyWd.
 */

int
parseMsKeyValStrForDataObjInp (msParam_t *inpParam, dataObjInp_t *dataObjInp,
char *hintForMissingKw, int validKwFlags, char **outBadKeyWd)
{
    char *msKeyValStr;
    keyValPair_t *condInput; 
    parsedMsKeyValStr_t parsedMsKeyValStr;
    int status;


    if (inpParam == NULL || dataObjInp == NULL) {
        rodsLog (LOG_ERROR,
        "parseMsKeyValStrForDataObjInp: input inpParam or dataObjInp is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if (strcmp(inpParam->type, STR_MS_T) != 0) return USER_PARAM_TYPE_ERR;

    msKeyValStr = (char *) inpParam->inOutStruct;

    condInput = &dataObjInp->condInput;

    if (outBadKeyWd != NULL) *outBadKeyWd = NULL;

    if ((status = initParsedMsKeyValStr (msKeyValStr, &parsedMsKeyValStr)) < 0)
	return status;

    while (getNextKeyValFromMsKeyValStr (&parsedMsKeyValStr) >= 0) {
	if (parsedMsKeyValStr.kwPtr == NULL) {
	    if (hintForMissingKw == NULL) {
		status = NO_KEY_WD_IN_MS_INP_STR;
                rodsLogError (LOG_ERROR, status,
      		 "parseMsKeyValStrForDataObjInp: no keyWd for %s",
		  parsedMsKeyValStr.valPtr);
		clearParsedMsKeyValStr (&parsedMsKeyValStr);
        	return status;
	    } else if (strcmp (hintForMissingKw, KEY_WORD_KW) == 0) {
		/* XXXXX need to check if keywd is allowed */
		/* the value should be treated at keyWd */
		parsedMsKeyValStr.kwPtr = parsedMsKeyValStr.valPtr;
		parsedMsKeyValStr.valPtr = parsedMsKeyValStr.endPtr;
	    } else {
		/* use the input hintForMissingKw */
		parsedMsKeyValStr.kwPtr = hintForMissingKw;
	    }
	} 
        if ((status = chkDataObjInpKw (parsedMsKeyValStr.kwPtr,
          validKwFlags)) < 0) {
            if (outBadKeyWd != NULL) 
                *outBadKeyWd = strdup (parsedMsKeyValStr.kwPtr);
            return status;
        }
	/* check for some of the special keyWd */
	if (status == CREATE_MODE_FLAG) {
	    dataObjInp->createMode = atoi (parsedMsKeyValStr.valPtr);
	    continue;
	} else if (status == OPEN_FLAGS_FLAG) {
	    if (strstr (parsedMsKeyValStr.valPtr, "O_RDWR") != NULL)
		dataObjInp->openFlags |= O_RDWR;
	    else if (strstr (parsedMsKeyValStr.valPtr, "O_WRONLY") != NULL)
		dataObjInp->openFlags |= O_WRONLY;
	    else if (strstr (parsedMsKeyValStr.valPtr, "O_RDONLY") != NULL)
		dataObjInp->openFlags |= O_RDONLY;
	    if (strstr (parsedMsKeyValStr.valPtr, "O_CREAT") != NULL)
		dataObjInp->openFlags |= O_CREAT;
	    if (strstr (parsedMsKeyValStr.valPtr, "O_TRUNC") != NULL)
		dataObjInp->openFlags |= O_TRUNC;
	    continue;
	} else if (status == DATA_SIZE_FLAGS) {
	    dataObjInp->dataSize =  strtoll (parsedMsKeyValStr.valPtr, 0, 0);
	    continue;
        } else if (status == NUM_THREADS_FLAG) {
	    dataObjInp->numThreads = atoi (parsedMsKeyValStr.valPtr);
	    continue;
        } else if (status == OPR_TYPE_FLAG) {
	    dataObjInp->oprType = atoi (parsedMsKeyValStr.valPtr);
            continue;
	} else if (status == OBJ_PATH_FLAG) {
            rstrcpy (dataObjInp->objPath, parsedMsKeyValStr.valPtr,
	      MAX_NAME_LEN);
            continue;
	}
        addKeyVal (condInput, parsedMsKeyValStr.kwPtr, 
	  parsedMsKeyValStr.valPtr); 
    }

    clearParsedMsKeyValStr (&parsedMsKeyValStr);

    return 0;
}
 
int
chkDataObjInpKw (char *keyWd, int validKwFlags)
{
    int i;

    if (keyWd == NULL) return SYS_INTERNAL_NULL_INPUT_ERR;
    for (i = 0; i < NumDataObjInpKeyWd; i++) {
	if (strcmp (DataObjInpKeyWd[i].keyWd, keyWd) == 0) {
	    if ((DataObjInpKeyWd[i].flag & validKwFlags) == 0) {
		/* not valid */
		break;
	    } else {
		/* OK */
		return DataObjInpKeyWd[i].flag;
	    }
	}
    }
    return USER_BAD_KEYWORD_ERR;
}

int
parseMsKeyValStrForCollInp (msParam_t *inpParam, collInp_t *collInp,
char *hintForMissingKw, int validKwFlags, char **outBadKeyWd)
{
    char *msKeyValStr;
    keyValPair_t *condInput; 
    parsedMsKeyValStr_t parsedMsKeyValStr;
    int status;


    if (inpParam == NULL || collInp == NULL) {
        rodsLog (LOG_ERROR,
        "parseMsKeyValStrForCollInp: input inpParam or collInp is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if (strcmp(inpParam->type, STR_MS_T) != 0) return USER_PARAM_TYPE_ERR;

    msKeyValStr = (char *) inpParam->inOutStruct;

    condInput = &collInp->condInput;

    if (outBadKeyWd != NULL) *outBadKeyWd = NULL;

    if ((status = initParsedMsKeyValStr (msKeyValStr, &parsedMsKeyValStr)) < 0)
	return status;

    while (getNextKeyValFromMsKeyValStr (&parsedMsKeyValStr) >= 0) {
	if (parsedMsKeyValStr.kwPtr == NULL) {
	    if (hintForMissingKw == NULL) {
		status = NO_KEY_WD_IN_MS_INP_STR;
                rodsLogError (LOG_ERROR, status,
      		 "parseMsKeyValStrForCollInp: no keyWd for %s",
		  parsedMsKeyValStr.valPtr);
		clearParsedMsKeyValStr (&parsedMsKeyValStr);
        	return status;
	    } else if (strcmp (hintForMissingKw, KEY_WORD_KW) == 0) {
		/* XXXXX need to check if keywd is allowed */
		/* the value should be treated at keyWd */
		parsedMsKeyValStr.kwPtr = parsedMsKeyValStr.valPtr;
		parsedMsKeyValStr.valPtr = parsedMsKeyValStr.endPtr;
	    } else {
		/* use the input hintForMissingKw */
		parsedMsKeyValStr.kwPtr = hintForMissingKw;
	    }
	} 
        if ((status = chkCollInpKw (parsedMsKeyValStr.kwPtr,
          validKwFlags)) < 0) {
            if (outBadKeyWd != NULL) 
                *outBadKeyWd = strdup (parsedMsKeyValStr.kwPtr);
            return status;
        }
	/* check for some of the special keyWd */
	if (status == COLL_FLAGS_FLAG) {
	    collInp->flags = atoi (parsedMsKeyValStr.valPtr);
	    continue;
        } else if (status == OPR_TYPE_FLAG) {
	    collInp->oprType = atoi (parsedMsKeyValStr.valPtr);
            continue;
	} else if (status == COLL_NAME_FLAG) {
            rstrcpy (collInp->collName, parsedMsKeyValStr.valPtr,
	      MAX_NAME_LEN);
            continue;
	}
        addKeyVal (condInput, parsedMsKeyValStr.kwPtr, 
	  parsedMsKeyValStr.valPtr); 
    }

    clearParsedMsKeyValStr (&parsedMsKeyValStr);

    return 0;
}
 
int
chkCollInpKw (char *keyWd, int validKwFlags)
{
    int i;

    if (keyWd == NULL) return SYS_INTERNAL_NULL_INPUT_ERR;
    for (i = 0; i < NumCollInpKeyWd; i++) {
        if (strcmp (CollInpKeyWd[i].keyWd, keyWd) == 0) {
            if ((CollInpKeyWd[i].flag & validKwFlags) == 0) {
                /* not valid */
                break;
            } else {
                /* OK */
                return CollInpKeyWd[i].flag;            }
        }
    }
    return USER_BAD_KEYWORD_ERR;
}

int
addKeyValToMspStr (msParam_t *keyStr, msParam_t *valStr, 
msParam_t *msKeyValStr)
{
    int keyLen, valLen;
    char *valPtr, *keyPtr, *oldKeyValPtr, *newKeyValPtr, *tmpPtr;
    int oldLen, newLen;

    if ((keyStr == NULL && valStr == NULL) || msKeyValStr == NULL) 
	return SYS_INTERNAL_NULL_INPUT_ERR;

    if (msKeyValStr->type == NULL) fillStrInMsParam (msKeyValStr, NULL);

    keyPtr = parseMspForStr (keyStr);
    if (keyPtr == NULL || strcmp (keyPtr, MS_NULL_STR) == 0) {
	keyLen = 0;
    } else {
	keyLen = strlen (keyPtr);
    }

    valPtr = parseMspForStr (valStr);
    if (valPtr == NULL || strcmp (valPtr, MS_NULL_STR) == 0) {
        valLen = 0;
    } else {
        valLen = strlen (valPtr);
    }
    if (valLen + keyLen <= 0) return 0;

    oldKeyValPtr = parseMspForStr (msKeyValStr);
    if (oldKeyValPtr == NULL) {
	oldLen = 0;
	newLen = valLen + keyLen + 10;
	newKeyValPtr = (char *)malloc (newLen);
	*newKeyValPtr = '\0';
	tmpPtr = newKeyValPtr;
    } else {
	oldLen = strlen (oldKeyValPtr);
	newLen = oldLen + valLen + keyLen + 10;
	newKeyValPtr = (char *)malloc (newLen);
	snprintf (newKeyValPtr, newLen, "%s%s", oldKeyValPtr, MS_INP_SEP_STR);
	tmpPtr = newKeyValPtr + oldLen + 4;
	free (oldKeyValPtr);
    }

    if (keyLen > 0) {
	snprintf (tmpPtr, keyLen + 2, "%s=", keyPtr);
	tmpPtr += keyLen + 1;
    }
    if (valLen > 0) {
        snprintf (tmpPtr, valLen + 2, "%s", valPtr);
    }
    msKeyValStr->inOutStruct = (void *) newKeyValPtr;

    return 0;
}

int
parseMsKeyValStrForStructFileExtAndRegInp (msParam_t *inpParam, 
structFileExtAndRegInp_t *structFileExtAndRegInp,
char *hintForMissingKw, int validKwFlags, char **outBadKeyWd)
{
    char *msKeyValStr;
    keyValPair_t *condInput; 
    parsedMsKeyValStr_t parsedMsKeyValStr;
    int status;


    if (inpParam == NULL || structFileExtAndRegInp == NULL) {
        rodsLog (LOG_ERROR,
        "parseMsKeyValStrForStructFile:inpParam or structFileInp is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    if (strcmp(inpParam->type, STR_MS_T) != 0) return USER_PARAM_TYPE_ERR;

    msKeyValStr = (char *) inpParam->inOutStruct;

    condInput = &structFileExtAndRegInp->condInput;

    if (outBadKeyWd != NULL) *outBadKeyWd = NULL;

    if ((status = initParsedMsKeyValStr (msKeyValStr, &parsedMsKeyValStr)) < 0)
	return status;

    while (getNextKeyValFromMsKeyValStr (&parsedMsKeyValStr) >= 0) {
	if (parsedMsKeyValStr.kwPtr == NULL) {
	    if (hintForMissingKw == NULL) {
		status = NO_KEY_WD_IN_MS_INP_STR;
                rodsLogError (LOG_ERROR, status,
      		 "parseMsKeyValStrForStructFileExtAndRegInp: no keyWd for %s",
		  parsedMsKeyValStr.valPtr);
		clearParsedMsKeyValStr (&parsedMsKeyValStr);
        	return status;
	    } else if (strcmp (hintForMissingKw, KEY_WORD_KW) == 0) {
		/* the value should be treated at keyWd */
		parsedMsKeyValStr.kwPtr = parsedMsKeyValStr.valPtr;
		parsedMsKeyValStr.valPtr = parsedMsKeyValStr.endPtr;
	    } else {
		/* use the input hintForMissingKw */
		parsedMsKeyValStr.kwPtr = hintForMissingKw;
	    }
	} 
        if ((status = chkStructFileExtAndRegInpKw (parsedMsKeyValStr.kwPtr,
          validKwFlags)) < 0) {
            if (outBadKeyWd != NULL) 
                *outBadKeyWd = strdup (parsedMsKeyValStr.kwPtr);
            return status;
        }
	/* check for some of the special keyWd */
	if (status == COLL_FLAGS_FLAG) {
	    structFileExtAndRegInp->flags = atoi (parsedMsKeyValStr.valPtr);
	    continue;
        } else if (status == OPR_TYPE_FLAG) {
	    structFileExtAndRegInp->oprType = atoi (parsedMsKeyValStr.valPtr);
            continue;
        } else if (status == OBJ_PATH_FLAG) {
            rstrcpy (structFileExtAndRegInp->objPath, 
              parsedMsKeyValStr.valPtr, MAX_NAME_LEN);
            continue;
	} else if (status == COLL_NAME_FLAG) {
            rstrcpy (structFileExtAndRegInp->collection, 
	      parsedMsKeyValStr.valPtr, MAX_NAME_LEN);
            continue;
	}
        addKeyVal (condInput, parsedMsKeyValStr.kwPtr, 
	  parsedMsKeyValStr.valPtr); 
    }

    clearParsedMsKeyValStr (&parsedMsKeyValStr);

    return 0;
}

int
chkStructFileExtAndRegInpKw (char *keyWd, int validKwFlags)
{
    int i;

    if (keyWd == NULL) return SYS_INTERNAL_NULL_INPUT_ERR;
    for (i = 0; i < NumStructFileExtAndRegInpKeyWd; i++) {
        if (strcmp (StructFileExtAndRegInpKeyWd[i].keyWd, keyWd) == 0) {
            if ((StructFileExtAndRegInpKeyWd[i].flag & validKwFlags) 
	      == 0) {
                /* not valid */
                break;
            } else {
                /* OK */
                return StructFileExtAndRegInpKeyWd[i].flag;            }
        }
    }
    return USER_BAD_KEYWORD_ERR;
}

