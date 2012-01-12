/**
 * @file msoIrodsMS.c
 *
 */

/*** Copyright (c), University of North Carolina            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file  msoIrodsMS.c
 *
 */


#include "rodsClient.h"
#include "msoDriversMS.h"
#include <strings.h>

int
connectToRemoteiRODS(char * inStr, rcComm_t **rcComm)
{
  int i, port;
  rErrMsg_t errMsg;
  char *t, *s;  
  char *host = NULL;
  char *user = NULL;
  char *zone = NULL;
  char *pass = NULL;
  char *str;
  
  /*inStr of form: //irods:host[:port][:user[@zone][:pass]]/remotePath
    if port is not give default pott 1247  is used
    if user@zone is not given ANONYMOUS_USER is used
    if pass is not given ANONYMOUS_USER is used
  */
  str = strdup(inStr);
  if ((t = strstr(str, "/")) == NULL){
    free(str);
    return(USER_INPUT_FORMAT_ERR);
  }
  else 
    *t = '\0';
  if ((t = strstr(str, ":")) == NULL) {
    free(str);
    return(USER_INPUT_FORMAT_ERR);
  }
  s = t+1;
  port = -1;
  host = s;
  while ((t = strstr(s, ":")) != NULL) {
    *t = '\0';
    s = t+1;
    if (port == -1 && user == NULL && isdigit(*s))
      port = atoi(s);
    else if (user == NULL)
      user = s;
    else if (pass == NULL){
      pass = s;
      break;
    }
  }


  if (user == NULL)
    strcpy(user, ANONYMOUS_USER);
  if ((t = strstr(user, "@")) != NULL) {
    *t = '\0';
    zone = t+1;
  }
  if (port == -1)
    port = 1247;
  if (pass != NULL) {
    /* do something */
    t++;
  }

  printf("MM: host=%s,port=%i,user=%s\n",host, port, user);
    
  *rcComm = rcConnect (host, port, user, zone, 0, &errMsg);
  if (*rcComm == NULL) {
    free( str ); // JMC cppcheck - leak
    return(REMOTE_IRODS_CONNECT_ERR);
  }
  i = clientLogin(*rcComm);
  if (i != 0) {
    rcDisconnect(*rcComm);
  }
  free( str ); // JMC cppcheck - leak
  return(i);
  
  
}  

/**
 * \fn msiobjget_irods(msParam_t*  inRequestPath, msParam_t* inFileMode, msParam_t* inFileFlags, msParam_t* inCacheFilename,  ruleExecInfo_t* rei )
 *
 * \brief Gets an iRODS object
 *
 * \module msoDrivers_irods
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
msiobjget_irods(msParam_t*  inRequestPath, msParam_t* inFileMode, 
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
  rcComm_t *rcComm = NULL;

  dataObjInp_t dataObjInp;
  int objFD;
  openedDataObjInp_t dataObjReadInp;
  openedDataObjInp_t dataObjCloseInp;
  bytesBuf_t readBuf;



  RE_TEST_MACRO( "    Calling msiobjget_irods" );
  
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
  if ((t = strstr(str, ":")) != NULL) {
    locStr = t+1;
    if ((t = strstr(locStr,"/"))!= NULL)
      locStr = t;
    else {
      free(str);
      return(USER_INPUT_FORMAT_ERR);
    }
  }

  else {
    free(str);
    return(USER_INPUT_FORMAT_ERR);
  }


  cacheFilename = (char *) inCacheFilename->inOutStruct;
  mode  = atoi((char *) inFileMode->inOutStruct);
  flags = atoi((char *) inFileFlags->inOutStruct);
  rsComm = rei->rsComm;

  /* Do the processing */

  i = connectToRemoteiRODS((char *) inRequestPath->inOutStruct, &rcComm);
  if (i < 0) {
    printf("msiputobj_irods: error connecting to remote iRODS: %s:%i\n",
           (char *) inRequestPath->inOutStruct,i);
    free(str);
    return(i);
  }


  bzero (&dataObjInp, sizeof (dataObjInp));
  bzero (&dataObjReadInp, sizeof (dataObjReadInp));
  bzero (&dataObjCloseInp, sizeof (dataObjCloseInp));
  bzero (&readBuf, sizeof (readBuf));

  dataObjInp.openFlags = O_RDONLY;
  rstrcpy(dataObjInp.objPath, locStr, MAX_NAME_LEN);
  free(str);

  objFD = rcDataObjOpen (rcComm, &dataObjInp);
  if (objFD < 0) {
    printf("msigetobj_irods: Unable to open file %s:%i\n", dataObjInp.objPath,objFD);
    rcDisconnect(rcComm);
    return(objFD);
  }

  destFd = open (cacheFilename, O_WRONLY | O_CREAT | O_TRUNC, mode);
  if (destFd < 0) {
    status = UNIX_FILE_OPEN_ERR - errno;
    printf (
	    "msigetobj_irods: open error for cacheFilename %s, status = %d",
	    cacheFilename, status);
    rcDisconnect(rcComm);
    return status;
  }

  dataObjReadInp.l1descInx = objFD;
  dataObjCloseInp.l1descInx = objFD;

  readBuf.len = MAX_SZ_FOR_SINGLE_BUF;
  readBuf.buf = (char *)malloc(readBuf.len);
  dataObjReadInp.len = readBuf.len;

  while ((bytesRead = rcDataObjRead (rcComm, &dataObjReadInp, &readBuf)) > 0) {
    bytesWritten = write (destFd, readBuf.buf, bytesRead);
    if (bytesWritten != bytesRead ) {
      free(readBuf.buf);
      close (destFd);
      rcDataObjClose (rcComm, &dataObjCloseInp);
      rcDisconnect(rcComm);
      printf(
	     "msigetobj_irods: In Cache File %s bytesWritten %d != returned objLen %i\n",
	     cacheFilename, bytesWritten, bytesRead);
      return SYS_COPY_LEN_ERR;
    }
  }
  free(readBuf.buf);
  close (destFd);
  i = rcDataObjClose (rcComm, &dataObjCloseInp);
  rcDisconnect(rcComm);
  return(i);

}





/**
 * \fn msiobjput_irods(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  msParam_t*  inFileSize, ruleExecInfo_t* rei )
 *
 * \brief Puts an iRODS object
 *
 * \module msoDrivers_irods
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
msiobjput_irods(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  
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
  rcComm_t *rcComm = NULL;


  RE_TEST_MACRO( "    Calling msiobjput_irods" );
  
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
  if ((t = strstr(str, ":")) != NULL) {
    reqStr = t+1;
    if ((t = strstr(reqStr,"/"))!= NULL)
      reqStr = t;
    else {
      free(str);
      return(USER_INPUT_FORMAT_ERR);
    }
  }
  else {
    free(str);
    return(USER_INPUT_FORMAT_ERR);
  }

  cacheFilename = (char *) inCacheFilename->inOutStruct;
  dataSize  = atol((char *) inFileSize->inOutStruct);
  rsComm = rei->rsComm;

  i = connectToRemoteiRODS((char *) inMSOPath->inOutStruct, &rcComm);
  if (i < 0) {
    printf("msiputobj_irods: error connecting to remote iRODS: %s:%i\n",
	   (char *) inMSOPath->inOutStruct,i);
    free(str);
    return(i);
  }

  /* Read the cache and Do the upload*/
  srcFd = open (cacheFilename, O_RDONLY, 0);
  if (srcFd < 0) {
    status = UNIX_FILE_OPEN_ERR - errno;
    printf ("msiputobj_irods: open error for %s, status = %d\n",
	    cacheFilename, status);
    free(str);
    rcDisconnect(rcComm);
    return status;
  }

  bzero (&dataObjInp, sizeof(dataObjInp_t));
  bzero(&dataObjWriteInp, sizeof (dataObjWriteInp));
  bzero(&dataObjCloseInp, sizeof (dataObjCloseInp));

  rstrcpy(dataObjInp.objPath, reqStr, MAX_NAME_LEN);
  addKeyVal (&dataObjInp.condInput, FORCE_FLAG_KW, "");
  free(str);

  outDesc = rcDataObjCreate (rcComm, &dataObjInp);
  if (outDesc < 0) {
    printf ("msiputobj_irods: Unable to open file %s:%i\n", dataObjInp.objPath,outDesc);
    rcDisconnect(rcComm);
    close(srcFd);
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
    bytesWritten = rcDataObjWrite(rcComm, &dataObjWriteInp, &writeBuf);
    if (bytesWritten != bytesRead) {
      free(myBuf);
      close (srcFd);
      rcDataObjClose (rcComm, &dataObjCloseInp);
      rcDisconnect(rcComm);
      printf ("msiputobj_irods: Write Error: bytesRead %d != bytesWritten %d\n",
	      bytesRead, bytesWritten);
      return SYS_COPY_LEN_ERR;
    }
  }  
  free(myBuf);
  close (srcFd);
  i = rcDataObjClose (rcComm, &dataObjCloseInp);
  rcDisconnect(rcComm);

  return(i);
}

