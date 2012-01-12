/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* msoFileDriver.c - Driver to get/put objects using micro-service-based drivers
 */



#include "rsGlobalExtern.h"
#include "reGlobalsExtern.h"
#include "msoFileDriver.h"

int
msoFileUnlink (rsComm_t *rsComm, char *filename)
{
  return(0);

}

int
msoFileStat (rsComm_t *rsComm, char *filename, struct stat *statbuf)
{



    statbuf->st_mode = S_IFREG;
    statbuf->st_nlink = 1;
    statbuf->st_uid = getuid ();
    statbuf->st_gid = getgid ();
    statbuf->st_atime = statbuf->st_mtime = statbuf->st_ctime = time(0);
    statbuf->st_size = UNKNOWN_FILE_SZ;

  return 0;
}


rodsLong_t
msoFileGetFsFreeSpace (rsComm_t *rsComm, char *path, int flag)
{
  return(LARGE_SPACE);
}



/* msoStageToCache - 
 * Just copy the file from filename to cacheFilename. optionalInfo info
 * is not used.
 * 
 */
  
int
msoStageToCache (rsComm_t *rsComm, fileDriverType_t cacheFileType, 
int mode, int flags, char *msoObjName, 
char *cacheFilename,  rodsLong_t dataSize,
keyValPair_t *condInput)
{
    char *myMSICall;
    char *t;
    int i;
    ruleExecInfo_t rei2;
    msParamArray_t msParamArray;
    char callCode[100];

    if ((t = strstr(msoObjName, ":")) == NULL) {
      return(MICRO_SERVICE_OBJECT_TYPE_UNDEFINED);
    }
    *t = '\0';
    sprintf(callCode, "%s", msoObjName+2);
    *t = ':';
    myMSICall = (char *) malloc(strlen(msoObjName) + strlen(cacheFilename) + 200);
    sprintf(myMSICall, "msiobjget_%s(\"%s\",\"%i\",\"%i\",\"%s\")",callCode,msoObjName+2,mode,flags,cacheFilename);



    memset ((char*)&rei2, 0, sizeof (ruleExecInfo_t));
    memset ((char*)&msParamArray, 0, sizeof(msParamArray_t));

    
  rei2.rsComm = rsComm;
  if (rsComm != NULL) {
    rei2.uoic = &rsComm->clientUser;
    rei2.uoip = &rsComm->proxyUser;
  }

  i = applyRule(myMSICall, &msParamArray, &rei2, NO_SAVE_REI);


  if (i < 0) {
    if (rei2.status < 0) {
      i = rei2.status;
    }
    rodsLog (LOG_ERROR,
             "msoStageToCache: Error in rule for %s  error=%d",myMSICall,i);

  }
  free(myMSICall);
  clearMsParamArray(&msParamArray,0);
  return(i);

}

/* msoSyncToArch - 
 * Just copy the file from cacheFilename to filename. optionalInfo info
 * is not used.
 *
 */

int
msoSyncToArch (rsComm_t *rsComm, fileDriverType_t cacheFileType, 
int mode, int flags, char *msoObjName,
char *cacheFilename,  rodsLong_t dataSize, keyValPair_t *condInput)
{
    int status;
    struct stat statbuf;
    char *myMSICall;
    char *t;
    int i;
    ruleExecInfo_t rei2;
    msParamArray_t msParamArray;
    char callCode[100];

    status = stat (cacheFilename, &statbuf);

    if (status < 0) {
      status = UNIX_FILE_STAT_ERR - errno;
      printf ("msoSyncToArch: CacheFile stat of %s error, status = %d\n",
	      cacheFilename, status);
    }

    if ((statbuf.st_mode & S_IFREG) == 0) {
      status = UNIX_FILE_STAT_ERR - errno;
      printf (
	      "msoSyncToArch: %s is not a file, status = %d\n",
	      cacheFilename, status);
      return status;
    }

    if (dataSize > 0 && dataSize != statbuf.st_size) {
      printf (
	      "msoSyncToArch: %s inp size %lld does not match actual size %lld\n",
	      cacheFilename, (long long int) dataSize,
	      (long long int) statbuf.st_size);
      return SYS_COPY_LEN_ERR;
    }
    dataSize = statbuf.st_size;

    if ((t = strstr(msoObjName, ":")) == NULL) {
      return(MICRO_SERVICE_OBJECT_TYPE_UNDEFINED);
    }
    *t = '\0';
    sprintf(callCode, "%s", msoObjName+2);
    *t = ':';
    myMSICall = (char *) malloc(strlen(msoObjName) + strlen(cacheFilename) + 200);
    sprintf(myMSICall, "msiobjput_%s(\"%s\",\"%s\",\"%lld\")",callCode,msoObjName+2,cacheFilename,dataSize);



  memset ((char*)&rei2, 0, sizeof (ruleExecInfo_t));
  memset ((char*)&msParamArray, 0, sizeof(msParamArray_t));

  rei2.rsComm = rsComm;
  if (rsComm != NULL) {
    rei2.uoic = &rsComm->clientUser;
    rei2.uoip = &rsComm->proxyUser;
  }


  i = applyRule(myMSICall, &msParamArray, &rei2, NO_SAVE_REI);
  if (i < 0) {
    if (rei2.status < 0) {
      i = rei2.status;
    }
    rodsLog (LOG_ERROR,
	     "msoSyncToArch: Error in rule for %s  error=%d",myMSICall,i);
  }
  free(myMSICall);
  clearMsParamArray(&msParamArray,0);
  return(i);
}

