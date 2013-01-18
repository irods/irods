/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* hpssFileDriver.c - The HPSS file driver
 */


#include "hpssFileDriver.h"
#include "hpss_rpc.h"
#include "pdata.h"
#include "rsGlobalExtern.h"
#include "miscServerFunct.h"

struct hpssCosDef *HpssCosHead = NULL;
int HpssDefCos = COS_NOT_INIT;
int HpssAuthFlag = 0;
api_config_t      Api_config;

int
hpssFileUnlink (rsComm_t *rsComm, char *filename)
{
    int status;

    if ((status = initHpssAuth ()) < 0) return status;

    status = hpss_Unlink (filename);

    if (status < 0) {
        status = HPSS_FILE_UNLINK_ERR + status;
        rodsLog (LOG_ERROR, "hpssFileUnlink: unlink of %s error, status = %d",
         filename, status);
    }

    return (status);
}

int
hpssFileStat (rsComm_t *rsComm, char *filename, struct stat *statbuf)
{
    int status;
    hpss_stat_t hpssstat;

    if ((status = initHpssAuth ()) < 0) return status;
    status = hpss_Stat (filename, &hpssstat);

    if (status < 0) {
        status = HPSS_FILE_STAT_ERR + status;
        rodsLog (LOG_ERROR, "hpssFileStat: stat of %s error, status = %d",
         filename, status);
    } else {
        hpssStatToStat (&hpssstat, statbuf);
    }
    
    return (status);
}

int
hpssFileMkdir (rsComm_t *rsComm, char *filename, int mode)
{
    int status;

    if ((status = initHpssAuth ()) < 0) return status;
    status = hpss_Mkdir (filename, mode);

    if (status < 0) {
	if (status != HPSS_EEXIST)
            rodsLog (LOG_NOTICE,
              "hpssFileMkdir: mkdir of %s error, status = %d", 
	      filename, status);
        status = HPSS_FILE_MKDIR_ERR + status;
    }

    return (status);
}       

int
hpssFileChmod (rsComm_t *rsComm, char *filename, int mode)
{
    int status;

    if ((status = initHpssAuth ()) < 0) return status;
    status = hpss_Chmod (filename, mode);

    if (status < 0) {
        status = HPSS_FILE_CHMOD_ERR + status;
        rodsLog (LOG_ERROR,
          "hpssFileChmod: chmod of %s error, status = %d", 
          filename, status);
    }

    return (status);
}

int
hpssFileRmdir (rsComm_t *rsComm, char *filename)
{
    int status;

    if ((status = initHpssAuth ()) < 0) return status;
    status = hpss_Rmdir (filename);

    if (status < 0) {
        status = HPSS_FILE_RMDIR_ERR + status;
        rodsLog (LOG_ERROR,
          "hpssFileRmdir: rmdir of %s error, status = %d",
          filename, status);
    }

    return (status);
}

int
hpssFileOpendir (rsComm_t *rsComm, char *dirname, void **outDirPtr)
{
    int status;
    int *intPtr;

    if ((status = initHpssAuth ()) < 0) return status;
    status = hpss_Opendir (dirname);
    if (status < 0) {
        status = HPSS_FILE_OPENDIR_ERR + status;
        rodsLog (LOG_ERROR,
          "hpssFileOpendir: opendir of %s error, status = %d",
          dirname, status);
	return status;
    }

    intPtr = (int *) malloc (sizeof (int));
    *intPtr = status;
    *outDirPtr = (void *) intPtr;
    return (0);
}

int
hpssFileClosedir (rsComm_t *rsComm, void *dirPtr)
{
    int status;

    status = hpss_Closedir (* (int *) dirPtr);

    if (status < 0) {
        status = HPSS_FILE_CLOSEDIR_ERR + status;
        rodsLog (LOG_ERROR, 
          "hpssFileClosedir: closedir error, status = %d", status);
    }
    return (status);
}

int
hpssFileReaddir (rsComm_t *rsComm, void *dirPtr, struct dirent *direntPtr)
{
    int status;
    int dirdesc;

    dirdesc = *(int *) dirPtr;
    status = hpss_Readdir (dirdesc, (hpss_dirent_t *) direntPtr);

    if (status < 0) {
        status = HPSS_FILE_READDIR_ERR + status;
         rodsLog (LOG_ERROR,
           "hpssFileReaddir: readdir error, status = %d", status);
    }
    return status;
}

int
hpssFileRename (rsComm_t *rsComm, char *oldFileName, char *newFileName)
{
    int status;

    if ((status = initHpssAuth ()) < 0) return status;

    status = hpss_Rename (oldFileName, newFileName);

    if (status < 0) {
        status = HPSS_FILE_RENAME_ERR + status;
        rodsLog (LOG_ERROR,
         "hpssFileRename: rename error, status = %d\n",
         status);
    }

    return (status);
}

rodsLong_t
hpssFileGetFsFreeSpace (rsComm_t *rsComm, char *path, int flag)
{
    int space = LARGE_SPACE;
    return (space * 1024 * 1024);
}

/* hpssStageToCache - This routine is for testing the TEST_STAGE_FILE_TYPE.
 * Just copy the file from filename to cacheFilename. optionalInfo info
 * is not used.
 * 
 */
  
int
hpssStageToCache (rsComm_t *rsComm, fileDriverType_t cacheFileType, 
int mode, int flags, char *filename, 
char *cacheFilename,  rodsLong_t dataSize,
keyValPair_t *condInput)
{
    int status;

    struct stat statbuf;
    rodsLong_t mySize;

    if ((status = initHpssAuth ()) < 0) return status;

    status = hpssFileStat (NULL, filename, &statbuf);

    if (status < 0 || (statbuf.st_mode & S_IFREG) == 0) {
        status = HPSS_FILE_STAT_ERR + status;
        rodsLog (LOG_ERROR, "hpssStageToCache: stat of %s error, status = %d",
         filename, status);
        return status;
    }


    if (dataSize > 0 && dataSize != statbuf.st_size) {
        rodsLog (LOG_ERROR,
          "hpssStageToCache: %s inp dataSize %lld does not match size %lld",
         filename, dataSize, statbuf.st_size);
        return SYS_COPY_LEN_ERR;
    }
    mySize = statbuf.st_size;

    if (dataSize > MAX_SZ_FOR_SINGLE_BUF) {
        status = paraHpssGet (filename, cacheFilename, mode, flags, mySize);
    } else {
        status = seqHpssGet (filename, cacheFilename, mode, flags, mySize);
    }
    return status;

}

/* hpssSyncToArch - This routine is for testing the TEST_STAGE_FILE_TYPE.
 * Just copy the file from cacheFilename to filename. optionalInfo info
 * is not used.
 *
 */

int
hpssSyncToArch (rsComm_t *rsComm, fileDriverType_t cacheFileType, 
int mode, int flags, char *filename,
char *cacheFilename,  rodsLong_t dataSize, keyValPair_t *condInput)
{
    int status;
    struct stat statbuf;

    if ((status = initHpssAuth ()) < 0) return status;

    status = stat (cacheFilename, &statbuf);

    if (status < 0) {
        status = UNIX_FILE_STAT_ERR - errno;
        rodsLog (LOG_ERROR, "hpssSyncToArch: stat of %s error, status = %d",
         cacheFilename, status);
        return status;
    }

    if ((statbuf.st_mode & S_IFREG) == 0) {
        status = UNIX_FILE_STAT_ERR - errno;
        rodsLog (LOG_ERROR, "hpssSyncToArch: %s is not a file, status = %d",
         cacheFilename, status);
        return status;
    }

    if (dataSize > 0 && dataSize != statbuf.st_size) {
        rodsLog (LOG_ERROR,
          "hpssSyncToArch: %s inp size %lld does not match actual size %lld",
         cacheFilename, dataSize, statbuf.st_size);
        return SYS_COPY_LEN_ERR;
    }
    dataSize = statbuf.st_size;

    if (dataSize > MAX_SZ_FOR_SINGLE_BUF) {
        status = paraHpssPut (cacheFilename, filename, mode, flags, dataSize);
    } else {
        status = seqHpssPut (cacheFilename, filename, mode, flags, dataSize);
    }

    return status;
}

int
seqHpssPut (char *srcUnixFile, char *destHpssFile, int mode, int flags,
rodsLong_t mySize)
{
    int srcFd, destFd;
    char myBuf[TRANS_BUF_SZ];
    rodsLong_t bytesCopied = 0;
    int bytesRead;
    int status;

    srcFd = open (srcUnixFile, O_RDONLY, 0);
    if (srcFd < 0) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog (LOG_ERROR,
         "seqHpssPut: open error for srcUnixFile %s, status = %d",
         srcUnixFile, status);
        return status;
    }

    destFd = hpssOpenForWrite (destHpssFile, mode, flags, mySize);
    if (destFd < 0) {
	close (srcFd);
	return destFd;
    }

    while ((bytesRead = read (srcFd, (void *) myBuf, TRANS_BUF_SZ)) > 0) {
	int left, written;
	char *bufptr;

	left = bytesRead;
	bufptr = myBuf;
        while (left > 0) {
            written = hpss_Write (destFd, bufptr, left);
            if (written < 0) {
		status = HPSS_FILE_WRITE_ERR + written;
        	rodsLog (LOG_ERROR,
		  "seqHpssPut: hpss_Write err for %s, status = %d",
        	   destHpssFile, status);
		close (srcFd);
		hpss_Close (destFd);
        	return status;
            }
            left -= written;
            bufptr += written;
	}
        bytesCopied += bytesRead;
    }

    if (mySize != bytesCopied) {
        rodsLog (LOG_ERROR, 
          "seqHpssPut: %s bytesCopied %lld does not match actual size %lld",
           srcUnixFile, bytesCopied, mySize);
        status = SYS_COPY_LEN_ERR;
    } else {
	status = 0;
    }
    close (srcFd);
    hpss_Close (destFd);

    return status;
}

int
seqHpssGet (char *srcHpssFile, char *destUnixFile, int mode, int flags,
rodsLong_t dataSize)
{
    int srcFd, destFd;
    char myBuf[TRANS_BUF_SZ];
    rodsLong_t bytesCopied = 0;
    int bytesRead, bytesWritten;
    int status;

    srcFd = hpssOpenForRead (srcHpssFile, O_RDONLY);
    if (srcFd < 0) {
        status = HPSS_FILE_OPEN_ERR + srcFd;
        rodsLog (LOG_ERROR,
         "seqHpssGet: hpssOpenForRead error for srcHpssFile %s, status = %d",
         srcHpssFile, status);
        return status;
    }

    destFd = open (destUnixFile, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (destFd < 0) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog (LOG_ERROR,
         "hpssFileCopy: open error for destUnixFile %s, status = %d",
         destUnixFile, status);
        hpss_Close (srcFd);
        return status;
    }

    while ((bytesRead = hpss_Read (srcFd, (void *) myBuf, TRANS_BUF_SZ)) > 0) {
        bytesWritten = write (destFd, (void *) myBuf, bytesRead);
        if (bytesWritten <= 0) {
            status = UNIX_FILE_WRITE_ERR - errno;
            rodsLog (LOG_ERROR,
             "seqHpssGet: write error for destUnixFile %s, status = %d",
             destUnixFile, status);
            hpss_Close (srcFd);
            close (destFd);
            return status;
        }
        bytesCopied += bytesWritten;
    }

    if (dataSize != bytesCopied) {
        rodsLog (LOG_ERROR, 
          "seqHpssPut: %s bytesCopied %lld does not match actual size %lld",
           srcHpssFile, bytesCopied, dataSize);
        status = SYS_COPY_LEN_ERR;
    } else {
	status = 0;
    }

    hpss_Close (srcFd);
    close (destFd);

    return status;
}

int
hpssOpenForWrite (char *destHpssFile, int mode, int flags, rodsLong_t dataSize)
{
    hpss_cos_hints_t  HintsIn;
    hpss_cos_priorities_t HintsPri;
    hpss_cos_hints_t  HintsOut;
    int myCos;
    int destFd;
    int myFlags;
    int status;

    if ((status = initHpssAuth ()) < 0) return status;

    myFlags = O_CREAT | O_TRUNC | O_RDWR;
    (void) hpss_Umask((mode_t) 0000);
    myCos = getHpssCos (dataSize);

    if (myCos < 0) {
	/* don't have a valid cos */
	if (dataSize > 0) {
	    memset(&HintsIn,  0, sizeof HintsIn);
            memset(&HintsPri, 0, sizeof HintsPri);
	    CONVERT_LONGLONG_TO_U64(dataSize, HintsIn.MinFileSize);
	    HintsIn.MaxFileSize = HintsIn.MinFileSize;
	    HintsPri.MaxFileSizePriority = HintsPri.MinFileSizePriority = 
	     REQUIRED_PRIORITY;
	    destFd = hpss_Open (destHpssFile, myFlags, mode, &HintsIn, 
	      &HintsPri, NULL);
	} else {
	    destFd = hpss_Open (destHpssFile, myFlags, mode, NULL, NULL, NULL);
	}
    } else {
	memset(&HintsIn,  0, sizeof HintsIn);
        memset(&HintsPri, 0, sizeof HintsPri);
        memset(&HintsOut, 0, sizeof HintsOut);
        HintsIn.COSId = myCos; 
	HintsPri.COSIdPriority = REQUIRED_PRIORITY; 
	destFd = hpss_Open (destHpssFile, myFlags, mode, &HintsIn ,&HintsPri,
          &HintsOut);
    }

    if (destFd < 0) {
	if (destFd != HPSS_ENOENT) {
            char *errorText;
            errorText = hpss_RPCGetLastErrorText();
            if (errorText != NULL) {
                rodsLog (LOG_ERROR,
                  "hpssOpenForWrite: error text = %s", errorText);
            }
            rodsLog (LOG_ERROR,
             "hpssOpenForWrite: hpss_Open error, status = %d\n",
             destFd);
	}
        destFd = HPSS_FILE_OPEN_ERR + destFd;
    }
    return destFd;
}

int
hpssOpenForRead (char *srcHpssFile, int flags)
{
    int srcFd;
    int myFlags;

    int status;

    if ((status = initHpssAuth ()) < 0) return status;

    myFlags = O_RDONLY;
    srcFd = hpss_Open (srcHpssFile, myFlags, 0, NULL, NULL, NULL);

    if (srcFd < 0) {
        if (srcFd != HPSS_ENOENT) {
            rodsLog (LOG_ERROR,
             "hpssOpenForRead: hpss_Open error, status = %d\n", srcFd);
        }
        srcFd = HPSS_FILE_OPEN_ERR + srcFd;
    }
    return srcFd;
}

int
hpssStatToStat (hpss_stat_t *hpssstat, struct stat *statbuf)
{

    memset (statbuf, 0, sizeof (struct stat));
    statbuf->st_dev = hpssstat->st_dev;
    statbuf->st_mode = hpssstat->st_mode;
    statbuf->st_nlink = hpssstat->st_nlink;
    statbuf->st_uid = hpssstat->st_uid;
    statbuf->st_gid = hpssstat->st_gid;
    statbuf->st_rdev = hpssstat->st_rdev;
    CONVERT_U64_TO_LONGLONG(hpssstat->st_size, statbuf->st_size);
    /* memcpy (&statbuf->st_size, & hpssstat->st_size, sizeof (rodsLong_t)); */
    statbuf->st_atime = hpssstat->st_atime_n;
    statbuf->st_mtime = hpssstat->st_mtime_n;
    statbuf->st_ctime = hpssstat->st_ctime_n;
    statbuf->st_blksize = hpssstat->st_blksize;
    statbuf->st_blocks = hpssstat->st_blocks;
#if defined(aix_platform)
    statbuf->st_vfstype = hpssstat->st_vfstype;
    statbuf->st_vfs = hpssstat->st_vfs;
    statbuf->st_type = hpssstat->st_type;
    statbuf->st_flag = hpssstat->st_flag;
#endif 	/* aix_platform */
#if defined(aix_platform) || defined(alpha_platform)
    statbuf->st_gen = hpssstat->st_gen;
#endif  /* defined(aix_platform) || defined(alpha_platform) */
    return (0);
}

int
initHpssAuth ()
{
    char hpssUser[MAX_NAME_LEN], hpssAuthInfo[MAX_NAME_LEN];
    int status;
    hpss_authn_mech_t mech_type;
    const char * mech_name;

    if (HpssAuthFlag) return 0;

    HpssAuthFlag = 1;
    
    if ((status = readHpssAuthInfo (hpssUser, hpssAuthInfo)) < 0) {
        rodsLog (LOG_ERROR,
          "initHpssAuth: readHpssAuthInfo error. status = %d", status);
	return status;
    }

   /* Set the authentication type to use
     */

    status = hpss_GetConfiguration(&Api_config);
    if(status != 0) {
        rodsLog (LOG_ERROR,
          "initHpssAuth: hpss_GetConfiguration error. status = %d", status);
        return (status + HPSS_AUTH_ERR);
    }

#ifdef HPSS_KRB5_AUTH
    Api_config.AuthnMech = hpss_authn_mech_krb5;
#else
    Api_config.AuthnMech = hpss_authn_mech_unix;
#endif
    Api_config.Flags |= API_USE_CONFIG;

    status = hpss_SetConfiguration(&Api_config);
    if(status != 0) {
        rodsLog (LOG_ERROR,
          "initHpssAuth: hpss_SetConfiguration error. status = %d", status);
	return (status + HPSS_AUTH_ERR);
    }

#ifdef HPSS_KRB5_AUTH
    mech_name = "krb5";
#else
    mech_name = "unix";
#endif
    status = hpss_AuthnMechTypeFromString(mech_name, &mech_type);
    if(status != 0) {
	rodsLog (LOG_ERROR,
          "initHpssAuth: invalid authentication type %s", mech_name);
        status = HPSS_AUTH_NOT_SUPPORTED + status;
	return status;
    }
#ifdef HPSS_UNIX_PASSWD_AUTH
    status = hpss_SetLoginCred (hpssUser, mech_type, hpss_rpc_cred_client,
      hpss_rpc_auth_type_passwd, hpssAuthInfo);
    memset (hpssAuthInfo, 0, MAX_NAME_LEN);
    if(status != 0) {
	rodsLog (LOG_ERROR,
          "initHpssAuth: hpss_SetLoginCred error,stat=%d.hpssUser=%s,errno=%d",
          status, hpssUser, errno);
        status = HPSS_AUTH_ERR - errno;
	return status;
    }
#else	/* use keytab. default */
    status = hpss_SetLoginCred(hpssUser, mech_type, hpss_rpc_cred_client,
      hpss_rpc_auth_type_keytab, hpssAuthInfo);
    if(status != 0) {
	char *errorText;
	errorText = hpss_RPCGetLastErrorText();
	if (errorText != NULL) {
	    rodsLog (LOG_ERROR,
              "hpss_SetLoginCred: error text = %s", errorText);
	} 
        rodsLog (LOG_ERROR,
          "initHpssAuth: hpss_SetLoginCred err,stat=%d.User=%s,Info=%s,err=%d",
          status, hpssUser, hpssAuthInfo, errno);
        status = HPSS_AUTH_ERR - errno;
        return status;
    }
#endif
    return status;
}

int
readHpssAuthInfo (char *hpssUser, char *hpssAuthInfo)
{
    FILE *fptr;
    char hpssAuthFile[MAX_NAME_LEN];
    char inbuf[MAX_NAME_LEN];
    int lineLen, bytesCopied;
    int linecnt = 0;

    snprintf (hpssAuthFile, MAX_NAME_LEN, "%-s/%-s", 
      getConfigDir(), HPSS_AUTH_FILE);

    fptr = fopen (hpssAuthFile, "r");

    if (fptr == NULL) {
        rodsLog (LOG_ERROR,
          "readHpssAuthInfo: open HPSS_AUTH_FILE file %s err. ernro = %d",
          hpssAuthFile, errno);
        return (SYS_CONFIG_FILE_ERR);
    }
    while ((lineLen = getLine (fptr, inbuf, MAX_NAME_LEN)) > 0) {
        char *inPtr = inbuf;
	if (linecnt == 0) {
            while ((bytesCopied = getStrInBuf (&inPtr, hpssUser,
              &lineLen, LONG_NAME_LEN)) > 0) {
	        linecnt ++;
		break;
	    }
	} else if (linecnt == 1) {
            while ((bytesCopied = getStrInBuf (&inPtr, hpssAuthInfo,
              &lineLen, LONG_NAME_LEN)) > 0) {
                linecnt ++;
                break;
            }
	}
    }
    if (linecnt != 2)  {
        rodsLog (LOG_ERROR,
          "readHpssAuthInfo: read %d lines in HPSS_AUTH_FILE file",
          linecnt);
        return (SYS_CONFIG_FILE_ERR);
    }
    return 0;
}

int
initHpssCos ()
{
    FILE *fptr;
    char hpssCosFile[MAX_NAME_LEN];
    char inbuf[MAX_NAME_LEN];
    char strbuf[LONG_NAME_LEN];
    int lineLen, bytesCopied;
    int strcnt;
    hpssCosDef_t *tmpHpssCos = NULL;

    if (HpssDefCos != COS_NOT_INIT) return 0;

    snprintf (hpssCosFile, MAX_NAME_LEN, "%-s/%-s",
      getConfigDir(), HPSS_COS_CONFIG_FILE);

    fptr = fopen (hpssCosFile, "r");

    if (fptr == NULL) {
        rodsLog (LOG_ERROR,
          "initHpssCos: open HPSS_COS_CONFIG_FILE file %s err. ernro = %d",
          hpssCosFile, errno);
	HpssDefCos = NO_DEF_COS;
        return (SYS_CONFIG_FILE_ERR);
    }
    while ((lineLen = getLine (fptr, inbuf, MAX_NAME_LEN)) > 0) {
        char *inPtr = inbuf;
	strcnt = 0;
        while ((bytesCopied = getStrInBuf (&inPtr, strbuf, &lineLen, 
	  LONG_NAME_LEN)) > 0) {
	    if (strcnt == 0) {
		/* first string */
		tmpHpssCos =  malloc (sizeof(hpssCosDef_t));
		bzero (tmpHpssCos, sizeof sizeof(hpssCosDef_t));
		tmpHpssCos->cos = atoi (strbuf);
	    } else if (strcnt == 1) {
		tmpHpssCos->maxSzInKByte = strtoll (strbuf, 0, 0);
	    } else {
		if (strcmp (strbuf, DEF_COS_KW) == 0) {
		    HpssDefCos = tmpHpssCos->cos;
		}
	    }
	    strcnt++;
	}
	if (tmpHpssCos != NULL) {
	    if (strcnt <= 1) {
		rodsLog (LOG_ERROR,
          	  "initHpssCos: input cos %d has no other entries", 
		  tmpHpssCos->cos);
		free (tmpHpssCos);
	    } else {
		queCos (tmpHpssCos);
	    }
	    tmpHpssCos = NULL;
	}
    }

    if (HpssDefCos == COS_NOT_INIT) HpssDefCos = NO_DEF_COS;

    return 0;
}

int
queCos (hpssCosDef_t *myHpssCos)
{
    hpssCosDef_t *lastHpssCos = NULL;
    hpssCosDef_t *tmpHpssCos;

    if (HpssCosHead == NULL) {
        HpssCosHead = myHpssCos;
        HpssCosHead->next = NULL;
        return 0;
    }
    tmpHpssCos = HpssCosHead;
    while (tmpHpssCos != NULL) {
        if (tmpHpssCos->maxSzInKByte > myHpssCos->maxSzInKByte)
            break;
        lastHpssCos = tmpHpssCos;
        tmpHpssCos = lastHpssCos->next;
    }
    if (lastHpssCos == NULL) {
        myHpssCos->next = HpssCosHead;
        HpssCosHead = myHpssCos;
    } else {
        myHpssCos->next = lastHpssCos->next;
        lastHpssCos->next = myHpssCos;
    }
    return (0);
}

int
getHpssCos (rodsLong_t fileSize)
{
    hpssCosDef_t *tmpHpssCos;
    int lastCos = -1;

    initHpssCos ();
    
    if (HpssCosHead == NULL || fileSize < 0) return HpssDefCos;

    tmpHpssCos = HpssCosHead;

    while (tmpHpssCos != NULL) {
        if ((fileSize / 1024) < tmpHpssCos->maxSzInKByte)
            return (tmpHpssCos->cos);
        lastCos = tmpHpssCos->cos;
        tmpHpssCos = tmpHpssCos->next;
    }
    if (lastCos >= 0) return lastCos;
    return HpssDefCos;
}

int
paraHpssPut (char *srcUnixFile, char *destHpssFile, int mode, int flags,
rodsLong_t mySize)
{
    int destFd;
    int status;
    hpssSession_t myHpssSession;
    hpss_IOD_t      iod;         	/* IOD passed to hpss_WriteList */
    hpss_IOR_t      ior;         	/* IOR returned from hpss_WriteList */
    iod_srcsinkdesc_t   srcDesc, sinkDesc;  /* IOD source/sink descriptors */

    status = initHpssSession (&myHpssSession, HPSS_PUT_OPR, 
      srcUnixFile, mySize, mode);
    if (status < 0) return status;

    destFd = hpssOpenForWrite (destHpssFile, mode, flags, mySize);
    if (destFd < 0) {
        return destFd;
    }

    status = createControlSocket (&myHpssSession);
    if (status < 0) return status;

    pthread_create(&myHpssSession.moverConnManagerThr, pthread_attr_default,
      (void *(*)(void *)) moverConnManager,
      (void *) &myHpssSession);

#if !defined(solaris_platform)
    sched_yield(); 
#endif
   initHpssIodForWrite (&iod, &srcDesc, &sinkDesc, destFd, &myHpssSession);

    bzero (&ior, sizeof(ior));
    status = hpss_WriteList (&iod, 0, &ior);
    if (status != 0) {
        if (ior.Status != HPSS_E_NOERROR) {
	    status = HPSS_WRITE_LIST_ERR + ior.Status;
            rodsLog (LOG_ERROR,
              "paraHpssPut: hpss_WriteList error for %s. ior ststus = %d",
               destHpssFile, status);
             myHpssSession.status = status;
        } else if (myHpssSession.status == HPSS_E_NOERROR) {
             rodsLog (LOG_ERROR,
               "paraHpssPut: hpss_WriteList error for %s. ststus = %d",
                destHpssFile, status);
             myHpssSession.status = HPSS_WRITE_LIST_ERR;
	}
    }
    hpss_Close(destFd);

    status = postProcSessionThr (&myHpssSession, destHpssFile);
    return status;
}

int
initHpssSession (hpssSession_t *hpssSession, int operation, char *unixFilePath,
rodsLong_t fileSize, int createMode)
{
    struct hostent *hostEntry;
    char *hostname;
    char myhostname[NAME_LEN];
    
    bzero (hpssSession, sizeof (hpssSession_t));
    hpssSession->fileSize64 = cast64m (fileSize);
    hpssSession->operation = operation;
    hpssSession->unixFilePath = unixFilePath;
    hpssSession->createFlag = 0;
    hpssSession->createMode = createMode;
    hpssSession->status = HPSS_E_NOERROR;
    hpssSession->requestId = getpid();

    if ((hostname = getenv("IRODS_HPSS_HOSTNAME")) == NULL &&
      (hostname = getLocalSvrAddr ()) == NULL) { 
         if (gethostname (myhostname, NAME_LEN) < 0) {
             rodsLog (LOG_ERROR,
               "initHpssSession: gethostname error, errno=%d", errno);
             return (SYS_INVALID_SERVER_HOST - errno);
	} else {
	    hostname = myhostname;
	}
    }
    hostEntry = gethostbyname (hostname);
    if (hostEntry == NULL) {
         rodsLog (LOG_ERROR,
           "initHpssSession: gethostbyname of %s error, errno=%d",
            hostname, errno);
         return (SYS_INVALID_SERVER_HOST - errno);
    }
    hpssSession->ipAddr = *((unsigned32 *) hostEntry->h_addr_list[0]);
    pthread_mutex_init (&hpssSession->myMutex, pthread_mutexattr_default);
    return (0);
}

/* createControlSocket - Create the  control socket where all movers will 
 * connect to. */
int
createControlSocket (hpssSession_t *hpssSession) 
{
    int status, myLen;

    hpssSession->mySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (hpssSession->mySocket < 0) {
         rodsLog (LOG_ERROR,
           "createControlSocket: create socket error, errno=%d", errno);
         return (SYS_INVALID_SERVER_HOST - errno);
    }

    bzero (&hpssSession->mySocketAddr, sizeof(struct sockaddr_in));
    hpssSession->mySocketAddr.sin_family = AF_INET;
    hpssSession->mySocketAddr.sin_addr.s_addr = INADDR_ANY;
    hpssSession->mySocketAddr.sin_port = 0;
    status = bind (hpssSession->mySocket,
      (const struct sockaddr *) &hpssSession->mySocketAddr,
       sizeof(struct sockaddr_in)); 
    if (status < 0) {
         rodsLog (LOG_ERROR,
           "createControlSocket: create socket error, errno=%d", errno);
         return (SYS_INVALID_SERVER_HOST - errno);
    }

    myLen = sizeof (struct sockaddr_in);

    status = getsockname (hpssSession->mySocket,
      (struct sockaddr *) &hpssSession->mySocketAddr, (socklen_t *) &myLen);

    if (status < 0) {
         rodsLog (LOG_ERROR,
           "createControlSocket: getsockname error, errno=%d", errno);
         return (SYS_INVALID_SERVER_HOST - errno);
    }

    status = listen (hpssSession->mySocket, SOMAXCONN);
    if (status < 0) {
         rodsLog (LOG_ERROR,
           "createControlSocket: listen error, errno=%d", errno);
         return (SYS_INVALID_SERVER_HOST - errno);
    }

    return (0);
}

int
initHpssIodForWrite (hpss_IOD_t *iod, iod_srcsinkdesc_t *srcDesc,
iod_srcsinkdesc_t *sinkDesc, int destFd, hpssSession_t *hpssSession)
{

    memset(iod, 0, sizeof(hpss_IOD_t));
    memset(srcDesc, 0, sizeof(iod_srcsinkdesc_t));
    memset(sinkDesc, 0, sizeof(iod_srcsinkdesc_t));

    srcDesc->Flags = HPSS_IOD_XFEROPT_IP;
    srcDesc->Flags |= HPSS_IOD_CONTROL_ADDR;

    /* Set the the block of bytes we want */
    sinkDesc->Offset = srcDesc->Offset = cast64m(0);
    sinkDesc->Length = srcDesc->Length = hpssSession->fileSize64;

    sinkDesc->SrcSinkAddr.Type = CLIENTFILE_ADDRESS;
    sinkDesc->SrcSinkAddr.Addr_u.ClientFileAddr.FileDes = destFd;
    sinkDesc->SrcSinkAddr.Addr_u.ClientFileAddr.FileOffset = cast64m(0);
    srcDesc->SrcSinkAddr.Type = NET_ADDRESS;
    srcDesc->SrcSinkAddr.Addr_u.NetAddr.SockTransferID =
     cast64m(hpssSession->requestId);
#ifdef HPSS7
    srcDesc->SrcSinkAddr.Addr_u.NetAddr.SockAddr.Addr.hpss_saddr_u.ipv4_addr =
     hpssSession->ipAddr;
    srcDesc->SrcSinkAddr.Addr_u.NetAddr.SockAddr.Addr.family =
     hpssSession->mySocketAddr.sin_family;
#else
    srcDesc->SrcSinkAddr.Addr_u.NetAddr.SockAddr.addr =
     hpssSession->ipAddr;
    srcDesc->SrcSinkAddr.Addr_u.NetAddr.SockAddr.family =
     hpssSession->mySocketAddr.sin_family;
#endif
    srcDesc->SrcSinkAddr.Addr_u.NetAddr.SockAddr.port = 
     hpssSession->mySocketAddr.sin_port;
    srcDesc->SrcSinkAddr.Addr_u.NetAddr.SockOffset = cast64m(0);
    iod->Function = HPSS_IOD_WRITE;
    iod->RequestID = hpssSession->requestId;
    iod->SrcDescLength = 1;
    iod->SinkDescLength = 1;
    iod->SrcDescList = srcDesc;
    iod->SinkDescList = sinkDesc;

    hpssSession->totalBytesMoved64 = cast64m(0);
    hpssSession->status = HPSS_E_NOERROR;

    return (0);
}

void
moverConnManager (hpssSession_t *hpssSession)
{
    int moverSocket;
    int i;    
    struct sockaddr_in socketAddr;
    int len;
    /* Loop until the thread is cancelled */
    for (;;) {
        len = sizeof(socketAddr);
        while ((moverSocket = accept (hpssSession->mySocket,
          (struct sockaddr *) &socketAddr, (socklen_t *) &len)) < 0) {
             if ((errno != EINTR) && (errno != EAGAIN)) {
                 rodsLog (LOG_ERROR,
                   "moverConnManager: socket accept error, errno=%d", errno);
		hpssSession->status = SYS_SOCK_ACCEPT_ERR - errno;
                 return;
	     }
	     hpssSession->status = SYS_SOCK_ACCEPT_ERR - errno;
             break;
        }
        if (moverSocket < 0) break;

        do {
            pthread_mutex_lock(&hpssSession->myMutex);
            for (i = 0; i < MAX_HPSS_CONNECTIONS; i++) {
                 if (!hpssSession->thrInfo[i].active) {
                    hpssSession->thrInfo[i].active = 1;
                    hpssSession->thrInfo[i].moverSocket = moverSocket;
                    hpssSession->thrInfo[i].hpssSession = hpssSession;
        	    hpssSession->thrCnt++;
                    break;
	    	}
	    }
            pthread_mutex_unlock (&hpssSession->myMutex);
            if (i == MAX_HPSS_CONNECTIONS) {
                rodsSleep (0, 500000);
            }
	} while (i == MAX_HPSS_CONNECTIONS);
        rodsSetSockOpt (moverSocket, 0);

        if (hpssSession->operation == HPSS_GET_OPR) {
            pthread_create(&hpssSession->thrInfo[i].threadId,
                     pthread_attr_default,
                     (void *(*)(void *)) getMover,
                     (void *) &hpssSession->thrInfo[i]);
        } else if (hpssSession->operation == HPSS_PUT_OPR) {
            pthread_create (&hpssSession->thrInfo[i].threadId,
                     pthread_attr_default,
                     (void *(*)(void *)) putMover,
                     (void *) &hpssSession->thrInfo[i]);
        }
#if !defined(solaris_platform)
        sched_yield();
#endif
    }       
    return;
}

void 
getMover (hpssThrInfo_t *thrInfo)
{
    int status = 0;
    int             transferListenSocket = -1;   /* Socket listen descriptors */
    int             transferSocketFd = -1;    /* accepted socket */
    int             bytesReceived;
    initiator_msg_t initMessage, initReply;
    initiator_ipaddr_t ipAddr;   /* TCP socket address info */
    completion_msg_t completionMessage;
    char buffer[HPSS_BUF_SIZE];
    int destFd = -1;
    rodsLong_t offset = -1;
    rodsLong_t curoffset = 0;
    hpssSession_t *mySession = (hpssSession_t *) thrInfo->hpssSession;
    int bytesWritten;
    int length = 0;

    pthread_mutex_lock(&mySession->myMutex);
    if (mySession->createFlag == 0) {
	/* noone has created the file yet */
        destFd = open (mySession->unixFilePath, O_WRONLY | O_CREAT | O_TRUNC, 
	  mySession->createMode);
	mySession->createFlag = 1;
    } else {
        destFd = open (mySession->unixFilePath, O_WRONLY, 
	  mySession->createMode);
    }
    pthread_mutex_unlock(&mySession->myMutex);
    
    if (destFd < 0) {
        mySession->status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog (LOG_ERROR,
         "getMover: open error for destUnixFile %s, status = %d",
         mySession->unixFilePath, mySession->status);
        return;
    }

    /* Loop until we reach a condition to discontinue talking with Mover */
    while (mySession->status == HPSS_E_NOERROR) {
        /* Get the next transfer initiation message from the Mover. HPSS_ECONN
         * will be returned when the Mover is done.  */
        status = procMoverInitmsg (thrInfo, &initMessage, &initReply);
        if (status == HPSS_ECONN) {
             break;                 /* break out of the while loop */
        } else if (status != HPSS_E_NOERROR) {
            mySession->status = status;
            continue;
        }

        status = procTransferListenSocket (thrInfo, &ipAddr,
          &transferListenSocket);
        if (status != HPSS_E_NOERROR) {
            mySession->status = status;
            continue;
        }
        /*
         * Wait for the new Mover socket connection, if you don't already
         * have one
         */
        status = procTransferSocketFd (thrInfo, transferListenSocket,
          &transferSocketFd);
        if (status != HPSS_E_NOERROR) {
            mySession->status = status;
            continue;
        }

        /*
         * Send the data to the Mover via our socket
         */
        status = mover_socket_recv_data(transferSocketFd,
                                cast64m(mySession->requestId),
                                initMessage.Offset,
                                buffer,
                                low32m(initReply.Length),
                                &bytesReceived, 1);

        if (status <= 0) {
            rodsLog (LOG_ERROR,
             "getMover: mover_socket_recv_data error, status = %d", status);
            mySession->status = HPSS_MOVER_PROT_ERR + status;
            continue;
        } else {
            length = low32m (initReply.Length);
	}
        CONVERT_U64_TO_LONGLONG(initMessage.Offset, offset);
        if (offset != curoffset) {
            lseek (destFd, offset, SEEK_SET);
            curoffset = offset;
        }
        bytesWritten = write (destFd, (void *) buffer, length);
	if (bytesWritten <= 0) {
            mySession->status = UNIX_FILE_WRITE_ERR - errno;
            rodsLog (LOG_ERROR,
             "getMover: write error for destUnixFile %s, status = %d",
             mySession->unixFilePath, mySession->status);
	    continue;
	} else if (bytesWritten != length) {
            mySession->status = SYS_COPY_LEN_ERR;
            rodsLog (LOG_ERROR,
             "getMover: write error for unixFile %s, towrite %d != written %d",
             mySession->unixFilePath, length, bytesWritten);
            continue;
	} else {
            curoffset += length;

	}
        /*
         * Get a transfer completion message from the Mover
         */
        status = mvrprot_recv_compmsg( thrInfo->moverSocket,
          &completionMessage);
        if (status != HPSS_E_NOERROR) {
            rodsLog (LOG_ERROR,
             "getMover: mvrprot_recv_compmsg error, status = %d",
             status);
            mySession->status = HPSS_MOVER_PROT_ERR + status;
            continue;
        }
        pthread_mutex_lock(&mySession->myMutex);
        inc64m (mySession->totalBytesMoved64, completionMessage.BytesMoved);
        if (ge64m(mySession->totalBytesMoved64, mySession->fileSize64)) {
            mySession->status = 0;
            pthread_mutex_unlock(&mySession->myMutex);
            break;
        }
        pthread_mutex_unlock(&mySession->myMutex);
    }                            /* end while loop */

    /* Close down the TCP transfer socket if it got opened */
    if (transferSocketFd != -1) (void) close(transferSocketFd);
    if (transferListenSocket != -1) (void) close(transferListenSocket);
    close (destFd);

    pthread_mutex_lock(&mySession->myMutex);
    (void) close(thrInfo->moverSocket);
    thrInfo->active = 0;
    pthread_mutex_unlock(&mySession->myMutex);
    return;
}

void 
putMover (hpssThrInfo_t *thrInfo)
{
    int status = 0;
    int             transferListenSocket = -1;   /* Socket listen descriptors */
    int             transferSocketFd = -1;    /* accepted socket */
    int             bytesSent;
    initiator_msg_t initMessage, initReply;
    initiator_ipaddr_t ipAddr;   /* TCP socket address info */
    completion_msg_t completionMessage;
    char buffer[HPSS_BUF_SIZE];
    int srcFd;
    rodsLong_t offset = -1;
    rodsLong_t curoffset = 0;
    hpssSession_t *mySession = (hpssSession_t *) thrInfo->hpssSession;
    int length = 0;


    srcFd = open (mySession->unixFilePath, O_RDONLY, 0);
    if (srcFd < 0) {
        mySession->status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog (LOG_ERROR,
         "paraHpssPut: open error for unixFilePath %s, status = %d",
         mySession->unixFilePath, mySession->status);
        return;
    }

    /* Loop until we reach a condition to discontinue talking with Mover */
    while (mySession->status == HPSS_E_NOERROR) {
        /* Get the next transfer initiation message from the Mover. HPSS_ECONN
         * will be returned when the Mover is done.  */
	status = procMoverInitmsg (thrInfo, &initMessage, &initReply);
	if (status == HPSS_ECONN) {
             break;                 /* break out of the while loop */
	} else if (status != HPSS_E_NOERROR) {
	    mySession->status = status;
	    continue;
	}

        status = procTransferListenSocket (thrInfo, &ipAddr, 
	  &transferListenSocket);
        if (status != HPSS_E_NOERROR) {
            mySession->status = status;
            continue;
        }

        /*
         * Wait for the new Mover socket connection, if you don't already
         * have one
         */
        status = procTransferSocketFd (thrInfo, transferListenSocket,
	  &transferSocketFd);
        if (status != HPSS_E_NOERROR) {
            mySession->status = status;
            continue;
        }

        CONVERT_U64_TO_LONGLONG(initMessage.Offset, offset);
	if (offset != curoffset) {
            lseek (srcFd, offset, SEEK_SET);
	    curoffset = offset;
        } 
        length = low32m (initReply.Length);

        status = read (srcFd, buffer, length);

	if (status != length) {
            rodsLog (LOG_ERROR,
             "putMover: bytes read %d != requested %d", status, length);
            mySession->status = SYS_COPY_LEN_ERR;
            continue;
        }

        /*
         * Send the data to the Mover via our socket
         */
        status = mover_socket_send_requested_data(transferSocketFd,
                                cast64m(mySession->requestId),
                                                 initMessage.Offset,
                                                  buffer,
                                                   low32m(initReply.Length),
                                                   &bytesSent, 1);
        if (status <= 0) {
            rodsLog (LOG_ERROR,
             "putMover: mover_socket_send_requested_data error, status = %d",
             status);
            mySession->status = HPSS_MOVER_PROT_ERR + status;
            continue;
        } else {
	    curoffset += length;
	}
        /*
         * Get a transfer completion message from the Mover
         */
        status = mvrprot_recv_compmsg( thrInfo->moverSocket, 
          &completionMessage);
        if (status != HPSS_E_NOERROR) {
            rodsLog (LOG_ERROR,
             "putMover: mvrprot_recv_compmsg error, status = %d",
             status);
            mySession->status = HPSS_MOVER_PROT_ERR + status;
            continue;
        }

        pthread_mutex_lock(&mySession->myMutex);
        inc64m (mySession->totalBytesMoved64, completionMessage.BytesMoved);
        if (ge64m(mySession->totalBytesMoved64, mySession->fileSize64)) {
            mySession->status = 0;
            pthread_mutex_unlock(&mySession->myMutex);
            break;
        }
        pthread_mutex_unlock(&mySession->myMutex);
    }                            /* end while loop */

    /* Close down the TCP transfer socket if it got opened */
    if (transferSocketFd != -1) (void) close(transferSocketFd);
    if (transferListenSocket != -1) (void) close(transferListenSocket);
    close (srcFd);

    pthread_mutex_lock(&mySession->myMutex);
    (void) close(thrInfo->moverSocket);
    thrInfo->active = 0;
    pthread_mutex_unlock(&mySession->myMutex);
    return;
}

int
procMoverInitmsg (hpssThrInfo_t *thrInfo, initiator_msg_t *initMessage, 
initiator_msg_t *initReply)
{
    int status;
    hpssSession_t *mySession = (hpssSession_t *) thrInfo->hpssSession;

    /* Get the next transfer initiation message from the Mover. HPSS_ECONN
     * will be returned when the Mover is done.  */

    status = mvrprot_recv_initmsg (thrInfo->moverSocket, initMessage);
    if (status != HPSS_E_NOERROR) {
	 if (status == HPSS_ECONN) return HPSS_ECONN;      /* done */
         status = HPSS_MOVER_PROT_ERR + status;
         rodsLog (LOG_ERROR,
           "procMoverInitmsg: mvrprot_recv_initmsg err, status = %d",
            mySession->status);
	return status;
    }

    /* Tell the Mover we will send the address next */
    initReply->Flags = MVRPROT_COMP_REPLY | MVRPROT_ADDR_FOLLOWS;
    /*
     * Let's agree to use the transfer protocol selected by the Mover and
     * let's accept the offset. However, the number of bytes the Mover can
     * transfer at one time is limited by our buffer size, so we tell the
     * Mover how much of the data he has offerred that we are willing to
     * accept.
     */

    initReply->Type = initMessage->Type;
    initReply->Offset = initMessage->Offset;
    if (gt64m(initMessage->Length, cast64m(HPSS_BUF_SIZE)))
        initReply->Length = cast64m(HPSS_BUF_SIZE);
    else
        initReply->Length = initMessage->Length;
    /*
     * Send our response back to the Mover
     */
    status = mvrprot_send_initmsg( thrInfo->moverSocket, initReply);
    if (status != HPSS_E_NOERROR) {
        status = HPSS_MOVER_PROT_ERR + status;
        rodsLog (LOG_ERROR,
          "procMoverInitmsg: mvrprot_send_initmsg err, status = %d", status);
        return status;
    }
    /*
     * Based on the type of transfer protocol, allocate memory, send
     * address information, and send the data to the HPSS Mover
     */
    if (initMessage->Type != NET_ADDRESS) {
        status = HPSS_MOVER_PROT_ERR;
        rodsLog (LOG_ERROR, 
	  "procMoverInitmsg: initMessage.Type != NET_ADDRESS");
        return status;
    }

    return HPSS_E_NOERROR;
}

int
procTransferListenSocket (hpssThrInfo_t *thrInfo, initiator_ipaddr_t *ipAddr,
int *transferListenSocket)
{
    int status;
    struct sockaddr_in transferSocketAddr;    /* Transfer socket address */
    int tmp;
    hpssSession_t *mySession = (hpssSession_t *) thrInfo->hpssSession;

    if (*transferListenSocket < 0) {
        *transferListenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (*transferListenSocket == -1) {
            rodsLog (LOG_ERROR,
             "procTransferListenSocket: socket error, errno = %d", errno);
            status = SYS_SOCK_OPEN_ERR - errno;
	    return status;
        }

        (void) memset(&transferSocketAddr, 0, sizeof(transferSocketAddr));
        transferSocketAddr.sin_family = AF_INET;
        transferSocketAddr.sin_port = 0;
        /*
         * Select the hostname (IP address) in a round-robin fashion
         */
        pthread_mutex_lock(&mySession->myMutex);
        transferSocketAddr.sin_addr.s_addr = mySession->ipAddr;
        pthread_mutex_unlock(&mySession->myMutex);
        if (bind(*transferListenSocket,
                 (const struct sockaddr *) &transferSocketAddr,
                 sizeof(transferSocketAddr)) == -1) {
            rodsLog (LOG_ERROR,
             "procTransferListenSocket: socket bind error, errno=%d", errno);
            status = SYS_SOCK_BIND_ERR - errno;
            return status;;
        }
        tmp = sizeof(transferSocketAddr);
        (void) memset(&transferSocketAddr, 0, sizeof(transferSocketAddr));
        if (getsockname(*transferListenSocket,
                        (struct sockaddr *) & transferSocketAddr,
                        (socklen_t *) & tmp) == -1) {
            rodsLog (LOG_ERROR,
             "procTransferListenSocket: getsockname error, errno=%d", errno);
            status = SYS_SOCK_OPEN_ERR - errno;
            return status;;
        }
        if (listen(*transferListenSocket, SOMAXCONN) == -1) {
            rodsLog (LOG_ERROR,
             "procTransferListenSocket: listen error, errno=%d", errno);
            status = SYS_SOCK_OPEN_ERR - errno;
            return status;;
        }

        memset(ipAddr, 0, sizeof(ipAddr));
        ipAddr->IpAddr.SockTransferID =
          cast64m(mySession->requestId);
#ifdef HPSS7
        ipAddr->IpAddr.SockAddr.Addr.family = transferSocketAddr.sin_family;
        ipAddr->IpAddr.SockAddr.Addr.hpss_saddr_u.ipv4_addr = 
	  transferSocketAddr.sin_addr.s_addr;
#else
        ipAddr->IpAddr.SockAddr.family = transferSocketAddr.sin_family;
        ipAddr->IpAddr.SockAddr.addr = transferSocketAddr.sin_addr.s_addr;
#endif
        ipAddr->IpAddr.SockAddr.port = transferSocketAddr.sin_port;
        ipAddr->IpAddr.SockOffset = cast64m(0);
    }
    status = mvrprot_send_ipaddr(thrInfo->moverSocket, ipAddr);
    if (status != HPSS_E_NOERROR) {
        rodsLog (LOG_ERROR,
         "procTransferListenSocket: mvrprot_send_ipaddr err, errno=%d",errno);
        status = HPSS_MOVER_PROT_ERR + status;
    }

    return status;
}

int
procTransferSocketFd (hpssThrInfo_t *thrInfo, int transferListenSocket,
int *transferSocketFd)
{
    int status, tmp;
    struct sockaddr_in transferSocketAddr;    /* Transfer socket address */

    /*
     * Wait for the new Mover socket connection, if you don't already
     * have one
     */
    if (*transferSocketFd >= 0) return HPSS_E_NOERROR;
    tmp = sizeof(transferSocketAddr);
    while ((*transferSocketFd = accept(transferListenSocket,
                   (struct sockaddr *) &transferSocketAddr,
                   (socklen_t *) & tmp)) < 0) {
        if ((errno != EINTR) && (errno != EAGAIN)) {
            rodsLog (LOG_ERROR,
             "procTransferSocketFd: accept error, errno=%d", errno);
            status = SYS_SOCK_OPEN_ERR - errno;
            return status;
        }
    } 

    if (*transferSocketFd < 0) {
	/* should not be here */
        rodsLog (LOG_ERROR,
         "procTransferSocketFd: accept error, errno=%d", errno);
	status = SYS_SOCK_OPEN_ERR - errno;
        return status;
    }
    rodsSetSockOpt (*transferSocketFd, 0);

    return HPSS_E_NOERROR;
}

int
paraHpssGet (char *srcHpssFile, char *destUnixFile, int mode, int flags,
rodsLong_t mySize)
{
    int srcFd;
    int status;
    hpssSession_t myHpssSession;
    hpss_IOD_t      iod;                /* IOD passed to hpss_WriteList */
    hpss_IOR_t      ior;                /* IOR returned from hpss_WriteList */
    iod_srcsinkdesc_t   srcDesc, sinkDesc;  /* IOD source/sink descriptors */
    u_signed64      bytesMoved;  /* Bytes transferred, as returned from IOR */
    u_signed64 gapLength;        /* Number of "gap" bytes within the file */
    int readListFlags = 0;       /* hpss_ReadList call - could be set
                                  * to HPSS_READ_SEQUENTIAL */



    status = initHpssSession (&myHpssSession, HPSS_GET_OPR,
      destUnixFile, mySize, mode);
    if (status < 0) return status;

    srcFd = hpssOpenForRead (srcHpssFile, O_RDONLY);
    if (srcFd < 0) {
        return srcFd;
    }

    status = createControlSocket (&myHpssSession);
    if (status < 0) return status;

    pthread_create(&myHpssSession.moverConnManagerThr, pthread_attr_default,
      (void *(*)(void *)) moverConnManager,
      (void *) &myHpssSession);

#if !defined(solaris_platform)
    sched_yield();
#endif
    initHpssIodForRead (&iod, &srcDesc, &sinkDesc, srcFd, &myHpssSession);

    gapLength = bytesMoved = cast64m(0);

    /*
     * Loop as long as the total bytes moved plus all reported gaps are less
     * than the total size of the file AND no transfer error has been
     * encountered
     */
    while (lt64m(add64m(bytesMoved, gapLength), myHpssSession.fileSize64) &&
      myHpssSession.status == HPSS_E_NOERROR) {
        /*
         * Set the source/sink length to the number of bytes we want
         */
        srcDesc.Offset = sinkDesc.Offset = add64m(bytesMoved, gapLength);
        srcDesc.Length = sinkDesc.Length = 
	  sub64m(myHpssSession.fileSize64, srcDesc.Offset);
        srcDesc.SrcSinkAddr.Addr_u.ClientFileAddr.FileOffset = srcDesc.Offset;
        memset(&ior, 0, sizeof(ior));
        status = hpss_ReadList(&iod, readListFlags, &ior);
        if (status) {
            if (ior.Status != HPSS_E_NOERROR) {
		rodsLog (LOG_ERROR,
                 "paraHpssGet: hpss_ReadList error,status=%d,ior status=%d",
                 status, ior.Status);
                 myHpssSession.status = HPSS_READ_LIST_ERR + ior.Status;
            } else if (myHpssSession.status == HPSS_E_NOERROR) {
		rodsLog (LOG_ERROR,
                  "paraHpssGet: hpss_ReadList error, status= %d", status);
                myHpssSession.status = HPSS_READ_LIST_ERR + status;
            }
        } else {
            inc64m(bytesMoved, ior.SinkReplyList->BytesMoved);
            /*
             * See if data transfer stopped at a gap (hole)
             */
            if (ior.Flags & HPSS_IOR_GAPINFO_VALID) {
                inc64m (gapLength,
                  ior.ReqSpecReply->ReqReply_s.ReqReply_u.GapInfo.Length);
            }
            if (ior.SrcReplyList) free (ior.SrcReplyList);
            if (ior.SinkReplyList) free (ior.SinkReplyList);
        }
    }   /* end while */

   /*
    * Close the HPSS file
    */
    status = hpss_Close(srcFd);

    status = postProcSessionThr (&myHpssSession, srcHpssFile);
    return status;

}

/* initHpssIodForRead - Must be done after initSession and initControlSocket
 */

int
initHpssIodForRead (hpss_IOD_t *iod, iod_srcsinkdesc_t *src, 
iod_srcsinkdesc_t *sink, int hpssSrcFd, hpssSession_t *hpssSession)
{

    memset(iod, 0, sizeof(hpss_IOD_t));
    memset(src, 0, sizeof(iod_srcsinkdesc_t));
    memset(sink, 0, sizeof(iod_srcsinkdesc_t));

    sink->Flags = HPSS_IOD_XFEROPT_IP;
    sink->Flags |= HPSS_IOD_CONTROL_ADDR; 
    src->SrcSinkAddr.Type = CLIENTFILE_ADDRESS;
    src->SrcSinkAddr.Addr_u.ClientFileAddr.FileDes = hpssSrcFd;
    hpssSession->requestId = getpid();
    sink->SrcSinkAddr.Type = NET_ADDRESS;
    sink->SrcSinkAddr.Addr_u.NetAddr.SockTransferID =
     cast64m(hpssSession->requestId);
#ifdef HPSS7
    sink->SrcSinkAddr.Addr_u.NetAddr.SockAddr.Addr.hpss_saddr_u.ipv4_addr =
    hpssSession->ipAddr;
    sink->SrcSinkAddr.Addr_u.NetAddr.SockAddr.Addr.family =
      hpssSession->mySocketAddr.sin_family;;
#else
    sink->SrcSinkAddr.Addr_u.NetAddr.SockAddr.addr =
    hpssSession->ipAddr;
    sink->SrcSinkAddr.Addr_u.NetAddr.SockAddr.family =
      hpssSession->mySocketAddr.sin_family;;
#endif
    sink->SrcSinkAddr.Addr_u.NetAddr.SockOffset = cast64m(0);
    sink->SrcSinkAddr.Addr_u.NetAddr.SockAddr.port =
      hpssSession->mySocketAddr.sin_port; 
    iod->Function = HPSS_IOD_READ;
    iod->RequestID = hpssSession->requestId;
    iod->SrcDescLength = 1;
    iod->SinkDescLength = 1;
    iod->SrcDescList = src;
    iod->SinkDescList = sink;

    hpssSession->totalBytesMoved64 = cast64m(0);
    hpssSession->status = HPSS_E_NOERROR;

   return (0);
}

int
postProcSessionThr (hpssSession_t *myHpssSession, char *hpssPath)
{
    int i;
    int status;
    int pthreadStatus;

    /* make sure that all data has been sent before killing any active
     * transfer threads */
    pthread_mutex_lock (&myHpssSession->myMutex);
    if (neq64m (myHpssSession->fileSize64, myHpssSession->totalBytesMoved64)) {
#if defined(aix_platform)
        struct timespec delay = {1, 0};   /* 1 second */
#else
        int delay = 1;
#endif
        pthread_mutex_unlock (&myHpssSession->myMutex);
        /* Wait for all transfer threads to complete before moving on */
        for (i = 0; i < MAX_HPSS_CONNECTIONS; i++) {
            pthread_mutex_lock (&myHpssSession->myMutex);
            while (myHpssSession->thrInfo[i].active) {
                pthread_mutex_unlock (&myHpssSession->myMutex);
#if defined(aix_platform)
                (void) pthread_delay_np (&delay);
#else
                rodsSleep (delay, 0);
#endif
                pthread_mutex_lock (&myHpssSession->myMutex);
            }
            pthread_mutex_unlock (&myHpssSession->myMutex);
        }
    }
    pthread_mutex_unlock(&myHpssSession->myMutex);

   /*
    * Now cancel the manage_mover_connections thread
    */
   (void) pthread_cancel (myHpssSession->moverConnManagerThr);

    for (i = 0; i < myHpssSession->thrCnt; i++) {
        if (myHpssSession->thrInfo[i].threadId != 0)
            (void) pthread_join (myHpssSession->thrInfo[i].threadId, NULL);
    }

#if !defined (solaris_platform)
     /* For solaris, pthread_join will hang.*/
    if (myHpssSession->moverConnManagerThr != 0)
       (void) pthread_join (myHpssSession->moverConnManagerThr, 
         (void **) &pthreadStatus);
#endif

    if (myHpssSession->status == HPSS_E_NOERROR ||
      myHpssSession->status == HPSS_ECONN) {
        rodsLong_t bytesWritten;
	rodsLong_t mySize;
	
        CONVERT_U64_TO_LONGLONG(myHpssSession->totalBytesMoved64, bytesWritten);
        CONVERT_U64_TO_LONGLONG(myHpssSession->fileSize64, mySize);
        if (bytesWritten != mySize) {
            rodsLog (LOG_ERROR,
              "postProcSessionThr: %s transfer size %lld != file size %lld",
               hpssPath, bytesWritten, mySize);
            status = SYS_COPY_LEN_ERR;
        } else {
            status = 0;
        }
    } else {
        status = myHpssSession->status;
    }
    return status;


    return HPSS_E_NOERROR;
}
