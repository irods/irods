/**
 * @file msoSlinkMS.c
 *
 */

/*** Copyright (c), University of North Carolina            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file  msoSlinkMS.c
 *
 */

#include "msoDriversMS.h"
#include <strings.h>

extern int rsDataObjWrite (rsComm_t *rsComm, 
			   openedDataObjInp_t *dataObjWriteInp,
			   bytesBuf_t *dataObjWriteInpBBuf);
extern int rsDataObjRead (rsComm_t *rsComm, 
			  openedDataObjInp_t *dataObjReadInp,
			  bytesBuf_t *dataObjReadOutBBuf);  
extern int rsDataObjOpen (rsComm_t *rsComm, dataObjInp_t *dataObjInp);
extern int rsDataObjClose (rsComm_t *rsComm, openedDataObjInp_t *dataObjCloseInp);
extern int rsDataObjCreate (rsComm_t *rsComm, dataObjInp_t *dataObjInp);


/**
 * \fn msiobjget_slink(msParam_t*  inRequestPath, msParam_t* inFileMode, msParam_t* inFileFlags, msParam_t* inCacheFilename,  ruleExecInfo_t* rei )
 *
 * \brief Gets an SLINK object
 *
 * \module msoDrivers_slink
 *
 * \since 3.0
 *
 * \author  Arcot Rajasekar
 * \date    2011
 *
 * \usage See clients/icommands/test/rules3.0/ 
 *
 * \param[in] inRequestPath - a STR_MS_T request string to external resource
 * \param[in] inFileMode - a STR_MS_T mode of cache file creation
 * \param[in] inFileFlags - a STR_MS_T flags for cache file creation
 * \param[in] inCacheFilename - a STR_MS_T cache file name (full path)
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/



int 
msiobjget_slink(msParam_t*  inRequestPath, msParam_t* inFileMode, 
	       msParam_t* inFileFlags, msParam_t* inCacheFilename,  
	       ruleExecInfo_t* rei )
{

  char *locStr;
  int mode, flags;
  char *cacheFilename; 
  char *str, *t;
  int status, bytesRead, bytesWritten;
  int destFd, i;
  rsComm_t *rsComm;

  dataObjInp_t dataObjInp;
  int objFD;
  openedDataObjInp_t dataObjReadInp;
  openedDataObjInp_t dataObjCloseInp;
  bytesBuf_t readBuf;


  RE_TEST_MACRO( "    Calling msiobjget_slink" );
  
  /*  check for input parameters */
  if (inRequestPath ==  NULL || 
      strcmp(inRequestPath->type , STR_MS_T) != 0 || 
      inRequestPath->inOutStruct == NULL)
    return(USER_PARAM_TYPE_ERR);

  if (inFileMode ==  NULL || 
      strcmp(inFileMode->type , STR_MS_T) != 0 || 
      inFileMode->inOutStruct == NULL)
    return(USER_PARAM_TYPE_ERR);

  if (inFileFlags ==  NULL || 
      strcmp(inFileFlags->type , STR_MS_T) != 0 || 
      inFileFlags->inOutStruct == NULL)
    return(USER_PARAM_TYPE_ERR);

  if (inCacheFilename ==  NULL || 
      strcmp(inCacheFilename->type , STR_MS_T) != 0 || 
      inCacheFilename->inOutStruct == NULL)
    return(USER_PARAM_TYPE_ERR);

  /*  coerce input to local variables */
  str = strdup((char *) inRequestPath->inOutStruct);
  if ((t = strstr(str, ":")) != NULL)
    locStr = t+1;
  else {
    free(str);
    return(USER_INPUT_FORMAT_ERR);
  }


  cacheFilename = (char *) inCacheFilename->inOutStruct;
  mode  = atoi((char *) inFileMode->inOutStruct);
  flags = atoi((char *) inFileFlags->inOutStruct);
  rsComm = rei->rsComm;

  /* Do the processing */
  bzero (&dataObjInp, sizeof (dataObjInp));
  bzero (&dataObjReadInp, sizeof (dataObjReadInp));
  bzero (&dataObjCloseInp, sizeof (dataObjCloseInp));
  bzero (&readBuf, sizeof (readBuf));

  dataObjInp.openFlags = O_RDONLY;
  rstrcpy(dataObjInp.objPath, locStr, MAX_NAME_LEN);
  free(str);

  objFD = rsDataObjOpen (rsComm, &dataObjInp);
  if (objFD < 0) {
    printf("msigetobj_slink: Unable to open file %s:%i\n", dataObjInp.objPath,objFD);
    return(objFD);
  }

  destFd = open (cacheFilename, O_WRONLY | O_CREAT | O_TRUNC, mode);
  if (destFd < 0) {
    status = UNIX_FILE_OPEN_ERR - errno;
    printf (
	    "msigetobj_slink: open error for cacheFilename %s, status = %d",
	    cacheFilename, status);
    return status;
  }

  dataObjReadInp.l1descInx = objFD;
  dataObjCloseInp.l1descInx = objFD;

  readBuf.len = MAX_SZ_FOR_SINGLE_BUF;
  readBuf.buf = (char *)malloc(readBuf.len);
  dataObjReadInp.len = readBuf.len;

  while ((bytesRead = rsDataObjRead (rsComm, &dataObjReadInp, &readBuf)) > 0) {
    bytesWritten = write (destFd, readBuf.buf, bytesRead);
    if (bytesWritten != bytesRead ) {
      free(readBuf.buf);
      close (destFd);
      rsDataObjClose (rsComm, &dataObjCloseInp);
      printf(
	     "msigetobj_slink: In Cache File %s bytesWritten %d != returned objLen %i\n",
	     cacheFilename, bytesWritten, bytesRead);
      return SYS_COPY_LEN_ERR;
    }
  }
  free(readBuf.buf);
  close (destFd);
  i = rsDataObjClose (rsComm, &dataObjCloseInp);
  
  return(i);

}





/**
 * \fn msiobjput_slink(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  msParam_t*  inFileSize, ruleExecInfo_t* rei )
 *
 * \brief Puts an SLINK object
 *
 * \module msoDrivers_slink
 *
 * \since 3.0
 *
 * \author  Arcot Rajasekar
 * \date    2011
 *
 * \usage See clients/icommands/test/rules3.0/ 
 *
 * \param[in] inMSOPath - a STR_MS_T path string to external resource
 * \param[in] inCacheFilename - a STR_MS_T cache file containing data to be written out
 * \param[in] inFileSize - a STR_MS_T size of inCacheFilename
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/



int 
msiobjput_slink(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  
	       msParam_t*  inFileSize, ruleExecInfo_t* rei )
{

  char *reqStr;
  char *str, *t;
  char *cacheFilename;
  rodsLong_t dataSize;
  int status, i;
  int srcFd;
  char *myBuf;
  int bytesRead;
  openedDataObjInp_t dataObjWriteInp;
  int bytesWritten;
  openedDataObjInp_t dataObjCloseInp;
  dataObjInp_t dataObjInp;
  int outDesc;
  bytesBuf_t writeBuf;
  int writeBufLen;
  rsComm_t *rsComm;

  RE_TEST_MACRO( "    Calling msiobjput_slink" );
  
  /*  check for input parameters */
  if (inMSOPath ==  NULL || 
      strcmp(inMSOPath->type , STR_MS_T) != 0 || 
      inMSOPath->inOutStruct == NULL)
    return(USER_PARAM_TYPE_ERR);

  if (inCacheFilename ==  NULL ||
      strcmp(inCacheFilename->type , STR_MS_T) != 0 ||
      inCacheFilename->inOutStruct == NULL)
    return(USER_PARAM_TYPE_ERR);

  if (inFileSize ==  NULL ||
      strcmp(inFileSize->type , STR_MS_T) != 0 ||
      inFileSize->inOutStruct == NULL)
    return(USER_PARAM_TYPE_ERR);


  /*  coerce input to local variables */
  str = strdup((char *) inMSOPath->inOutStruct);
  if ((t = strstr(str, ":")) != NULL)
    reqStr = t+1;
  else {
    free(str);
    return(USER_INPUT_FORMAT_ERR);
  }

  cacheFilename = (char *) inCacheFilename->inOutStruct;
  dataSize  = atol((char *) inFileSize->inOutStruct);
  rsComm = rei->rsComm;

  /* Read the cache and Do the upload*/
  srcFd = open (cacheFilename, O_RDONLY, 0);
  if (srcFd < 0) {
    status = UNIX_FILE_OPEN_ERR - errno;
    printf ("msiputobj_slink: open error for %s, status = %d\n",
	    cacheFilename, status);
    free(str);
    return status;
  }

  bzero (&dataObjInp, sizeof(dataObjInp_t));
  bzero(&dataObjWriteInp, sizeof (dataObjWriteInp));
  bzero(&dataObjCloseInp, sizeof (dataObjCloseInp));

  rstrcpy(dataObjInp.objPath, reqStr, MAX_NAME_LEN);
  addKeyVal (&dataObjInp.condInput, FORCE_FLAG_KW, "");
  free(str);

  outDesc = rsDataObjCreate (rsComm, &dataObjInp);
  if (outDesc < 0) {
    printf ("msiputobj_slink: Unable to open file %s:%i\n", dataObjInp.objPath,outDesc);
	close( srcFd ); // JMC cppcheck - resource
    return(outDesc);
  }

  dataObjWriteInp.l1descInx = outDesc;
  dataObjCloseInp.l1descInx = outDesc;

  if (dataSize > MAX_SZ_FOR_SINGLE_BUF)
    writeBufLen = MAX_SZ_FOR_SINGLE_BUF;
  else
    writeBufLen = dataSize;

  myBuf = (char *) malloc (writeBufLen);
  writeBuf.buf = myBuf;

  while ( (bytesRead = read(srcFd, (void *) myBuf, writeBufLen)) > 0) {
    writeBuf.len = bytesRead;
    dataObjWriteInp.len = bytesRead;
    bytesWritten = rsDataObjWrite(rsComm, &dataObjWriteInp, &writeBuf);
    if (bytesWritten != bytesRead) {
      free(myBuf);
      close (srcFd);
      rsDataObjClose (rsComm, &dataObjCloseInp);
      printf ("msiputobj_slink: Write Error: bytesRead %d != bytesWritten %d\n",
	      bytesRead, bytesWritten);
      return SYS_COPY_LEN_ERR;
    }
  }  
  free(myBuf);
  close (srcFd);
  i = rsDataObjClose (rsComm, &dataObjCloseInp);

  return(i);
}

