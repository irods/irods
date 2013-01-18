/**
 * @file msoSrbMS.c
 *
 */

/*** Copyright (c), University of North Carolina            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "msoDriversMS.h"




#if defined(MSO_SRB)

#ifdef  __cplusplus
extern "C" {
#endif


#include "srbmso.h"

int
connectToRemotesrb(char * inStr, srbConn **rcComm)
{

  int  port;
  char *t, *s;  
  char *host = NULL;
  char *user = NULL;
  char *zone = NULL;
  char *pass = NULL;
  char *str;
  char portEnv[100];
  char userEnv[100];
  char domainEnv[100];
  
  /*inStr of form: //srb:host[:port][:user[@zone][:pass]]/remotePath
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
  else {
    free(str);
    return(USER_INPUT_FORMAT_ERR);
  }
  if (port == -1)
    port = 1247;
  if (pass != NULL) {
    /* do something */
    t++;
  }


  printf("MM: host=%s,port=%i,user=%s,domain=%s,pass=%s\n",host, port, user,zone,pass);

  sprintf(portEnv,"%i",port);
  sprintf(userEnv,"srbUser=%s",user);
  sprintf(domainEnv,"mdasDomainName=%s",zone);
  *rcComm = srbConnect (host, portEnv, pass,  user, zone, "ENCRYPT1" , NULL);
  if (clStatus(*rcComm) != CLI_CONNECTION_OK) {
    printf("Connection to SRB server failed.\n");
    clFinish(*rcComm);
    free( str ); // JMC cppcheck - leak
    return(REMOTE_SRB_CONNECT_ERR);
  }
  free( str ); // JMC cppcheck - leak
  return(0);
}  
#ifdef  __cplusplus
}
#endif

#endif /* MSO_SRB */


/**
 * \fn msiobjget_srb(msParam_t*  inRequestPath, msParam_t* inFileMode, msParam_t* inFileFlags, msParam_t* inCacheFilename,  ruleExecInfo_t* rei )
 *
 * \brief Gets an SRB object
 *
 * \module msoDrivers_srb
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
msiobjget_srb(msParam_t*  inRequestPath, msParam_t* inFileMode, 
	       msParam_t* inFileFlags, msParam_t* inCacheFilename,  
	       ruleExecInfo_t* rei )
{
#if defined(MSO_SRB)
  char *locStr;
  int mode, flags;
  char *cacheFilename; 
  char *str, *t;
  int status, bytesRead, bytesWritten;
  int destFd, i;
  srbConn *rcComm = NULL;

  int objFD;
  char locDir[MAX_NAME_LEN];
  char locFile[MAX_NAME_LEN];
  char buf[BUFSIZE];




  RE_TEST_MACRO( "    Calling msiobjget_srb" );
  
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

  /* Do the processing */

  i = connectToRemotesrb((char *) inRequestPath->inOutStruct, &rcComm);
  if (i < 0) {
    printf("msiputobj_srb: error connecting to remote srb: %s:%i\n",
           (char *) inRequestPath->inOutStruct,i);
    free(str);
    return(i);
  }


  splitPathByKey (locStr, locDir, locFile, '/');
  free(str);


  objFD = srbObjOpen (rcComm, locFile, O_RDONLY, locDir);

  if (objFD < 0) {
    printf("msigetobj_srb: Unable to open file %s/%s:%i\n", locDir, locFile, objFD);
    clFinish(rcComm);
    return(objFD);
  }

  destFd = open (cacheFilename, O_WRONLY | O_CREAT | O_TRUNC, mode);
  if (destFd < 0) {
    status = UNIX_FILE_OPEN_ERR - errno;
    printf (
	    "msigetobj_srb: open error for cacheFilename %s, status = %d",
	    cacheFilename, status);
    clFinish(rcComm);
            eirods::stacktrace st;
            st.trace();
            st.dump();
    return status;
  }

  while ((bytesRead = srbObjRead (rcComm, objFD, buf, BUFSIZE)) > 0) {
    bytesWritten = write (destFd, buf, bytesRead);
    if (bytesWritten != bytesRead ) {
      close (destFd);
      srbObjClose (rcComm, objFD);
      clFinish(rcComm);
      printf(
	     "msigetobj_srb: In Cache File %s bytesWritten %d != returned objLen %i\n",
	     cacheFilename, bytesWritten, bytesRead);
      return SYS_COPY_LEN_ERR;
    }
  }
  close (destFd);
  i = srbObjClose (rcComm, objFD);
  clFinish(rcComm);
  return(i);
#else
  return(MICRO_SERVICE_OBJECT_TYPE_UNDEFINED);
#endif /* MSO_SRB */

}





/**
 * \fn msiobjput_srb(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  msParam_t*  inFileSize, ruleExecInfo_t* rei )
 *
 * \brief Puts an SRB object
 *
 * \module msoDrivers_srb
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
msiobjput_srb(msParam_t*  inMSOPath, msParam_t*  inCacheFilename,  
	       msParam_t*  inFileSize, ruleExecInfo_t* rei )
{
#if defined(MSO_SRB)
  char *reqStr;
  char *str, *t;
  char *cacheFilename;
  srb_long_t dataSize;
  int status, i;
  int srcFd;
  char *myBuf;
  int bytesRead;
  int bytesWritten;
  int outDesc;
  srbConn *rcComm = NULL;
  char locDir[MAX_NAME_LEN];
  char locFile[MAX_NAME_LEN];
  int writeBufLen;


  RE_TEST_MACRO( "    Calling msiobjput_srb" );
  
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

  i = connectToRemotesrb((char *) inMSOPath->inOutStruct, &rcComm);
  if (i < 0) {
    printf("msiputobj_srb: error connecting to remote srb: %s:%i\n",
	   (char *) inMSOPath->inOutStruct,i);
    free(str);
    return(i);
  }

  /* Read the cache and Do the upload*/
  srcFd = open (cacheFilename, O_RDONLY, 0);
  if (srcFd < 0) {
    status = UNIX_FILE_OPEN_ERR - errno;
    printf ("msiputobj_srb: open error for %s, status = %d\n",
	    cacheFilename, status);
    free(str);
            eirods::stacktrace st;
            st.trace();
            st.dump();
    return status;
  }


  splitPathByKey (reqStr, locDir, locFile, '/');
  free(str);

  outDesc =  srbObjOpen (rcComm, locFile, O_RDWR|SRB_O_TRUNC, locDir);
  if (outDesc < 0) {
    printf ("msiputobj_srb: Unable to open file %s/%s:%i\n", locDir, locFile,outDesc);
    clFinish(rcComm);
	close( srcFd ); // JMC cppcheck - resource
    return(outDesc);
  }

  if (dataSize > MAX_SZ_FOR_SINGLE_BUF)
    writeBufLen = MAX_SZ_FOR_SINGLE_BUF;
  else
    writeBufLen = dataSize;

  myBuf = (char *) malloc (writeBufLen);

  while ( (bytesRead = read(srcFd, (void *) myBuf, writeBufLen)) > 0) {
    bytesWritten = srbObjWrite(rcComm, outDesc, myBuf, bytesRead);
    if (bytesWritten != bytesRead) {
      free(myBuf);
      close (srcFd);
      srbObjClose (rcComm, outDesc);
      clFinish(rcComm);
      printf ("msiputobj_srb: Write Error: bytesRead %d != bytesWritten %d\n",
	      bytesRead, bytesWritten);
      return SYS_COPY_LEN_ERR;
    }
  }  
  free(myBuf);
  close (srcFd);
  i = srbObjClose (rcComm, outDesc);
  clFinish(rcComm);

  return(i);
#else
  return(MICRO_SERVICE_OBJECT_TYPE_UNDEFINED);
#endif /* MSO_SRB */
}


