/**
 * @file msoDboMS.c
 *
 */

/*** Copyright (c), University of North Carolina            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file  msoDboMS.c
 *
 */

#include "rsApiHandler.h"
#include "msoDriversMS.h"
#include "dboHighLevelRoutines.h"


#define LOCAL_BUFFER_SIZE 1000000 /* check dboHighLevelRoutines.c */



/**
 * \fn msiobjget_dbo(msParam_t*  inRequestPath, msParam_t* inFileMode, msParam_t* inFileFlags, msParam_t* inCacheFilename,  ruleExecInfo_t* rei )
 *
 * \brief Gets a DBO object using microservice drivers
 *
 * \module msoDrivers_dbo
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
msiobjget_dbo(msParam_t*  inRequestPath, msParam_t* inFileMode, 
	       msParam_t* inFileFlags, msParam_t* inCacheFilename,  
	       ruleExecInfo_t* rei )
{
#if defined(DBR)
  char *locStr, *fileStr;
  int mode, flags;
  char *cacheFilename; 
  char *str, *t, *outBuf;
  int status, bytesWritten, objLen;
  int destFd, i;
  rsComm_t *rsComm;
  char *args[10];

  RE_TEST_MACRO( "    Calling msiobjget_dbo" );
  
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
  if ((t = strstr(locStr, ":")) == NULL) {
    free(str);
    return(USER_INPUT_FORMAT_ERR);
  }
  *t = '\0';
  fileStr = t+1;
  outBuf = (char *) malloc(LOCAL_BUFFER_SIZE);
  for (i = 0;i<10;i++) {
    args[i] = NULL;
  }
  /*printf("MM:locStr=%s,fileStr=%s\n", locStr, fileStr);*/
  i = dboExecute(rsComm, locStr, fileStr, (char *) NULL, 0, outBuf, LOCAL_BUFFER_SIZE,
		 args);
  if (i < 0) {
    free(outBuf);
    free(str);
    return(i);
  }
  objLen = strlen(outBuf);

  /* write the resulting buffer   in  cache file */
  destFd = open (cacheFilename, O_WRONLY | O_CREAT | O_TRUNC, mode);
  if (destFd < 0) {
    status = UNIX_FILE_OPEN_ERR - errno;
    printf (
	    "msigetobj_dbo: open error for cacheFilename %s, status = %d",
	    cacheFilename, status);
    free(outBuf);
    free(str);
    return status;
  }
  bytesWritten = write (destFd, outBuf, objLen);
  close (destFd);
  if (bytesWritten != objLen) {
    printf(
	   "msigetobj_dbo: In Cache File %s bytesWritten %d != returned objLen %i\n",
	   cacheFilename, bytesWritten, objLen);
    free(outBuf);
    free(str);
    return SYS_COPY_LEN_ERR;
  }
  
  /* clean up */
  free(outBuf);
  free(str);
  /*return */
  return(0);
#else
  return(DBR_NOT_COMPILED_IN);
#endif
}





/**
 * \fn msiobjput_dbo(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  msParam_t*  inFileSize, ruleExecInfo_t* rei )
 *
 * \brief Puts a DBO object
 *
 * \module msoDrivers_dbo
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
msiobjput_dbo(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  
	       msParam_t*  inFileSize, ruleExecInfo_t* rei )
{
#if defined(DBR)
  char *reqStr;
  char *cacheFilename;
  rodsLong_t dataSize;
  int status;
  int srcFd;
  char *myBuf;
  int bytesRead;


  RE_TEST_MACRO( "    Calling msiobjput_dbo" );
  
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
  reqStr = (char *) inMSOPath->inOutStruct;
  cacheFilename = (char *) inCacheFilename->inOutStruct;
  dataSize  = atol((char *) inFileSize->inOutStruct);



  /* Read the cache and Do the upload*/
  srcFd = open (cacheFilename, O_RDONLY, 0);
  if (srcFd < 0) {
    status = UNIX_FILE_OPEN_ERR - errno;
    printf ("msiputobj_dbo: open error for %s, status = %d\n",
	    cacheFilename, status);
    return status;
  }
  myBuf = (char *) malloc (dataSize);
  bytesRead = read (srcFd, (void *) myBuf, dataSize);
  close (srcFd);
  myBuf[dataSize-1] = '\0';
  if (bytesRead > 0 && bytesRead != dataSize) {
    printf ("msiputobj_dbo: bytesRead %d != dataSize %lld\n",
	    bytesRead, dataSize);
    free(myBuf);
    return SYS_COPY_LEN_ERR;
  }
  
  rodsLog(LOG_NOTICE,"MSO_DBO file contains: %s\n",myBuf);
  free(myBuf);
  return(0);
#else
  return(DBR_NOT_COMPILED_IN);
#endif

}

