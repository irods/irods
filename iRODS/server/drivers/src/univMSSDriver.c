/*** Copyright (c) 2009 Data Intensive Cyberinfrastructure Foundation. All rights reserved.    ***
 *** For full copyright notice please refer to files in the COPYRIGHT directory                ***/
/* Written by Jean-Yves Nief of CCIN2P3 and copyright assigned to Data Intensive Cyberinfrastructure Foundation */
 
#include "univMSSDriver.h"

/* univMSSSyncToArch - This function is for copying the file from cacheFilename to filename in the MSS. 
 * optionalInfo info is not used.
 */

int univMSSSyncToArch (rsComm_t *rsComm, fileDriverType_t cacheFileType, 
				   int mode, int flags, char *filename,
				   char *cacheFilename,  rodsLong_t dataSize, keyValPair_t *condInput) {
				   
    int lenDir, rc, status;
	execCmd_t execCmdInp;
	char *lastpart;
	char cmdArgv[HUGE_NAME_LEN] = "";
	char dirname[MAX_NAME_LEN] = "";
	execCmdOut_t *execCmdOut = NULL;
	
        bzero (&execCmdInp, sizeof (execCmdInp));
	/* first create the directory on the target MSS */
	lastpart = strrchr(filename, '/');
	lenDir = strlen(filename) - strlen(lastpart);
	strncpy(dirname, filename, lenDir);
	
	status = univMSSFileMkdir (rsComm, dirname, mode);
	if ( status == 0 ) {
		rstrcpy(execCmdInp.cmd, UNIV_MSS_INTERF_SCRIPT, LONG_NAME_LEN);
		strcat(cmdArgv, "syncToArch");
		strcat(cmdArgv, " ");
		strcat(cmdArgv, cacheFilename);
		strcat(cmdArgv, " ");
		strcat(cmdArgv, filename);
		rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
		rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
		status = _rsExecCmd(rsComm, &execCmdInp, &execCmdOut);
		if ( status == 0 ) {
			rc = univMSSFileChmod (rsComm, filename, mode);
		}
		else {
			status = UNIV_MSS_SYNCTOARCH_ERR - errno;
			rodsLog (LOG_ERROR, "univMSSSyncToArch: copy of %s to %s failed, status = %d",
					cacheFilename, filename, status);
		}
	}
    return (status);
}

/* univMSSStageToCache - This function is to stage filename (stored in the MSS) to cacheFilename. 
 * optionalInfo info is not used.
 */
int univMSSStageToCache (rsComm_t *rsComm, fileDriverType_t cacheFileType, 
				     int mode, int flags, char *filename,
				     char *cacheFilename,  rodsLong_t dataSize, keyValPair_t *condInput) {
				   
    int status;
	execCmd_t execCmdInp;
	char cmdArgv[HUGE_NAME_LEN] = "";
	execCmdOut_t *execCmdOut = NULL;
	
        bzero (&execCmdInp, sizeof (execCmdInp));
	rstrcpy(execCmdInp.cmd, UNIV_MSS_INTERF_SCRIPT, LONG_NAME_LEN);
	strcat(cmdArgv, "stageToCache");
	strcat(cmdArgv, " ");
	strcat(cmdArgv, filename);
	strcat(cmdArgv, " ");
	strcat(cmdArgv, cacheFilename);
	rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
	rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
	status = _rsExecCmd(rsComm, &execCmdInp, &execCmdOut);
	
	if (status < 0) {
        status = UNIV_MSS_STAGETOCACHE_ERR - errno; 
		rodsLog (LOG_ERROR, "univMSSStageToCache: staging from %s to %s failed, status = %d",
         filename, cacheFilename, status);
    }
	
    return (status);
	
}

/* univMSSFileUnlink - This function is to remove a file stored in the MSS. 
 */
int univMSSFileUnlink (rsComm_t *rsComm, char *filename) {
    
	int status;
	execCmd_t execCmdInp;
	char cmdArgv[HUGE_NAME_LEN] = "";
	execCmdOut_t *execCmdOut = NULL;
	
        bzero (&execCmdInp, sizeof (execCmdInp));
	rstrcpy(execCmdInp.cmd, UNIV_MSS_INTERF_SCRIPT, LONG_NAME_LEN);
	strcat(cmdArgv, "rm");
	strcat(cmdArgv, " ");
	strcat(cmdArgv, filename);
	rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
	rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
	status = _rsExecCmd(rsComm, &execCmdInp, &execCmdOut);

    if (status < 0) {
        status = UNIV_MSS_UNLINK_ERR - errno;
		rodsLog (LOG_ERROR, "univMSSFileUnlink: unlink of %s error, status = %d",
         filename, status);
    }

    return (status);
}

/* univMSSFileMkdir - This function is to create a directory in the MSS. 
 */
int univMSSFileMkdir (rsComm_t *rsComm, char *dirname, int mode) {
	
	int status;
	execCmd_t execCmdInp;
	char cmdArgv[HUGE_NAME_LEN] = "";
	execCmdOut_t *execCmdOut = NULL;  

	bzero (&execCmdInp, sizeof (execCmdInp));
	rstrcpy(execCmdInp.cmd, UNIV_MSS_INTERF_SCRIPT, LONG_NAME_LEN);
	strcat(cmdArgv, "mkdir");
	strcat(cmdArgv, " ");
	strcat(cmdArgv, dirname);
	rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
	rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
	status = _rsExecCmd(rsComm, &execCmdInp, &execCmdOut);
	
	if (status < 0) {
		status = UNIV_MSS_MKDIR_ERR - errno;
		rodsLog (LOG_ERROR, "univMSSFileMkdir: cannot create directory for %s error, status = %d",
		         dirname, status);
    }
	
	mode = getDefDirMode(); 
	status = univMSSFileChmod(rsComm, dirname, mode);
	
	return (status);
}

/* univMSSFileChmod - This function is to change ACL for a directory a file in the MSS. 
 */
int univMSSFileChmod (rsComm_t *rsComm, char *name, int mode) {
	
	int status;
	execCmd_t execCmdInp;
	char cmdArgv[HUGE_NAME_LEN] = "";
	char strmode[4];
	execCmdOut_t *execCmdOut = NULL;  
	
	if ( mode != getDefDirMode() ) {
		mode = getDefFileMode();
	}
        bzero (&execCmdInp, sizeof (execCmdInp));
	rstrcpy(execCmdInp.cmd, UNIV_MSS_INTERF_SCRIPT, LONG_NAME_LEN);
	strcat(cmdArgv, "chmod");
	strcat(cmdArgv, " ");
	strcat(cmdArgv, name);
	strcat(cmdArgv, " ");
	sprintf (strmode, "%o", mode);
	strcat(cmdArgv, strmode);
	rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
	rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
	status = _rsExecCmd(rsComm, &execCmdInp, &execCmdOut);
	
	if (status < 0) {
		status = UNIV_MSS_CHMOD_ERR - errno;
		rodsLog (LOG_ERROR, "univMSSFileChmod: cannot chmod for %s, status = %d",
		         name, status);
    }
	
	return (status);
}

/* univMSSFileStat - This function returns the stats of the file stored in the MSS. 
 */
 int univMSSFileStat (rsComm_t *rsComm, char *filename, struct stat *statbuf) {
 
	int i, status;
	execCmd_t execCmdInp;
	char cmdArgv[HUGE_NAME_LEN] = "";
	char splchain1[13][MAX_NAME_LEN], splchain2[4][MAX_NAME_LEN], splchain3[3][MAX_NAME_LEN];
	char *outputStr;
	const char *delim1 = ":\n";
	const char *delim2 = "-";
	const char *delim3 = ".";
	execCmdOut_t *execCmdOut = NULL;
	struct tm mytm;
	time_t myTime;
	
        bzero (&execCmdInp, sizeof (execCmdInp));
	rstrcpy(execCmdInp.cmd, UNIV_MSS_INTERF_SCRIPT, LONG_NAME_LEN);
	strcat(cmdArgv, "stat");
	strcat(cmdArgv, " ");
	strcat(cmdArgv, filename);
	rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
	rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
	status = _rsExecCmd(rsComm, &execCmdInp, &execCmdOut);
	
	if (status == 0) {
		if ( execCmdOut->stdoutBuf.buf != NULL) {
			outputStr = (char*)execCmdOut->stdoutBuf.buf;
			memset(&splchain1, 0, sizeof(splchain1));
			strSplit(outputStr, delim1, splchain1);
			statbuf->st_dev = atoi(splchain1[0]);
			statbuf->st_ino = atoi(splchain1[1]);
			statbuf->st_mode = atoi(splchain1[2]);
			statbuf->st_nlink = atoi(splchain1[3]);
			statbuf->st_uid = atoi(splchain1[4]);
			statbuf->st_gid = atoi(splchain1[5]);
			statbuf->st_rdev = atoi(splchain1[6]);
			statbuf->st_size = atoll(splchain1[7]);
			statbuf->st_blksize = atoi(splchain1[8]);
			statbuf->st_blocks = atoi(splchain1[9]);
			for (i = 0; i < 3; i++) {
				memset(&splchain2, 0, sizeof(splchain2));
				memset(&splchain3, 0, sizeof(splchain3));
				strSplit(splchain1[10+i], delim2, splchain2);
				mytm.tm_year = atoi(splchain2[0]) - 1900;
				mytm.tm_mon = atoi(splchain2[1]) - 1;
				mytm.tm_mday = atoi(splchain2[2]);
				strSplit(splchain2[3], delim3, splchain3);
				mytm.tm_hour = atoi(splchain3[0]);
				mytm.tm_min = atoi(splchain3[1]);
				mytm.tm_sec = atoi(splchain3[2]);
				myTime = mktime(&mytm);
				switch (i) {
					case 0:
						statbuf->st_atime = myTime;
						break;
					case 1:
						statbuf->st_mtime = myTime;
						break;
					case 2:
						statbuf->st_ctime = myTime;
						break;
				}
			}
		}
	} 
	else {
		status = UNIV_MSS_STAT_ERR - errno;
		rodsLog (LOG_ERROR, "univMSSFileStat: cannot have stat informations for %s, status = %d",
				filename, status);
	}
 
	return (status);
	
}

/* univMSSFileRename - This function is to rename a file stored in the MSS. 
 */
int univMSSFileRename (rsComm_t *rsComm, char *oldFileName, char *newFileName) {
    
	int status;
	execCmd_t execCmdInp;
	char cmdArgv[HUGE_NAME_LEN] = "";
	execCmdOut_t *execCmdOut = NULL;
	
    bzero (&execCmdInp, sizeof (execCmdInp));
	rstrcpy(execCmdInp.cmd, UNIV_MSS_INTERF_SCRIPT, LONG_NAME_LEN);
	strcat(cmdArgv, "mv");
	strcat(cmdArgv, " ");
	strcat(cmdArgv, oldFileName);
	strcat(cmdArgv, " ");
	strcat(cmdArgv, newFileName);
	rstrcpy(execCmdInp.cmdArgv, cmdArgv, HUGE_NAME_LEN);
	rstrcpy(execCmdInp.execAddr, "localhost", LONG_NAME_LEN);
	status = _rsExecCmd(rsComm, &execCmdInp, &execCmdOut);

    if (status < 0) {
        status = UNIV_MSS_RENAME_ERR - errno;
		rodsLog (LOG_ERROR, "univMSSFileRename: rename of %s to error, status = %d",
         oldFileName, newFileName, status);
    }

    return (status);
}
