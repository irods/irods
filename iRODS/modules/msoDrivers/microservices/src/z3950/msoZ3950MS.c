/**
 * @file msoZ3950MS.c
 *
 */

/*** Copyright (c), University of North Carolina            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file  msoZ3950MS.c
 *
 */

#include "rsApiHandler.h"
#include "reGlobalsExtern.h"
#include "msoDriversMS.h"


int
getz3950Params(char *reqStr, char **locStr, char **queryStr, char **syntaxStr)
{
  char *r, *t, *q, *s;

  r = strdup(reqStr);
  if ((t = strstr(r,"?")) == NULL) {
    free(r);
    return(-1);
  }
  *t = '\0';
  *locStr = strdup(r);
  *t = '&';

  q = strstr(t, "&query=");
  s = strstr(t, "&recordsyntax=");

  if (q  != NULL)
    *q = '\0';;
  if (s  != NULL)
    *s = '\0';;

  if (q == NULL) {
    if (s != t) {
      *queryStr = strdup(t);
    }
    else {
      free(r);
      return(-2);
    }
  }
  else {
    q = q + 7;
    *queryStr = strdup(q);
  }
  if (s == NULL)
    *syntaxStr = strdup("XML");
  else {
    s = s + 14;
    *syntaxStr = strdup(s);
  }
  free(r);
  return(0);

}
/**
 * \fn msiobjget_z3950(msParam_t*  inRequestPath, msParam_t* inFileMode, msParam_t* inFileFlags, msParam_t* inCacheFilename,  ruleExecInfo_t* rei )
 *
 * \brief Gets an object from a Z3950 data source
 *
 * \module msoDrivers_z3950
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
msiobjget_z3950(msParam_t*  inRequestPath, msParam_t* inFileMode, 
	       msParam_t* inFileFlags, msParam_t* inCacheFilename,  
	       ruleExecInfo_t* rei )
{

  char *reqStr;
  int mode, flags;
  char *cacheFilename; 
  char *str;
  int status, bytesWritten, objLen;
  int destFd;
  char *locStr, *queryStr, *syntaxStr;
  char *myMSICall;
  int i;
  ruleExecInfo_t rei2;
  msParamArray_t msParamArray;
  msParam_t *mP;


  RE_TEST_MACRO( "    Calling msiobjget_z3950" );
  
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
  mode  = atoi((char *) inFileMode->inOutStruct);
  flags = atoi((char *) inFileFlags->inOutStruct);

  /* Do the processing */

  /* first skipping the mso-name */
  if ((str = strstr(reqStr,":")) == NULL)
    str = reqStr;
  else
    str++;

  /* getting parameters for the calls */
  i = getz3950Params(str, &locStr, &queryStr, &syntaxStr);
  if (i != 0) {
    printf("msiobjget_z3950: Error in request format for %s:%i\n",str,i);
    return(USER_INPUT_FORMAT_ERR);
  }

  myMSICall = (char *) malloc(strlen(reqStr) + 200);

  sprintf(myMSICall, "msiz3950Submit(\"%s\",\"%s\",\"%s\",*OutBuffer)",
	  locStr, queryStr, syntaxStr); 


  /*  sprintf(myMSICall, "msiz3950Submit(\"z3950.loc.gov:7090/Voyager\",\"@attr 1=1003 Marx\",\"USMARC\",*OutBuffer)" ); 
  printf("MM:%s***\n",myMSICall);
  */
  free(locStr);
  free(queryStr);
  free(syntaxStr);
  memset ((char*)&rei2, 0, sizeof (ruleExecInfo_t));
  memset ((char*)&msParamArray, 0, sizeof(msParamArray_t));


  rei2.rsComm = rei->rsComm;
  if (rei2.rsComm != NULL) {
    rei2.uoic = &rei2.rsComm->clientUser;
    rei2.uoip = &rei2.rsComm->proxyUser;
  }



  i = applyRule(myMSICall, &msParamArray, &rei2, NO_SAVE_REI);
  if (i != 0) {
    printf("msiobjget_z3950: Error in calling %s: %i\n", myMSICall, i);
    free(myMSICall);
    return(i);
  }
  free(myMSICall);
  mP = getMsParamByLabel(&msParamArray,"*OutBuffer");
  if (mP == NULL) {
    printf("msiobjget_z3950: Error in Getting Parameter after call for *OutBuffer\n");
    return(UNKNOWN_PARAM_IN_RULE_ERR);
  }
  str = (char *) mP->inOutStruct;
  objLen = strlen(str);

  /* write the resulting buffer   in  cache file */
  destFd = open (cacheFilename, O_WRONLY | O_CREAT | O_TRUNC, mode);
  if (destFd < 0) {
    status = UNIX_FILE_OPEN_ERR - errno;
    printf (
	    "msigetobj_z3950: open error for cacheFilename %s, status = %d",
	    cacheFilename, status);
    clearMsParamArray(&msParamArray,0);
    return status;
  }
  bytesWritten = write (destFd, str, objLen);
  close (destFd);
  clearMsParamArray(&msParamArray,0);
  if (bytesWritten != objLen) {
    printf(
	   "msigetobj_z3950: In Cache File %s bytesWritten %d != returned objLen %i\n",
	   cacheFilename, bytesWritten, objLen);
    return SYS_COPY_LEN_ERR;
  }
  
  /* clean up */

  /*return */
  return(0);
}





/**
 * \fn msiobjput_z3950(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  msParam_t*  inFileSize, ruleExecInfo_t* rei )
 *
 * \brief Puts an object into a z3950 server
 *
 * \module msoDrivers_z3950
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
msiobjput_z3950(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  
	       msParam_t*  inFileSize, ruleExecInfo_t* rei )
{

  char *reqStr;
  char *cacheFilename;
  rodsLong_t dataSize;
  int status;
  int srcFd;
  char *myBuf;
  int bytesRead;


  RE_TEST_MACRO( "    Calling msiobjput_z3950" );
  
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
    printf ("msiputobj_z3950: open error for %s, status = %d\n",
	    cacheFilename, status);
    return status;
  }
  myBuf = (char *) malloc (dataSize);
  bytesRead = read (srcFd, (void *) myBuf, dataSize);
  close (srcFd);
  myBuf[dataSize-1] = '\0';
  if (bytesRead > 0 && bytesRead != dataSize) {
    printf ("msiputobj_z3950: bytesRead %d != dataSize %lld\n",
	    bytesRead, dataSize);
    free(myBuf);
    return SYS_COPY_LEN_ERR;
  }
  
  rodsLog(LOG_NOTICE,"MSO_Z3950 file contains: %s\n",myBuf);
  free(myBuf);
  return(0);
}

