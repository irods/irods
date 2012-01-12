/*** Copyright (c) 2010 Data Intensive Cyberinfrastructure Foundation. All rights reserved.    ***
 *** For full copyright notice please refer to files in the COPYRIGHT directory                ***/
/* Written by Jean-Yves Nief of CCIN2P3 and copyright assigned to Data Intensive Cyberinfrastructure Foundation */

#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "fsckUtil.h"
#include "miscUtil.h"

int
fsckObj (rcComm_t *conn, rodsArguments_t *myRodsArgs, rodsPathInp_t *rodsPathInp, char hostname[LONG_NAME_LEN])
{
	char inpPath[LONG_NAME_LEN] = "";
	char *inpPathO, *lastChar;
#ifndef USE_BOOST_FS
	struct stat sbuf;
#endif
	int lenInpPath, status;
	
	if ( rodsPathInp->numSrc != 1 ) {
		rodsLog (LOG_ERROR, "fsckObj: gave %i input source path, should give one and only one", rodsPathInp->numSrc);
		status = USER_INPUT_PATH_ERR;
	}
	else {
		inpPathO = rodsPathInp->srcPath[0].outPath;
#ifdef USE_BOOST_FS
                        path p (inpPathO);
                        if (exists(p)) {
                            /* don't do anything if it is symlink */
                            if (is_symlink (p)) return 0;
#else
		status = lstat(inpPathO, &sbuf);
		if ( status == 0 ) {
#endif	/* USE_BOOST_FS */
			/* remove any trailing "/" from inpPathO */
			lenInpPath = strlen(inpPathO);
			lastChar = strrchr(inpPathO, '/');
			if ( strlen(lastChar) == 1 ) {
				lenInpPath = lenInpPath - 1;
			}
			strncpy(inpPath, inpPathO, lenInpPath);
#ifdef USE_BOOST_FS
                        if (is_directory(p)) {
#else
			if ( S_ISDIR(sbuf.st_mode) == 1 ) {   /* check if it is not included into a mounted collection */
#endif
				status = checkIsMount(conn, inpPath);
				if ( status != 0 ) {  /* if it is part of a mounted collection, abort */
					printf("The directory %s or one of its subdirectories to be checked is declared as being \
							used for a mounted collection: abort!\n", inpPath);
					return (status);
				}
			}
			status = fsckObjDir(conn, myRodsArgs, inpPath, hostname);
		}
		else {
			status = USER_INPUT_PATH_ERR;
			rodsLog (LOG_ERROR, "fsckObj: %s does not exist", inpPathO);
		}
	}
	
	return (status);
}

int 
fsckObjDir (rcComm_t *conn, rodsArguments_t *myRodsArgs, char *inpPath, char *hostname)
{
#ifndef USE_BOOST_FS
	DIR *dirPtr;
	struct dirent *myDirent;
	struct stat sbuf;
#endif
	int status;
	char fullPath[LONG_NAME_LEN] = "\0";
	
#ifndef USE_BOOST_FS
	dirPtr = opendir (inpPath);
#endif
	/* check if it is a directory */
#ifdef USE_BOOST_FS
        path srcDirPath (inpPath);
        if (is_symlink (srcDirPath)) {
            /* don't do anything if it is symlink */
            return 0;
        } else if (is_directory(srcDirPath)) {
#else   /* USE_BOOST_FS */
	lstat(inpPath, &sbuf);
	if ( S_ISDIR(sbuf.st_mode) == 1 ) {
		if ( dirPtr == NULL ) {
			return (-1);
		}
#endif
	}
	else {
		status = chkObjConsistency(conn, myRodsArgs, inpPath, hostname);
		return (status);
	}
#ifdef USE_BOOST_FS
        directory_iterator end_itr; // default construction yields past-the-end
        for (directory_iterator itr(srcDirPath); itr != end_itr;++itr) {
            path cp = itr->path();
            snprintf (fullPath, MAX_NAME_LEN, "%s",
              cp.c_str ());
#else
	
	while ( ( myDirent = readdir(dirPtr) ) != NULL ) {
        if ( strcmp(myDirent->d_name, ".") == 0 || strcmp(myDirent->d_name, "..") == 0 ) {
			continue;
        }
        strcpy(fullPath, inpPath);
        strcat(fullPath, "/");
        strcat(fullPath, myDirent->d_name);
#endif
#ifdef USE_BOOST_FS
        if (is_symlink (cp)) {
            /* don't do anything if it is symlink */
            continue;
        } else if (is_directory(cp)) {
#else
        lstat(fullPath, &sbuf);
		if ( S_ISDIR(sbuf.st_mode) == 1 ) {
#endif
			if ( myRodsArgs->recursive == True ) {
				status = fsckObjDir(conn, myRodsArgs, fullPath, hostname);
			}
        }
        else {
			status = chkObjConsistency(conn, myRodsArgs, fullPath, hostname);
        }
	}
#ifndef USE_BOOST_FS
   closedir (dirPtr);
#endif
   return (status);
	
}

int
chkObjConsistency (rcComm_t *conn, rodsArguments_t *myRodsArgs, char *inpPath, char *hostname)
{
	int objSize, srcSize, status;
	genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
	char condStr[MAX_NAME_LEN], locChksum[NAME_LEN], *objChksum, *objName, *objPath;
#ifndef USE_BOOST_FS
	struct stat sbuf;
#endif

	/* retrieve the local file size */
#ifdef USE_BOOST_FS
        path p (inpPath);
        /* don't do anything if it is symlink */
        if (is_symlink (p)) return 0;
	srcSize = file_size(p);
#else   /* USE_BOOST_FS */
	lstat(inpPath, &sbuf);
	srcSize = sbuf.st_size;
#endif
	
	/* retrieve object size and checksum in iRODS */
	memset (&genQueryInp, 0, sizeof (genQueryInp));
	addInxIval(&genQueryInp.selectInp, COL_DATA_NAME, 1);
	addInxIval(&genQueryInp.selectInp, COL_COLL_NAME, 1);
	addInxIval(&genQueryInp.selectInp, COL_DATA_SIZE, 1);
	addInxIval(&genQueryInp.selectInp, COL_D_DATA_CHECKSUM, 1);
	genQueryInp.maxRows = MAX_SQL_ROWS;
	
	snprintf (condStr, MAX_NAME_LEN, "='%s'", inpPath);
    addInxVal (&genQueryInp.sqlCondInp, COL_D_DATA_PATH, condStr);
    snprintf (condStr, MAX_NAME_LEN, "like '%s%s' || ='%s'", hostname, "%", hostname);
	addInxVal (&genQueryInp.sqlCondInp, COL_R_LOC, condStr);
	
	status =  rcGenQuery (conn, &genQueryInp, &genQueryOut);
	if (status != CAT_NO_ROWS_FOUND) {
		objName = genQueryOut->sqlResult[0].value;
		objPath = genQueryOut->sqlResult[1].value;
		objSize = atoi(genQueryOut->sqlResult[2].value);
		objChksum = genQueryOut->sqlResult[3].value;
		if ( srcSize == objSize ) {
			if ( myRodsArgs->verifyChecksum == True ) {
				if ( strcmp(objChksum,"") != 0 ) {
					status = chksumLocFile(inpPath, locChksum);
					if ( status == 0 ) {
						if ( strcmp(locChksum, objChksum) != 0 ) {
							printf ("CORRUPTION: local file %s checksum not consistent with \
iRODS object %s/%s checksum.\n", inpPath, objPath, objName);
						}
					}
					else {
						printf ("ERROR: unable to compute checksum for local file %s.\n", inpPath);
					}
				}
				else {
					printf ("WARNING: checksum not available for iRODS object %s/%s, no checksum comparison \
possible with local file %s .\n", objPath, objName, inpPath);
				}
			}
		}
		else {
			printf ("CORRUPTION: local file %s size not consistent with iRODS object %s/%s size.\n",\
				inpPath, objPath, objName);
		}
	}
	
	clearGenQueryInp(&genQueryInp);
	freeGenQueryOut(&genQueryOut);
	
	return (status);
	
}
