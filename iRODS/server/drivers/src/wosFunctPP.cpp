/* wosFunctPP.cpp - C wrapper WOS functions */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <vector>

#include "rodsDef.h"
#include "rodsType.h"
#include "rodsErrorTable.h"
#include "wos_obj.hpp"
#include "wos_cluster.hpp"
#include "wosFunctPP.hpp"

using namespace wosapi;

WosClusterPtr MyWos;
WosPolicy MyPolicy;
int WosConnected = 0;

#if 0	/* moved */
int
main(int argc, char** argv)
{
    int status;
    char wosOidStr[MAX_NAME_LEN];

    *wosOidStr = '\0';
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
#endif

int
connectWos ()
{
    char *wosHost, *wosPolicy;

    if (WosConnected) return 0;

    if ((wosHost = getenv (WOS_HOST_ENV)) == NULL) {
	printf (
	  "connectWos: env %s not defined\n", WOS_HOST_ENV);
	return WOS_CONNECT_ERR;
    }
    if ((wosPolicy = getenv (WOS_POLICY_ENV)) == NULL) {
        printf (
          "connectWos: env %s not defined\n", WOS_POLICY_ENV);
        return WOS_CONNECT_ERR;
    }
    try {
        MyWos = WosCluster::Connect((char*) wosHost);
    }
    catch (WosException& e) {
      printf("WOS Exception: %s\n", e.what());
        return WOS_CONNECT_ERR;
    }
    MyPolicy = MyWos->GetPolicy(wosPolicy);
    WosConnected = 1;
    return 0;
}


int
wosSyncToArchPP (int mode, int flags, char *filename,
char *cacheFilename,  rodsLong_t dataSize)
{
    int status;
    int srcFd;
    struct stat statbuf;
    char *myBuf;
    rodsLong_t bytesCopied = 0;
    int bytesRead;
    WosStatus wosStatus;
    WosOID oid;

    status = stat (cacheFilename, &statbuf);

    if (status < 0) {
        status = UNIX_FILE_STAT_ERR - errno;
        printf ("wosSyncToArch: stat of %s error, status = %d\n",
         cacheFilename, status);
        return status;
    }

    if ((statbuf.st_mode & S_IFREG) == 0) {
        status = UNIX_FILE_STAT_ERR - errno;
        printf (
	  "wosSyncToArch: %s is not a file, status = %d\n",
          cacheFilename, status);
        return status;
    }
    if (dataSize > 0 && dataSize != statbuf.st_size) {
        printf (
          "wosSyncToArch: %s inp size %lld does not match actual size %lld\n",
         cacheFilename, (long long int) dataSize, 
         (long long int) statbuf.st_size);
        return SYS_COPY_LEN_ERR;
    }
    dataSize = statbuf.st_size;

    srcFd = open (cacheFilename, O_RDONLY, 0);
    if (srcFd < 0) {
        status = UNIX_FILE_OPEN_ERR - errno;
        printf (
         "wosSyncToArch: open error for %s, status = %d\n",
         cacheFilename, status);
        return status;
    }
    status = connectWos ();
    if (status < 0) {
	close (srcFd);
	return status;
    }
    if (dataSize <= WSIZE_MAX) {
	/* use single buf transfer */
        WosObjPtr wosObj;
	myBuf = (char *) malloc (dataSize);
	bytesRead = read (srcFd, (void *) myBuf, dataSize);
	close (srcFd);
	if (bytesRead != dataSize) {
	    printf (
	      "wosSyncToArchPP: bytesRead %d != dataSize %lld\n", 
	      bytesRead, dataSize);
		free( myBuf ); // JMC cppcheck - leak
	    return SYS_COPY_LEN_ERR;
	}
	wosObj = WosObj::Create();
        wosObj->SetData(myBuf, bytesRead);
        MyWos->Put(wosStatus, oid, MyPolicy, wosObj);
	free (myBuf);
        if (wosStatus != wosapi::ok) {
	    status = WOS_PUT_ERR - wosStatus.Value();
            printf(
	     "wosSyncToArchPP: Wos put %s error %d\n", cacheFilename, status);
	    return WOS_PUT_ERR - wosStatus.Value();
	}
    } else {
	/* for large files */
	WosPutStreamPtr wosPutStream;
	uint64_t offset = 0;
	rodsLong_t bytesLeft = dataSize;
	myBuf = (char *) malloc (WOS_STREAM_BUF_SIZE);
	int toRead;

	wosPutStream = MyWos->CreatePutStream(MyPolicy);
	while (bytesLeft > 0) {
	    if (bytesLeft > WOS_STREAM_BUF_SIZE) {
		toRead = WOS_STREAM_BUF_SIZE;
	    } else {
		toRead = bytesLeft;
	    }
	    bytesRead = read (srcFd, (void *) myBuf, toRead);
            if (bytesRead != toRead) {
                printf (
                  "wosSyncToArchPP: bytesRead %d != toRead %d\n",
                  bytesRead, toRead);
                close (srcFd);
                return SYS_COPY_LEN_ERR;
            }
	    wosPutStream->PutSpan(wosStatus, myBuf, offset, bytesRead);
	    offset += bytesRead;
	    bytesLeft -= bytesRead;
	    if (wosStatus != wosapi::ok) {
	        printf(
		  "wosSyncToArchPP: PutSpan %s error status = %d\n", 
		  cacheFilename, wosStatus.Value());
                close (srcFd);
                return WOS_STREAM_PUT_ERR - wosStatus.Value();
            }
	}
	close (srcFd);
	free (myBuf);
	wosPutStream->Close(wosStatus, oid);
        if (wosStatus != wosapi::ok) {
            printf(
              "wosSyncToArchPP: Close %s error status = %d\n",
              cacheFilename, wosStatus.Value());
            return WOS_STREAM_CLOSE_ERR - wosStatus.Value();
        }
    }
    if (*filename != '\0') {
printf ("deleting old oid %s\n", filename);
        MyWos->Delete(wosStatus, filename);
    }
    strncpy (filename, oid.c_str(), MAX_NAME_LEN);

    return 0;
}

int
wosStageToCachePP (int mode, int flags, char *filename,
char *cacheFilename, rodsLong_t dataSize)
{
    int destFd;
    WosStatus wosStatus;
    int bytesWritten;
    int status;
    WosOID oid;
    WosObjPtr wosObj;
    const void* dataPtr;
    uint64_t objLen;


    status = connectWos ();
    if (status < 0) {
        return status;
    }

    destFd = open (cacheFilename, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (destFd < 0) {
        status = UNIX_FILE_OPEN_ERR - errno;
	printf (
         "wosStageToCachePP: open error for cacheFilename %s, status = %d",
         cacheFilename, status);
        return status;
    }

    oid = filename;
    if (dataSize == 0) {
	close (destFd);
	return 0;
    } else if (dataSize > 0 && dataSize <= WSIZE_MAX) {

	MyWos->Get(wosStatus, oid, wosObj);
        if (wosStatus != wosapi::ok) {
            printf(
              "wosStageToCachePP: wos Get  %s error status = %d\n",
              cacheFilename, wosStatus.Value());
            return WOS_GET_ERR - wosStatus.Value();
        }
	if (wosObj) {
	    wosObj->GetData(dataPtr, objLen);
	    if (objLen != dataSize) {
                printf(
                  "wosStageToCachePP: %s GetData len %lld != dataSize %lld\n",
                  cacheFilename, (rodsLong_t) objLen, dataSize);
                return SYS_COPY_LEN_ERR;
	    }
        }
	bytesWritten = write (destFd, dataPtr, (int) objLen);
	close (destFd);
	if (bytesWritten != objLen) {
            printf(
              "wosStageToCachePP: %s bytesWritten %d != dataSize %lld\n",
              cacheFilename, bytesWritten, dataSize);
            return SYS_COPY_LEN_ERR;
	}
    } else {
	/* large files */
	WosGetStreamPtr wosGetStream;
	uint64_t fileLen;
	uint64_t objLen;
	uint64_t offset = 0;
	rodsLong_t bytesLeft;
	int toRead;

	try {
	    wosGetStream = MyWos->CreateGetStream(oid);
	}
	catch (WosE_ObjectNotFound& e) {
	    printf (
	      "wosStageToCachePP: CreateGetStream error for %s\n", 
	      cacheFilename);
	    close (destFd);
	    return WOS_STREAM_GET_ERR;
	}
	bytesLeft = fileLen = wosGetStream->GetLength();
        while (bytesLeft > 0) {
            if (bytesLeft > WOS_STREAM_BUF_SIZE) {
                toRead = WOS_STREAM_BUF_SIZE;
            } else {
                toRead = bytesLeft;
            }
	    wosGetStream->GetSpan(wosStatus, wosObj, offset, toRead);
            if (wosStatus != wosapi::ok) {
                printf(
                  "wosStageToCachePP: wos GetSpan  %s error status = %d\n",
                  cacheFilename, wosStatus.Value());
	        close (destFd);
                return WOS_STREAM_GET_ERR - wosStatus.Value();
            }
	    wosObj->GetData(dataPtr, objLen);

            bytesWritten = write (destFd, dataPtr, (int) objLen);
            if (bytesWritten != objLen) {
                printf(
                  "wosStageToCachePP: %s bytesWritten %d != objLen %d\n",
                  cacheFilename, bytesWritten, (int) objLen);
	        close (destFd);
                return SYS_COPY_LEN_ERR;
            }
            offset += bytesWritten;
            bytesLeft -= bytesWritten;
	}
	close (destFd);
    }
    return 0;
}

int
wosFileUnlinkPP (char *filename)
{
    int status;
    WosOID oid;
    WosStatus wosStatus;

    status = connectWos ();
    if (status < 0) {
        return status;
    }
    oid = filename;

    MyWos->Delete(wosStatus, oid);

    if (wosStatus != wosapi::ok) {
        printf(
          "wosFileUnlinkPP: wos Delete  %s error status = %d\n",
           filename, wosStatus.Value());
	return WOS_UNLINK_ERR - wosStatus.Value();
    } else {
	return 0;
    }
}

rodsLong_t
wosGetFileSizePP (char *filename) 
{
    int status;
    WosOID oid;
    WosStatus wosStatus;
    WosGetStreamPtr wosGetStream;
    uint64_t fileLen;


    status = connectWos ();
    if (status < 0) {
        return status;
    }
    oid = filename;

    try {
        wosGetStream = MyWos->CreateGetStream(oid, false);
    }
    catch (WosE_ObjectNotFound& e) {
        printf (
          "wosGetFileSizePP: CreateGetStream error for %s\n",
          filename);
        return WOS_STAT_ERR;
    }
    fileLen = wosGetStream->GetLength();

    return (rodsLong_t) fileLen;
}
 
