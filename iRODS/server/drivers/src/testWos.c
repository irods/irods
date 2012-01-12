/* wosFunctPP.cpp - C wrapper WOS functions */
#include <stdio.h>
#include <stdlib.h>
// JMC #include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "wosFunctPP.hpp"

int
main(int argc, char** argv)
{
    int status;
    char wosOidStr[MAX_NAME_LEN];
    char *tmpStr;

    *wosOidStr = '\0';

    tmpStr = malloc (MAX_NAME_LEN);
    snprintf (tmpStr, MAX_NAME_LEN, "%s=%s", WOS_HOST_ENV, WOS_HOST);
    putenv (tmpStr);
	free( tmpStr ); // JMC cppcheck - leak
    tmpStr = malloc (MAX_NAME_LEN);
    snprintf (tmpStr, MAX_NAME_LEN, "%s=%s", WOS_POLICY_ENV, WOS_POLICY);
    putenv (tmpStr);
	free( tmpStr ); // JMC cppcheck - leak


    status = wosSyncToArchPP (DEF_FILE_CREATE_MODE, 0, wosOidStr, 
      (char *) SRC_FILE_NAME, -1);

    if (status >= 0) {
	rodsLong_t fileSize;
	fileSize = wosGetFileSizePP (wosOidStr); 
	printf ("fileSize of %s = %lld\n", wosOidStr, fileSize);
        status = wosStageToCachePP (DEF_FILE_CREATE_MODE, 0, wosOidStr,
          (char *) DEST_FILE_NAME, fileSize);
	printf ("deleting %s\n", wosOidStr);
	status = wosFileUnlinkPP (wosOidStr);
    }
}

