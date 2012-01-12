/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileRead.c - server routine that handles the fileRead
 * API
 */

/* script generated code */
#include "fileRead.h"
#include "fileClose.h"
#include "streamRead.h"
#include "miscServerFunct.h"
#include "rsGlobalExtern.h"

int
rsStreamRead (rsComm_t *rsComm, fileReadInp_t *streamReadInp,
bytesBuf_t *streamReadOutBBuf)
{
    int fileInx = streamReadInp->fileInx;
    int status;

    if (fileInx < 3 || fileInx >= NUM_FILE_DESC) {
        rodsLog (LOG_ERROR,
         "rsStreamRead: fileInx %d out of range", fileInx);
        return (SYS_FILE_DESC_OUT_OF_RANGE);
    }
    if (FileDesc[fileInx].inuseFlag != FD_INUSE) return SYS_BAD_FILE_DESCRIPTOR;

    if (FileDesc[fileInx].fileName == NULL) return SYS_INVALID_FILE_PATH;
    if (strcmp (FileDesc[fileInx].fileName, STREAM_FILE_NAME) != 0) {
        rodsLog (LOG_ERROR,
	  "rsStreamRead: fileName %s is invalid for stream",
	  FileDesc[fileInx].fileName);
	return SYS_INVALID_FILE_PATH;
    }
    status = rsFileRead (rsComm, streamReadInp, streamReadOutBBuf);

#if 0	/* need to close explicitly */
    if (status <= 0) {
	fileCloseInp_t fileCloseInp;
	/* the end or error */
	fileCloseInp.fileInx = fileInx;
	rsFileClose (rsComm, &fileCloseInp);
    }
#endif
    return status;
}

