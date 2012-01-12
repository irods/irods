/**
 * @file msoTestMS.c
 *
 */

/*** Copyright (c), University of North Carolina            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file  msoTestMS.c
 *
 */

#include "rsApiHandler.h"
#include "msoDriversMS.h"




/**
 * \fn msiobjget_test(msParam_t*  inRequestPath, msParam_t* inFileMode, msParam_t* inFileFlags, msParam_t* inCacheFilename,  ruleExecInfo_t* rei )
 *
 * \brief Test for getting a microservice object file
 *
 * \module msoDrivers_test
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
msiobjget_test(msParam_t*  inRequestPath, msParam_t* inFileMode, 
	       msParam_t* inFileFlags, msParam_t* inCacheFilename,  
	       ruleExecInfo_t* rei )
{

  char *reqStr;
  int mode, flags;
  char *cacheFilename; 
  char str[200];
  int status, bytesWritten, objLen;
  int destFd;


  RE_TEST_MACRO( "    Calling msiobjget_test" );
  
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
  reqStr = (char *) inRequestPath->inOutStruct;
  cacheFilename = (char *) inCacheFilename->inOutStruct;
  mode  = atoi((char*) inFileMode->inOutStruct);
  flags = atoi((char*) inFileFlags->inOutStruct);

  /* Do the accessing. In this case, we just create a string value */

  snprintf(str,199,"PID is %i. This is a test\n", getpid());
  objLen = strlen(str);


  /* write the resulting buffer   in  cache file */
  destFd = open (cacheFilename, O_WRONLY | O_CREAT | O_TRUNC, mode);
  if (destFd < 0) {
    status = UNIX_FILE_OPEN_ERR - errno;
    printf (
	    "msigetobj_test: open error for cacheFilename %s, status = %d",
	    cacheFilename, status);
    return status;
  }
  bytesWritten = write (destFd, str, objLen);
  close (destFd);
  if (bytesWritten != objLen) {
    printf(
	   "msigetobj_test: In Cache File %s bytesWritten %d != returned objLen %i\n",
	   cacheFilename, bytesWritten, objLen);
    return SYS_COPY_LEN_ERR;
  }
  
  /* clean up */

  /*return */
  return(0);
}





/**
 * \fn msiobjput_test(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  msParam_t*  inFileSize, ruleExecInfo_t* rei )
 *
 * \brief Test for putting a microservice object file
 *
 * \module msoDrivers_test
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
msiobjput_test(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  
	       msParam_t*  inFileSize, ruleExecInfo_t* rei )
{

  char *reqStr;
  char *cacheFilename;
  rodsLong_t dataSize;
  int status;
  int srcFd;
  char *myBuf;
  int bytesRead;


  RE_TEST_MACRO( "    Calling msiobjput_test" );
  
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
  dataSize  = atol((char *)inFileSize->inOutStruct);



  /* Read the cache and Do the upload*/
  /* In this case the file is read into myBuf and a 
     log message is written to the iRODSLog. */

  srcFd = open (cacheFilename, O_RDONLY, 0);
  if (srcFd < 0) {
    status = UNIX_FILE_OPEN_ERR - errno;
    printf ("msiputobj_test: open error for %s, status = %d\n",
	    cacheFilename, status);
    return status;
  }
  myBuf = (char *) malloc (dataSize);
  bytesRead = read (srcFd, (void *) myBuf, dataSize);
  close (srcFd);
  myBuf[dataSize-1] = '\0';
  if (bytesRead > 0 && bytesRead != dataSize) {
    printf ("msiputobj_test: bytesRead %d != dataSize %lld\n",
	    bytesRead, dataSize);
    free(myBuf);
    return SYS_COPY_LEN_ERR;
  }
  
  rodsLog(LOG_NOTICE,"MSO_TEST file contains: %s\n",myBuf);
  free(myBuf);
  return(0);
}

