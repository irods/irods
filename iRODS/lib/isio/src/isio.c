/* 
   irods standard i/o emulation library  (initial version)
   
   This package converts Unix standard I/O calls (stdio.h: fopen,
   fread, fwrite, etc) to the equivalent irods calls.  To convert an
   application you just need to add an include statement (isio.h) and
   recompile and relink with irods libraries.  fopen calls with input
   filenames that include the IRODS_PREFIX string ('irods:') will be
   opened and handled as iRODS files; without the prefix this package
   will call the fopen family to handle them as regular local files.

   Like the fopen family, this library does some caching to avoid
   small I/O (network) calls, greatly improving performance.

   The user callable functions are defined in the isio.h and are of
   the form irodsNAME, such as irodsfopen.  Internal function names
   begin with 'isio'.

   The irods environment is assummed.  That is, like i-commands, this
   library needs to read the user's .irodsEnv and authentication files
   to be able to connect to an iRODS server.

   See the ../include/isio.h, ../test*.c files, and ../Makefile for
   more information.

 */

#include <stdio.h>
#include "rodsClient.h"
#include "dataObjRead.h"

#define IRODS_PREFIX "irods:"
#define ISIO_MAX_OPEN_FILES 20
#define ISIO_MIN_OPEN_FD 5

/* The following two numberic values are also used by the
   test script (modified to be smaller as a test) */
#define ISIO_INITIAL_BUF_SIZE  65536
#define ISIO_MAX_BUF_SIZE    2097152

int debug=0;

long openFiles[ISIO_MAX_OPEN_FILES]={0,0,0,0,0,0,0,0,0,0,
				     0,0,0,0,0,0,0,0,0,0};

struct {
   FILE *fd;
   char *base;
   int bufferSize;
   char *ptr;
   int count;
   char usingUsersBuffer; /* y or n when active */
   int written; /* contains count of bytes written */
} cacheInfo[ISIO_MAX_OPEN_FILES];



static setupFlag=0;
char localZone[100]="";
rcComm_t *Comm;
rodsEnv myRodsEnv;

int
isioSetup() {
   int status;
   rErrMsg_t errMsg;
   char *mySubName;
   char *myName;

   if (debug) printf("isioSetup\n");

   status = getRodsEnv (&myRodsEnv);
   if (status < 0) {
      rodsLogError(LOG_ERROR, status, "isioSetup: getRodsEnv error.");
   }
   Comm = rcConnect (myRodsEnv.rodsHost, myRodsEnv.rodsPort, 
		     myRodsEnv.rodsUserName,
                     myRodsEnv.rodsZone, 0, &errMsg);

   if (Comm == NULL) {
      myName = rodsErrorName(errMsg.status, &mySubName);
      rodsLog(LOG_ERROR, "rcConnect failure %s (%s) (%d) %s",
	      myName,
	      mySubName,
	      errMsg.status,
	      errMsg.msg);
      status = errMsg.status;
      return(status);
   }

   status = clientLogin(Comm);
   if (status==0) {
      setupFlag=1;
   }
   return(status);
}

FILE *isioFileOpen(char *filename, char *modes) {
   int i;
   int status;
   dataObjInp_t dataObjInp;
   char **outPath;

   if (debug) printf("isioFileOpen: %s\n", filename);

   if (setupFlag==0) {
      status = isioSetup();
      if (status) return(NULL);
   }

   for (i=ISIO_MIN_OPEN_FD;i<ISIO_MAX_OPEN_FILES;i++) {
     if (openFiles[i]==0) break;
   }
   if (i>=ISIO_MAX_OPEN_FILES) {
     fprintf(stderr,"Too many open files in isioFileOpen\n");
     return(NULL);
   }

   memset (&dataObjInp, 0, sizeof (dataObjInp));
   status = parseRodsPathStr (filename , &myRodsEnv,
			      dataObjInp.objPath);
   if (status < 0) {
     rodsLogError (LOG_ERROR, status, "isioFileOpen");
     return(NULL);
   }

   /* Set the openFlags based on the input mode (incomplete
    * currently) // */
   dataObjInp.openFlags = O_RDONLY;
   if (strncmp(modes,"w",1)==0) {
      dataObjInp.openFlags = O_WRONLY;
      /* Need to handle other cases sometime */
   }
   if (strncmp(modes,"r+",2)==0) {
      dataObjInp.openFlags = O_RDWR;
   }

   status = rcDataObjOpen (Comm, &dataObjInp);

   if (status==CAT_NO_ROWS_FOUND &&
       dataObjInp.openFlags == O_WRONLY) {
      status = rcDataObjCreate(Comm, &dataObjInp);
   }
   if (status < 0) {
      rodsLogError (LOG_ERROR, status, "isioFileOpen");
      return(NULL);
   }
   openFiles[i]=status;
   cacheInfo[i].base=(char *)malloc(sizeof(char) * ISIO_INITIAL_BUF_SIZE);
   if (cacheInfo[i].base==NULL) {
      fprintf(stderr,"Memory Allocation error\n");
      return(NULL);
   }
   cacheInfo[i].bufferSize = sizeof(char) * ISIO_INITIAL_BUF_SIZE;
   cacheInfo[i].ptr=cacheInfo[i].base;
   cacheInfo[i].count = 0;
   cacheInfo[i].usingUsersBuffer = 'n';
   cacheInfo[i].written = 0;
   return((FILE *)i);
}

FILE *irodsfopen(char *filename, char *modes) {
   int status;
   int len;

   if (debug) printf("irodsfopen: %s\n", filename);

   len = strlen(IRODS_PREFIX);
   if (strncmp(filename,IRODS_PREFIX,len)==0) {
     return(isioFileOpen(filename+len, modes));
   }
   else {
     return(fopen(filename, modes));
   }
}

int
isioFillBuffer(int fileIndex) {
   int status, i;
   openedDataObjInp_t dataObjReadInp;
   bytesBuf_t dataObjReadOutBBuf;

   if (debug) printf("isioFillBuffer: %d\n", fileIndex);

   i = fileIndex;
   dataObjReadOutBBuf.buf = cacheInfo[i].base;
   dataObjReadOutBBuf.len = cacheInfo[i].bufferSize;

   memset(&dataObjReadInp, 0, sizeof (dataObjReadInp));

   dataObjReadInp.l1descInx = openFiles[fileIndex];
   dataObjReadInp.len = cacheInfo[i].bufferSize;

   status = rcDataObjRead (Comm, &dataObjReadInp,
			   &dataObjReadOutBBuf);

   if (debug) printf("isioFillBuffer rcDataObjRead stat: %d\n", status);
   if (status < 0) return(status);

   cacheInfo[i].ptr = cacheInfo[i].base;
   cacheInfo[i].count = status;

   return(0);
}

int
isioFileRead(int fileIndex, void *buffer, int maxToRead) {
   int status, i;
   int reqSize;
   char *myPtr;
   openedDataObjInp_t dataObjReadInp;
   bytesBuf_t dataObjReadOutBBuf;
   int count;
   int toMove;
   int newBufSize;

   if (debug) printf("isioFileRead: %d\n", fileIndex);

   /* If the buffer had been used for writing, flush it */
   status = isioFlush(fileIndex);
   if (status<0) return(status);

   reqSize = maxToRead;
   myPtr = buffer;
   if (cacheInfo[fileIndex].count > 0) {
      if (cacheInfo[fileIndex].count >= reqSize) {
	 memcpy(myPtr,cacheInfo[fileIndex].ptr, reqSize);
	 cacheInfo[fileIndex].ptr += reqSize;
	 cacheInfo[fileIndex].count -= reqSize;
	 return(maxToRead);
      }
      else {
	 memcpy(myPtr,cacheInfo[fileIndex].ptr, cacheInfo[fileIndex].count);
	 cacheInfo[fileIndex].ptr += cacheInfo[fileIndex].count;
	 reqSize -=  cacheInfo[fileIndex].count;
	 myPtr += cacheInfo[fileIndex].count;
	 cacheInfo[fileIndex].count = 0;
      }
   }

   newBufSize=(2*maxToRead)+8;
   i = fileIndex;

   if (cacheInfo[i].usingUsersBuffer == 'y') {
      /* previous time we used user's buffer, this time either
         allocate a new one or use their's this time too. */
      cacheInfo[i].bufferSize = 0;
   }

   if (newBufSize > cacheInfo[i].bufferSize) {
      if (newBufSize<=ISIO_MAX_BUF_SIZE) {
	 if (cacheInfo[i].usingUsersBuffer=='n') {
	    if (debug) printf("isioFileRead calling free\n");
	    free(cacheInfo[i].base);
	 }
	 if (debug) printf("isioFileRead calling malloc\n");
	 cacheInfo[i].base=(char *)malloc(newBufSize);
	 if (cacheInfo[i].base==NULL) {
	    fprintf(stderr,"Memory Allocation error\n");
	    return(0);
	 }
	 cacheInfo[i].bufferSize = newBufSize;
	 cacheInfo[i].usingUsersBuffer = 'n';
      }
      else {
         /* Use user's buffer */
	 cacheInfo[i].base=buffer;
	 cacheInfo[i].bufferSize = maxToRead;
	 cacheInfo[i].usingUsersBuffer = 'y';
      }
      cacheInfo[i].ptr=cacheInfo[i].base;
      cacheInfo[i].count = 0;
   }

   status = isioFillBuffer(fileIndex);
   if (status<0) return(status);

   if (cacheInfo[i].usingUsersBuffer=='y') {
      count = cacheInfo[fileIndex].count;
      cacheInfo[fileIndex].count = 0;
      if (debug) printf("isioFileRead return1: %d\n", count);
      return(count);
   }
   if (cacheInfo[fileIndex].count > 0) {
      toMove = reqSize;
      if (cacheInfo[fileIndex].count < reqSize) {
	 toMove=cacheInfo[fileIndex].count;
      }
      memcpy(myPtr,cacheInfo[fileIndex].ptr, toMove);
      cacheInfo[fileIndex].ptr += toMove;
      cacheInfo[fileIndex].count -= toMove;
      myPtr += toMove;
      reqSize -= toMove;
   }
   count = (void *)myPtr-buffer;
   if (debug) printf("isioFileRead return2: %d\n", count);
   return(count);

#if 0
   /* equivalent without caching */
   dataObjReadOutBBuf.buf = buffer;
   dataObjReadOutBBuf.len = maxToRead;

   memset(&dataObjReadInp, 0, sizeof (dataObjReadInp));

   dataObjReadInp.l1descInx = openFiles[fileIndex];
   dataObjReadInp.len = maxToRead;

   return(rcDataObjRead (Comm, &dataObjReadInp,
			 &dataObjReadOutBBuf));
#endif
}

size_t irodsfread(void *buffer, size_t itemsize, int nitems, FILE *fi_stream) {
   int i;
   i = (int)fi_stream;

   if (debug) printf("isiofread: %d\n", i);

   if (i<ISIO_MAX_OPEN_FILES && i>=ISIO_MIN_OPEN_FD && openFiles[i]>0) {
      return(isioFileRead(i, buffer, itemsize*nitems));
   }
   else {
      return(fread(buffer, itemsize, nitems, fi_stream));
   }
}

int
isioFileWrite(int fileIndex, void *buffer, int countToWrite) {
   int status;
   int spaceInBuffer;
   int newBufSize;

   openedDataObjInp_t dataObjWriteInp;
   bytesBuf_t dataObjWriteOutBBuf;

   if (debug) printf("isioFileWrite: %d\n", fileIndex);

   if (cacheInfo[fileIndex].count > 0) {
      /* buffer has read data in it, so seek to where the
         the app thinks the pointer is and disgard the buffered
         read data */
      long offset;
      offset = - cacheInfo[fileIndex].count;
      status = isioFileSeek(fileIndex, offset, SEEK_CUR);
      if (status) return(status);
      cacheInfo[fileIndex].ptr=cacheInfo[fileIndex].base;
      cacheInfo[fileIndex].count = 0;
   }

   spaceInBuffer = cacheInfo[fileIndex].bufferSize -
                   cacheInfo[fileIndex].written;
                   
   if (debug) printf("isioFileWrite: spaceInBuffer %d\n", spaceInBuffer);
   if (countToWrite < spaceInBuffer) {
      /* Fits in the buffer, just cache it */
      if (debug) printf("isioFileWrite: caching 1 %x %d\n", 
			(int) cacheInfo[fileIndex].ptr, countToWrite);
      memcpy(cacheInfo[fileIndex].ptr, buffer, countToWrite);
      cacheInfo[fileIndex].ptr += countToWrite;
      cacheInfo[fileIndex].written += countToWrite;
      return(countToWrite);
   }

   status = isioFlush(fileIndex); /* if anything is buffered, flush it */
   if (status < 0) return(status);

   if (countToWrite > ISIO_MAX_BUF_SIZE) {
      /* Too big to cache, just send it */
      dataObjWriteOutBBuf.buf = buffer;
      dataObjWriteOutBBuf.len = countToWrite;

      memset(&dataObjWriteInp, 0, sizeof (dataObjWriteInp));

      dataObjWriteInp.l1descInx = openFiles[fileIndex];
      dataObjWriteInp.len = countToWrite;

      status = rcDataObjWrite (Comm, &dataObjWriteInp,
				&dataObjWriteOutBBuf);
      if (debug) printf("isioFileWrite: rcDataWrite 2 %d\n", status);
      if (status < 0) return(status);

      return(status);  /* total bytes written */
   }

   newBufSize=(2*countToWrite)+8;  /* Possible next size */
   if (newBufSize > ISIO_MAX_BUF_SIZE) {
      newBufSize = ISIO_MAX_BUF_SIZE;
   }

   if (newBufSize > cacheInfo[fileIndex].bufferSize) {
       /* free old and make new larger buffer */
      int i=fileIndex;
      if (cacheInfo[i].usingUsersBuffer=='n') {
	 if (debug) printf("isioFilewrite calling free\n");
	 free(cacheInfo[i].base);
      }
      if (debug) printf("isioFilewrite calling malloc %d\n",
			newBufSize);
      cacheInfo[i].base=(char *)malloc(newBufSize);
      if (cacheInfo[i].base==NULL) {
	 fprintf(stderr,"Memory Allocation error\n");
	 return(0);
      }
      cacheInfo[i].bufferSize = newBufSize;
      cacheInfo[i].usingUsersBuffer = 'n';
      cacheInfo[i].ptr=cacheInfo[i].base;
   }

   /* Now it fits in the buffer, so cache it */
   if (debug) printf("isioFileWrite: caching 2 %x %d\n", 
		     (int) cacheInfo[fileIndex].ptr, countToWrite);
   memcpy(cacheInfo[fileIndex].ptr, buffer, countToWrite);
   cacheInfo[fileIndex].ptr += countToWrite;
   cacheInfo[fileIndex].count -= countToWrite; //??
   cacheInfo[fileIndex].written += countToWrite;
   return(countToWrite);

#if 0
/* non-caching version */
   dataObjWriteOutBBuf.buf = buffer;
   dataObjWriteOutBBuf.len = countToWrite;

   memset(&dataObjWriteInp, 0, sizeof (dataObjWriteInp));

   dataObjWriteInp.l1descInx = openFiles[fileIndex];
   dataObjWriteInp.len = countToWrite;

   return(rcDataObjWrite (Comm, &dataObjWriteInp,
			 &dataObjWriteOutBBuf));
#endif
}

size_t 
irodsfwrite(void *buffer, size_t itemsize, int nitems, FILE *fi_stream) {
   int i;
   i = (int)fi_stream;
   if (debug) printf("irodsfwrite: %d\n", i);
   if (i<ISIO_MAX_OPEN_FILES && i>=ISIO_MIN_OPEN_FD && openFiles[i]>0) {
      return(isioFileWrite(i, buffer, itemsize*nitems));
   }
   else {
      return(fwrite(buffer, itemsize, nitems, fi_stream));
   }
}

int
isioFileClose(int fileIndex) {
   openedDataObjInp_t dataObjCloseInp;
   int i, status;

   if (debug) printf("isioFileClose: %d\n", fileIndex);

   /* If the buffer had been used for writing, flush it */
   status = isioFlush(fileIndex);
   if (status<0) return(status);


   memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
   dataObjCloseInp.l1descInx = openFiles[fileIndex];

   openFiles[fileIndex]=0;

   i = fileIndex;
   if (cacheInfo[i].usingUsersBuffer == 'n') {
      if (debug) printf("isioFileClose calling free\n");
      free(cacheInfo[i].base);
   }

   cacheInfo[i].usingUsersBuffer = ' ';

   return(rcDataObjClose(Comm, &dataObjCloseInp));
}

size_t irodsfclose(FILE *fi_stream) {
   int i;
   i = (int)fi_stream;
   if (debug) printf("isiofclose: %d\n", i);
   if (i<ISIO_MAX_OPEN_FILES && i>=ISIO_MIN_OPEN_FD && openFiles[i]>0) {
      return(isioFileClose(i));
   }
   else {
      return(fclose(fi_stream));
   }
}


int
isioFileSeek(int fileIndex, long offset, int whence) {
   openedDataObjInp_t seekParam;
   fileLseekOut_t* seekResult = NULL;
   int status;
   if (debug) printf("isioFileSeek: %d\n", fileIndex);
   memset( &seekParam,  0, sizeof(openedDataObjInp_t) );
   seekParam.l1descInx = openFiles[fileIndex];
   seekParam.offset  = offset;
   seekParam.whence  = whence;
   status = rcDataObjLseek(Comm, &seekParam, &seekResult );
   if ( status < 0 ) {
      rodsLogError (LOG_ERROR, status, "isioFileSeek");
   }
   return(status);
}

int
irodsfseek(FILE *fi_stream, long offset, int whence) {
   int i;
   i = (int)fi_stream;
   if (debug) printf("isiofseek: %d\n", i);
   if (i<ISIO_MAX_OPEN_FILES && i>=ISIO_MIN_OPEN_FD && openFiles[i]>0) {
      return(isioFileSeek(i,offset,whence));
   }
   else {
      return(fseek(fi_stream, offset, whence));
   }
}

int
isioFlush(int fileIndex) {
   int i;
   int status;
   openedDataObjInp_t dataObjWriteInp;
   bytesBuf_t dataObjWriteOutBBuf;

   i = fileIndex;
   if (debug) printf("isioFlush: %d\n", i);
   if (cacheInfo[i].written > 0) {

      dataObjWriteOutBBuf.buf = cacheInfo[i].base;
      dataObjWriteOutBBuf.len = cacheInfo[i].written;

      memset(&dataObjWriteInp, 0, sizeof (dataObjWriteInp));

      dataObjWriteInp.l1descInx = openFiles[fileIndex];
      dataObjWriteInp.len = cacheInfo[i].written;

      if (debug) printf("isioFlush: writing %d\n", 
			cacheInfo[i].written);
      status = rcDataObjWrite (Comm, &dataObjWriteInp,
			       &dataObjWriteOutBBuf);
      if (status >=0) {
	 cacheInfo[i].ptr = cacheInfo[i].base;
	 cacheInfo[i].written = 0;
      }
      return(status);

   }
   return(0);
}


int
irodsfflush(FILE *fi_stream) {
   int i;
   i = (int)fi_stream;
   if (debug) printf("isiofflush: %d\n", i);
   if (i<ISIO_MAX_OPEN_FILES && i>=ISIO_MIN_OPEN_FD && openFiles[i]>0) {
      return(isioFlush(i));
   }
   else {
      return(fflush(fi_stream));
   }
}

int
isioFilePutc(int inchar, int fileIndex) {
   int mychar;
   mychar = inchar;
   return (isioFileWrite(fileIndex, (void*)&mychar, 1));
}

int
irodsfputc(int inchar, FILE *fi_stream) {
   int i;
   i = (int)fi_stream;
   if (debug) printf("isiofputc: %d\n", i);
   if (i<ISIO_MAX_OPEN_FILES && i>=ISIO_MIN_OPEN_FD && openFiles[i]>0) {
      return(isioFilePutc(inchar, i));
   }
   else {
      return(fputc(inchar, fi_stream));
   }
}

int
isioFileGetc(int fileIndex) {
   int mychar=0;
   int status;
   status = isioFileRead(fileIndex, (void*)&mychar, 1);
   if (status==0) return(EOF);
   if (status<0) return(status);
   return(mychar);
}

int
irodsfgetc(FILE *fi_stream) {
   int i;
   i = (int)fi_stream;
   if (debug) printf("isiofgetc: %d\n", i);
   if (i<ISIO_MAX_OPEN_FILES && i>=ISIO_MIN_OPEN_FD && openFiles[i]>0) {
      return(isioFileGetc(i));
   }
   else {
      return(fgetc(fi_stream));
   }
}


int
irodsexit(int exitValue) {
   int status;
   if (debug) printf("irodsexit: %d\n", exitValue);
   if (setupFlag>0) {
      status = rcDisconnect(Comm);
   }
   exit(exitValue);
}
