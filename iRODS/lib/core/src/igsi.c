/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
These functions provide a relatively simple interface to the Globus
GSI system for authentication.  It is used by the iRODS agent to
authenticate the irods clients when the user requests GSI
authentication.  Globus GSI follows GSS-API security service standard
and builds on top of an SSL library (see the Globus documentation).
GSI uses X.509 Certificates.

All functions called from outside have names that start with 'igsi'.
Internal routines start with '_igsi'.

Also see rsGsiAuthRequest.c, which includes the igsiServersideAuth
function which calls igsiEstablishContextServerside, and then queries and
sets up the login session.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "rods.h"
#include "rcGlobalExtern.h"

#if defined(GSI_AUTH)
#include <gssapi.h>
#endif

#include <stdio.h>
#include <sys/time.h>           /* for IGSI_TIMING only */
#include <string.h>

#include "rodsErrorTable.h"

rError_t *igsi_rErrorPtr;

/* #define IGSI_TIMING 1  (uncomment this out for timing) */


void _igsiTime(struct timeval *tdiff, struct timeval *t1,
                struct timeval *t0); 
#if defined(GSI_AUTH)
extern gss_OID gss_nt_service_name;
extern int OBJ_create(char *input1, char *input2, char *input3);
#endif

#define gss_nt_service_name_gsi 0

#define SCRATCH_BUFFER_SIZE 20000
char igsiScratchBuffer[SCRATCH_BUFFER_SIZE];

static int igsiDebugFlag = 0;

#define MAX_FDS 32
#if defined(GSI_AUTH)
static int igsiTokenHeaderMode = 1;  /* 1 is the normal mode, 
 0 means running in a non-token-header mode, ie Java; dynamically cleared. */
static gss_cred_id_t myCreds = GSS_C_NO_CREDENTIAL;
static gss_ctx_id_t context[MAX_FDS] = {
    GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT,
    GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT,
    GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT,
    GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT,
    GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT,
    GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT,
    GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT,
    GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT, GSS_C_NO_CONTEXT
};

static char *inCacheBuf[MAX_FDS] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
};

static int inCacheBufState[MAX_FDS] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0
};

static int inCacheBufLen[MAX_FDS];
static int inCacheBufWriteIndex[MAX_FDS];
static int inCacheBufReadIndex[MAX_FDS];

#endif  /* end of #if defined(GSI_AUTH) */

unsigned int context_flags;

/* Function for time test 
 * Returns the difference between start time and end time in tdiff 
 * struct in seconds */
void _igsiTime(struct timeval *tdiff, struct timeval *endTime,
                struct timeval *startTime)
{
    tdiff->tv_sec = endTime->tv_sec - startTime->tv_sec;
    tdiff->tv_usec = endTime->tv_usec - startTime->tv_usec;
    if (tdiff->tv_usec < 0) {
        tdiff->tv_sec--;
        tdiff->tv_usec += 1000000;
    }
}

/*
 Write a buffer to the network, continuing with subsequent writes if
 the write system call returns with only some of it sent.
 */
#if defined(GSI_AUTH)
static int _igsiWriteAll(int fd, char *buf, unsigned int nbyte)
{
    int ret;
    char *ptr;

    for (ptr = buf; nbyte; ptr += ret, nbyte -= ret) {
        ret = write(fd, ptr, nbyte);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            return (ret);
        } else if (ret == 0) {
            return (ptr - buf);
        }
    }
    if (igsiDebugFlag > 0)
        fprintf(stderr, "_igsiWriteAll, wrote=%d\n", ptr - buf);
    return (ptr - buf);
}
#endif

/* 
 Read a buffer, continuing until it is filled.
 */
static int _igsiReadAll(int fd, char *buf, unsigned int nbyte)
{
    int ret;
    char *ptr;

    for (ptr = buf; nbyte; ptr += ret, nbyte -= ret) {
        ret = read(fd, ptr, nbyte);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            return (GSI_SOCKET_READ_ERROR);
        } else if (ret == 0) {
            return (ptr - buf);
        }
    }

    return (ptr - buf);
}

/*
 Send a token (which is a buffer and a length); 
 write the token length (as a network long) and then the
 token data on the file descriptor.  It returns 0 on success, and
 -1 if an error occurs or if it could not write all the data.
*/
#if defined(GSI_AUTH)
int _gsiSendToken(int fd, gss_buffer_t tok)
{
    int len;
    unsigned int ret;
    char *cp;
    int status;

    if (igsiDebugFlag > 0)
        fprintf(stderr, "sending tok->length=%d\n", tok->length);

    if (igsiTokenHeaderMode) {
        len = htonl(tok->length);

        cp = (char *) &len;
        if (sizeof(len) > 4)
            cp += sizeof(len) - 4;
        ret = _igsiWriteAll(fd, cp, 4);
        if (ret < 0) {
            return GSI_ERROR_SENDING_TOKEN_LENGTH;
        } else if (ret != 4) {
            status = GSI_ERROR_SENDING_TOKEN_LENGTH;
	    rodsLogAndErrorMsg( LOG_ERROR, igsi_rErrorPtr, status,
				"sending token data: %d of %d bytes written",
				ret, tok->length);
	    return status;
        }
    }

    ret = _igsiWriteAll(fd, (char *)tok->value, tok->length);
    if (ret < 0) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    } else if (ret != tok->length) {
       status = GSI_ERROR_SENDING_TOKEN_LENGTH;
       rodsLogAndErrorMsg( LOG_ERROR, igsi_rErrorPtr, status,
			   "sending token data2: %d of %d bytes written",
			   ret, tok->length);
       return status;
    }

    return 0;
}
#endif

/*
 read the token hdr
 */
int _igsiRcvTokenHeader(int fd)
{
    int length;
    char *cp;
    int ret;
    int status;

    length = 0;
    cp = (char *) &length;
    if (sizeof(length) > 4)
        cp += sizeof(length) - 4;
    ret = _igsiReadAll(fd, cp, 4);
    if (ret < 0) {
        return  GSI_ERROR_READING_TOKEN_LENGTH;
    } else if (ret != 4) {
        if (ret == 0)
            return 0;
        status = GSI_ERROR_READING_TOKEN_LENGTH;
	rodsLogAndErrorMsg( LOG_ERROR, igsi_rErrorPtr, status,
			    "reading token length: %d of %d bytes written",
			    ret, 4);
	return status;
    }

    length = ntohl(length);

    if (igsiDebugFlag > 0)
        fprintf(stderr, "token length = %d\n", length);

    return length;
}

/*
 read the token body
 */
#if defined(GSI_AUTH)
int _igsiRcvTokenBody(int fd, gss_buffer_t tok, unsigned int length)
{
    unsigned int ret;
    int status;

    if (tok->length < length) {
        status = GSI_ERROR_TOKEN_TOO_LARGE;
	rodsLogAndErrorMsg( LOG_ERROR, igsi_rErrorPtr, status,
			    "_igsiRcvTokenBody error, token is too large for buffer, %d bytes in token, buffer is %d bytes",
			    length, tok->length);
	return status;
    }

    if (tok->value == NULL) {
        return GSI_ERROR_BAD_TOKEN_RCVED;
    }

    tok->length = length;

    ret = _igsiReadAll(fd, (char *) tok->value, tok->length);
    if (ret < 0) {
        return ret;
    } else if (ret != tok->length) {
        status = GSI_PARTIAL_TOKEN_READ;
	rodsLogAndErrorMsg( LOG_ERROR, igsi_rErrorPtr, status,
			   "reading token data: %d of %d bytes read\n",
			    ret, tok->length);
        return status;
    }

    return tok->length;
}
#endif

/* 
 Read a token from a socket (file descriptor): read the token length
 (as a network long), and then read the token data from the file
 descriptor.  It blocks to read the length and data, if necessary.

 Divided into two routines so that each can be called from igsi_read.
 */
#if defined(GSI_AUTH)
int _igsiRcvToken(int fd, gss_buffer_t tok)
{
    int length;
    int i;

    int tmpLength;
    char *cp;

    if (igsiTokenHeaderMode) {
        /*
           First, if in normal mode, peek to see if the other side is sending
           headers and possibly switch into non-header mode.
         */
        tmpLength = 0;
        cp = (char *) &tmpLength;
        if (sizeof(tmpLength) > 4)
            cp += sizeof(tmpLength) - 4;
        i = recv(fd, cp, 4, MSG_PEEK);
        tmpLength = ntohl(tmpLength);
        if (igsiDebugFlag > 0)
            fprintf(stderr, "peek length = %d\n", tmpLength);
        if (tmpLength > 100000) {
            igsiTokenHeaderMode = 0;
            if (igsiDebugFlag > 0)
                fprintf(stderr, "switching to non-hdr mode\n");
        }
    }

    if (igsiTokenHeaderMode) {
        length = _igsiRcvTokenHeader(fd);
        if (length <= 0)
            return length;

        return (_igsiRcvTokenBody(fd, tok, length));
    } else {
        i = read(fd, (char *) tok->value, tok->length);
        if (igsiDebugFlag > 0)
            fprintf(stderr, "rcved token, length = %d\n", i);
        if (i <= 0)
            return (i);
        tok->length = i;        /* Assume all of token is rcv'ed */
        return 1;               /* success */
    }
}
#endif

/*
 Call GSS routines to get the current (error) status and log/display it.
 */
#if defined(GSI_AUTH)
static void _igsiLogError_1(char *callerMsg, OM_uint32 code, int type)
{
    OM_uint32 majorStatus, minorStatus;
    gss_buffer_desc msg;
    unsigned int msg_ctx;
    int status;
    char *whichSide;

    if (ProcessType==CLIENT_PT) {
       whichSide="Client side:";
    }
    else {
       whichSide="On iRODS-Server side:";
    }

    msg_ctx = 0;
    status = GSI_ERROR_FROM_GSI_LIBRARY;
    while (1) {
        majorStatus = gss_display_status(&minorStatus, code,
				     type, GSS_C_NULL_OID,
				     &msg_ctx, &msg);
	rodsLogAndErrorMsg( LOG_ERROR, igsi_rErrorPtr, status,
			    "%sGSS-API error %s: %s", whichSide, callerMsg,
			    (char *) msg.value);
	(void) gss_release_buffer(&minorStatus, &msg);

        if (!msg_ctx)
            break;
    }
}
#endif

/*

Display an error message followed by GSS-API messages.
 */
#if defined(GSI_AUTH)
void _igsiLogError(char *msg, OM_uint32 majorStatus, OM_uint32 minorStatus)
{
    _igsiLogError_1(msg, majorStatus, GSS_C_GSS_CODE);
    _igsiLogError_1(msg, minorStatus, GSS_C_MECH_CODE);
}
#endif

/*
Display the context flags.
 */
#if defined(GSI_AUTH)
void _igsiDisplayCtxFlags()
{
    if (context_flags & GSS_C_DELEG_FLAG)
        fprintf(stdout, "context flag: GSS_C_DELEG_FLAG\n");
    if (context_flags & GSS_C_MUTUAL_FLAG)
        fprintf(stdout, "context flag: GSS_C_MUTUAL_FLAG\n");
    if (context_flags & GSS_C_REPLAY_FLAG)
        fprintf(stdout, "context flag: GSS_C_REPLAY_FLAG\n");
    if (context_flags & GSS_C_SEQUENCE_FLAG)
        fprintf(stdout, "context flag: GSS_C_SEQUENCE_FLAG\n");
    if (context_flags & GSS_C_CONF_FLAG)
        fprintf(stdout, "context flag: GSS_C_CONF_FLAG \n");
    if (context_flags & GSS_C_INTEG_FLAG)
        fprintf(stdout, "context flag: GSS_C_INTEG_FLAG \n");
}
#endif

/*
 Print the contexts of a token (for debug).
*/
#if defined(GSI_AUTH)
void _igsiPrintToken(gss_buffer_t tok)
{
    unsigned int i, j;
    unsigned char *p = (unsigned char *)tok->value;
    fprintf(stderr, "_igsiPrintToken, length=%d\n", tok->length);
    j = 0;
    for (i = 0; i < tok->length; i++, p++) {
        if (i < 16 || i > tok->length - 16) {
            fprintf(stderr, "%02x ", *p);
            if ((i % 16) == 15) {
                fprintf(stderr, "\n");
            }
        } else {
            j++;
            if (j < 4)
                fprintf(stderr, ".");
            if (j == 4)
                fprintf(stderr, "\n");
        }
    }
    fprintf(stderr, "\n");
    fflush(stderr);
}
#endif

/*

 Setup for GSSAPI functions, for either client or server side.

 If this is not called, the GSS routines will call the equivalent
 during the establishContext.  In some cases, the GSS routines will
 prompt for a password to decrypt the local private key.

 This calls gss_import_name to convert input string (if not NULL) to
 an internal representation (if it is NULL, a default is taken).  Then
 the credentials are acquired (read into memory and converted) via
 gss_acquire_cred (for SSL this is the certificate (with public key)
 and the private key (hence the password to decrypt).  

 If successful, credentials are established in the global myCreds.
 If unsuccessful, the GSS-API error messages are logged/displayed
 and an iRODS error code returned.
 */
int igsiSetupCreds(rcComm_t *Comm, rsComm_t *rsComm, char *specifiedName,
		   char **returnedName)
{
#if defined(GSI_AUTH)
    gss_buffer_desc name_buf;
    gss_name_t myName = GSS_C_NO_NAME;
    gss_name_t myName2 = GSS_C_NO_NAME;
    OM_uint32 majorStatus, minorStatus;
    gss_OID doid2;
    gss_buffer_desc client_name2;

#if defined(IGSI_TIMING)
    float fSec;
    struct timeval startTimeFunc, endTimeFunc, sTimeDiff;
    /* Starting timer for function */
    (void) gettimeofday(&startTimeFunc, (struct timezone *) 0);
#endif

    if (Comm != NULL) igsi_rErrorPtr = Comm->rError;
    if (rsComm != NULL) igsi_rErrorPtr = &rsComm->rError;

    if (specifiedName != NULL) {
        name_buf.value = specifiedName;
	name_buf.length = strlen((char*)name_buf.value) + 1;
	majorStatus = gss_import_name(&minorStatus, &name_buf,
				   (gss_OID) gss_nt_service_name_gsi,
				   &myName);
        if (majorStatus != GSS_S_COMPLETE) {
            _igsiLogError("importing name", majorStatus,
                                minorStatus);
            return GSI_ERROR_IMPORTING_NAME;
        }
    }

    if (myCreds == GSS_C_NO_CREDENTIAL) {
       majorStatus = gss_acquire_cred(&minorStatus, myName, 0,
				   GSS_C_NULL_OID_SET, GSS_C_ACCEPT,
				   &myCreds, NULL, NULL);
    }
    else {
       /* already set up, presumably via a call to igsi_import_credential */
       majorStatus = GSS_S_COMPLETE;  /* OK */
    }
    if (majorStatus != GSS_S_COMPLETE) {
       _igsiLogError("acquiring credentials", majorStatus, minorStatus);
       return GSI_ERROR_ACQUIRING_CREDS;
    }
    (void) gss_release_name(&minorStatus, &myName);

    majorStatus = gss_inquire_cred(&minorStatus, myCreds, &myName2, 
				   NULL, NULL, NULL);
    if (majorStatus != GSS_S_COMPLETE) {
       _igsiLogError("inquire_cred", majorStatus, minorStatus);
       return GSI_ERROR_ACQUIRING_CREDS;
    }

    majorStatus =
       gss_display_name(&minorStatus, myName2, &client_name2, &doid2);
    if (majorStatus != GSS_S_COMPLETE) {
       _igsiLogError("displaying name", majorStatus, minorStatus);
       return GSI_ERROR_DISPLAYING_NAME;
    }
    if (client_name2.value != 0 && client_name2.length>0) {
       *returnedName = (char*)malloc(client_name2.length+10);
       if (*returnedName != 0) {
	  memset(*returnedName,  0, sizeof(client_name2.length));
	  strncpy(*returnedName, (char *)client_name2.value, client_name2.length+1);
       }
    }

    /* release the name structure */
    majorStatus = gss_release_name(&minorStatus, &myName2);

    if (majorStatus != GSS_S_COMPLETE) {
       _igsiLogError("releasing name", majorStatus, minorStatus);
       return GSI_ERROR_RELEASING_NAME;
       }
    
    (void) gss_release_buffer(&minorStatus, &client_name2);


#if defined(IGSI_TIMING)
    (void) gettimeofday(&endTimeFunc, (struct timezone *) 0);
    (void) _igsiTime(&sTimeDiff, &endTimeFunc, &startTimeFunc);
    fSec =
        (float) sTimeDiff.tv_sec + ((float) sTimeDiff.tv_usec / 1000000.0);

    /* for IGSI_TIMING  print time */
    fprintf(stdout, " ---  %.5f sec time taken for executing"
            "igsiSetupCreds() ---  \n", fSec);
#endif
    return 0;

#else
    if (ProcessType==CLIENT_PT) {
       return(GSI_NOT_BUILT_INTO_CLIENT);
    }
    else {
       return(GSI_NOT_BUILT_INTO_SERVER);
    }
#endif
}

/*
 Set up an session between this server and a connected new client.
 
 Establishses a GSS-API context (as the service specified in
 igsiSetupCreds) with an incoming client, and returns the
 authenticated client name (the id on the other side of the socket).
 The other side (the client) must call igsiEstablishContextClientside at
 about the same time for the exchanges across the network to work
 (each side will block waiting for the other).

 If successful, the context handle is set in the global context array.
 If unsuccessful, an error message is displayed and -1 is returned.

 */
int igsiEstablishContextServerside(rsComm_t *rsComm, char *clientName, 
				   int maxLen_clientName)
{
#if defined(GSI_AUTH)
    int fd;
    int status;

    fd = rsComm->sock;
    igsi_rErrorPtr = &rsComm->rError;

    gss_buffer_desc send_buffer, recv_buffer;
    gss_buffer_desc client_name;
    gss_name_t client;
    gss_OID doid;
    OM_uint32 majorStatus, minorStatus;

    int i, j;

#if defined(IGSI_TIMING)
    struct timeval startTimeFunc, endTimeFunc, sTimeDiff;
    float fSec;
    /* Starting timer for function */
    (void) gettimeofday(&startTimeFunc, (struct timezone *) 0);

#endif

    context[fd] = GSS_C_NO_CONTEXT;

    recv_buffer.value = &igsiScratchBuffer;

    do {
        recv_buffer.length = SCRATCH_BUFFER_SIZE;
        status =  _igsiRcvToken(fd, &recv_buffer);
        if (status <= 0) {
	    rodsLogAndErrorMsg( LOG_ERROR, igsi_rErrorPtr, status,
				"igsiEstablishContextServerside");
            return status;
        }
        if (igsiDebugFlag > 0) {
            fprintf(stderr, "Received token (size=%d): \n",
                    recv_buffer.length);
            _igsiPrintToken(&recv_buffer);
        }

        majorStatus = gss_accept_sec_context(&minorStatus, 
                           &context[fd], myCreds, &recv_buffer, 
                           GSS_C_NO_CHANNEL_BINDINGS, &client, &doid, 
                           &send_buffer, &context_flags, 
                           NULL,     /* ignore time_rec */
                           NULL);    /* ignore del_cred_handle */

        if (majorStatus != GSS_S_COMPLETE
            && majorStatus != GSS_S_CONTINUE_NEEDED) {
                _igsiLogError("accepting context", majorStatus, minorStatus);
	    memset (igsiScratchBuffer, 0, SCRATCH_BUFFER_SIZE);
            return GSI_ACCEPT_SEC_CONTEXT_ERROR;
        }

        /* since buffer is not malloc'ed, don't need to call
           gss_release_buffer, instead clear it. */
	memset (igsiScratchBuffer, 0, SCRATCH_BUFFER_SIZE);

        if (send_buffer.length != 0) {
            if (igsiDebugFlag > 0) {
                fprintf(stderr,
                        "Sending accept_sec_context token (size=%d):\n",
                        send_buffer.length);
                _igsiPrintToken(&send_buffer);
            }
            status = _gsiSendToken(fd, &send_buffer);
	    if (status < 0) {
                return status;
            }
        }
        if (igsiDebugFlag > 0) {
            if (majorStatus == GSS_S_CONTINUE_NEEDED)
                fprintf(stderr, "continue needed...\n");
        }
    } while (majorStatus == GSS_S_CONTINUE_NEEDED);

    /* convert name (internally represented) to a string */
    majorStatus =
            gss_display_name(&minorStatus, client, &client_name, &doid);
    if (majorStatus != GSS_S_COMPLETE) {
        _igsiLogError("displaying name", majorStatus, minorStatus);
        return GSI_ERROR_DISPLAYING_NAME;
    }

    i = client_name.length;
    if (maxLen_clientName < i)
        i = maxLen_clientName;

    strncpy(clientName, (char*)client_name.value, i);
    j = client_name.length;
    if (maxLen_clientName > j)
        clientName[client_name.length] = '\0';

    /* release the name structure */
    majorStatus = gss_release_name(&minorStatus, &client);

    if (majorStatus != GSS_S_COMPLETE) {
        _igsiLogError("releasing name", majorStatus, minorStatus);
        return GSI_ERROR_RELEASING_NAME;
    }

    (void) gss_release_buffer(&minorStatus, &client_name);

#if defined(IGSI_TIMING)
    (void) gettimeofday(&endTimeFunc, (struct timezone *) 0);
    (void) _igsiTime(&sTimeDiff, &endTimeFunc, &startTimeFunc);
    fSec =
        (float) sTimeDiff.tv_sec + ((float) sTimeDiff.tv_usec / 1000000.0);

    /* for IGSI_TIMING  print time */
    fprintf(stdout, " ---  %.5f sec time taken for executing "
            "igsiEstablishContextServerside() ---  \n", fSec);
#endif

    return 0;

#else
    if (ProcessType==CLIENT_PT) {
       return(GSI_NOT_BUILT_INTO_CLIENT);
    }
    else {
       return(GSI_NOT_BUILT_INTO_SERVER);
    }
#endif
}

/*
 Return data previous read
 */
int _NoUsed_igsiReturnCachedData(int fd, char *buffer,int length)
{
#if defined(GSI_AUTH)
    int mv, i;
    char *cp, *cp2;
    mv = inCacheBufWriteIndex[fd] - inCacheBufReadIndex[fd];
    if (length < mv)
        mv = length;
    cp = buffer;
    cp2 = inCacheBuf[fd] + inCacheBufReadIndex[fd];
    for (i = 0; i < mv; i++)
        *cp++ = *cp2++;
    inCacheBufReadIndex[fd] += mv;
    if (inCacheBufReadIndex[fd] == inCacheBufWriteIndex[fd])
        inCacheBufState[fd] = 0;
    return mv;
#else
    return(0);
#endif
}

/*
 Close out the context for the associated fd (socket).
 */
void igsiClose(int fd)
{
#if defined(GSI_AUTH)
    OM_uint32 majorStatus, minorStatus;

    if (inCacheBuf[fd] != 0) {        /* input cache allocated */
        if (igsiDebugFlag > 0) {
            fprintf(stderr,
                    "IGSI Info: freeing a buffer of %d bytes for fd %d\n",
                    inCacheBufLen[fd], fd);
        }
        free(inCacheBuf[fd]);
        inCacheBuf[fd] = 0;
        inCacheBufState[fd] = 0;
    }

    majorStatus = gss_delete_sec_context(&minorStatus, &context[fd], NULL);

    if (majorStatus != GSS_S_COMPLETE && igsiDebugFlag > 0) {
       _igsiLogError("deleting context", majorStatus, minorStatus);
       return;
    }
#endif
    return;
}

/*
 Close out all existing contextes and delete (erase) our credentials.
 */
void igsiCloseAll()
{
#if defined(GSI_AUTH)
    OM_uint32 majorStatus, minorStatus;
    int i;

    if (myCreds != 0) {
        majorStatus = gss_release_cred(&minorStatus, &myCreds);
        if (majorStatus != GSS_S_COMPLETE) {
            _igsiLogError("releasing cred", majorStatus, minorStatus);
        }
    }

    for (i = 0; i < MAX_FDS; i++) {
        if (context[i] != GSS_C_NO_CONTEXT) {
            igsiClose(i);
        }
    }
#endif
}

/*
 Set up an session between this client and a connected server.
 
 Establishses a GSS-API context, confirming the identity of the server
 as matching the input service_name string.  Confirms the this
 process's (the client's) identity to the server (based on the id
 specified in igsiSetupCreds).

 The other side (the client) must call igsiEstablishContextServerside
 at about the same time for the exchanges across the network to work
 (each side will block waiting for the other).  If unsuccessful, an
 error message is displayed and errorcode returned.

 The service_name is imported as a GSS-API name (translated to
 internal format) and a GSS-API context is established with the
 corresponding service The default GSS-API mechanism is used, and
 mutual authentication and replay detection are requested.
 
 If successful, the context handle is set in the global context array.
*/

int igsiEstablishContextClientside(rcComm_t *Comm, char *serviceName, 
			       int delegFlag)
{
#if defined(GSI_AUTH)
    int fd;
    int status;

    fd = Comm->sock;

    igsi_rErrorPtr = Comm->rError;

    gss_OID oid = GSS_C_NULL_OID;
    gss_buffer_desc send_tok, recv_tok, *tokenPtr, name_buffer;
    gss_name_t target_name;
    OM_uint32 majorStatus, minorStatus;
    OM_uint32 flags = 0;

#if defined(IGSI_TIMING)
    struct timeval startTimeFunc, endTimeFunc, sTimeDiff;
    float fSec;
    /* Starting timer for function */
    (void) gettimeofday(&startTimeFunc, (struct timezone *) 0);
#endif

    if (serviceName != 0 && strlen(serviceName)>0) {
       /*
	* Import the name into target_name.
	*/
       name_buffer.value = serviceName;
       name_buffer.length = strlen(serviceName) + 1;

       majorStatus = gss_import_name(&minorStatus, &name_buffer,
			       (gss_OID) gss_nt_service_name_gsi,
			       &target_name);

       if (majorStatus != GSS_S_COMPLETE) {
       /* could use "if (GSS_ERROR(majorStatus))" but I believe it should
          always be GSS_S_COMPLETE if successful  */
          _igsiLogError
            ("importing name (igsiEstablishContextClientside)", majorStatus,
             minorStatus);
	  return GSI_ERROR_IMPORT_NAME; 
       } 
    }
    /*
     * Perform the context-establishment loop.
     *
     * On each pass through the loop, tokenPtr points to the token
     * to send to the server (or GSS_C_NO_BUFFER on the first pass).
     * Every generated token is stored in send_tok which is then
     * transmitted to the server; every received token is stored in
     * recv_tok, which tokenPtr is then set to, to be processed by
     * the next call to gss_init_sec_context.
     * 
     * GSS-API guarantees that send_tok's length will be non-zero
     * if and only if the server is expecting another token from us,
     * and that gss_init_sec_context returns GSS_S_CONTINUE_NEEDED if
     * and only if the server has another token to send us.
     */

    tokenPtr = GSS_C_NO_BUFFER;
    context[fd] = GSS_C_NO_CONTEXT;
    flags = GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG;
    if (delegFlag)
        flags |= GSS_C_DELEG_FLAG;

    do {
        if (igsiDebugFlag > 0) 
            fprintf(stderr, "--> calling gss_init_sec_context\n");

	if (serviceName != 0 && strlen(serviceName)>0) {
	   majorStatus = gss_init_sec_context(&minorStatus,
                       myCreds, &context[fd], target_name, oid, 
                       flags, 0, 
                       NULL,   /* no channel bindings */
                       tokenPtr, NULL,    /* ignore mech type */
                       &send_tok, &context_flags, 
                       NULL);   /* ignore time_rec */
	}
	else {
	   majorStatus = gss_init_sec_context(&minorStatus,
                       myCreds, &context[fd], GSS_C_NO_NAME, oid, 
                       flags, 0, 
                       NULL,   /* no channel bindings */
                       tokenPtr, NULL,    /* ignore mech type */
                       &send_tok, &context_flags, 
                       NULL);   /* ignore time_rec */
	}

        /* since recv_tok is not malloc'ed, don't need to call
           gss_release_buffer, instead clear it. */
	memset (igsiScratchBuffer, 0, SCRATCH_BUFFER_SIZE);

        if (majorStatus != GSS_S_COMPLETE
            && majorStatus != GSS_S_CONTINUE_NEEDED) {
	    _igsiLogError("initializing context", 
			       majorStatus, minorStatus);
	    (void) gss_release_name(&minorStatus, &target_name);
            return GSI_ERROR_INIT_SECURITY_CONTEXT;
        }

        if (send_tok.length != 0) {
	    status = _gsiSendToken(fd, &send_tok);
            if (status < 0) {
	        rodsLogAndErrorMsg( LOG_ERROR, igsi_rErrorPtr, status,
                        "Sending init_sec_context token (size=%d)\n",
                        send_tok.length);
		(void) gss_release_buffer(&minorStatus, &send_tok);
		(void) gss_release_name(&minorStatus, &target_name);
		return status; /* GSI_SOCKET_WRITE_ERROR */
	    }
	}

        (void) gss_release_buffer(&minorStatus, &send_tok);

        if (majorStatus == GSS_S_CONTINUE_NEEDED) {
            if (igsiDebugFlag > 0)
                fprintf(stderr, "continue needed...\n");
            recv_tok.value = &igsiScratchBuffer;
            recv_tok.length = SCRATCH_BUFFER_SIZE;
            if (_igsiRcvToken(fd, &recv_tok) <= 0) {
	       (void) gss_release_name(&minorStatus, &target_name);
                return GSI_ERROR_RELEASING_NAME;
            }
            tokenPtr = &recv_tok;
        }
    } while (majorStatus == GSS_S_CONTINUE_NEEDED);

    if (serviceName != 0 && strlen(serviceName)>0) {
       (void) gss_release_name(&minorStatus, &target_name);
    }

    if (igsiDebugFlag > 0)
        _igsiDisplayCtxFlags();

#if defined(IGSI_TIMING)
    (void) gettimeofday(&endTimeFunc, (struct timezone *) 0);
    (void) _igsiTime(&sTimeDiff, &endTimeFunc, &startTimeFunc);
    fSec =
        (float) sTimeDiff.tv_sec + ((float) sTimeDiff.tv_usec / 1000000.0);

    /* for IGSI_TIMING  print time */
    fprintf(stdout, " ---  %.5f sec time taken for executing "
            "igsiEstablishContextClientside() ---  \n", fSec);
#endif
    return 0;

#else
    if (ProcessType==CLIENT_PT) {
       return(GSI_NOT_BUILT_INTO_CLIENT);
    }
    else {
       return(GSI_NOT_BUILT_INTO_SERVER);
    }
#endif

}

/* 
 Set a the igsiDebugFlag for displaying various levels of messages.
  0 - none (default)
  1 - some
  2 - more
 */
int igsi_debug(int val)
{
    igsiDebugFlag = val;
    return(0);
}


/*

This routine can be defined to replace the default SSL one for
providing the key password.

int EVP_read_pw_string(char *buf,int len,char *prompt,int verify)
{
    int i;
    if (do_pw==1) {
       gets(buf);  This could be changed to read an encrypted pass phrase
                   from a file or something like that.
       return 0;
    }
 
    else {
       if ((prompt == NULL) && (prompt_string[0] != '\0'))
           prompt=prompt_string; 
       return(des_read_pw_string(buf,len,prompt,verify));
    }
}

*/

/* Currently untested and not used.  I beleive GSI no longer supports
 * the without-server-DN option.
 *
 * Function used by client in calling the server to establish security
 * context without providing Server DN. The server DN is then returned
 * in serverName after the creation of context. This server DN can be
 * verified by application before proceeding further if wished.
 * 
 * Input Parameters 
 * fd: Socket File Descriptor of the connection with the server
 * maxLenServerName: Maximum length of ServerDN willing to be accepted
 *      in serverName by client
 * delegFlag: Should be equal to 1 if delegation is to be initiated or 
 *                                0 if no delegation initiation.
 * 
 * Return Parameters:
 * serverName: Pointer to the string contain DN of the connected server
 *  */
int igsiEstablishContextClientsideWithoutServerDN(int fd, char *serverName,
                                                 unsigned int maxLenServerName,
                                                 int delegFlag)
{
#if defined(GSI_AUTH)
    gss_OID oid = GSS_C_NULL_OID;
    gss_buffer_desc send_tok, recv_tok, *tokenPtr;
    gss_name_t server_name = GSS_C_NO_NAME;
    gss_buffer_desc server_dn;
    OM_uint32 majorStatus, minorStatus;
    OM_uint32 flags = 0;
    unsigned int i;
    int status;

#if defined(IGSI_TIMING)
    struct timeval startTimeFunc, endTimeFunc, sTimeDiff;
    float fSec;
    /* Starting timer for function */
    (void) gettimeofday(&startTimeFunc, (struct timezone *) 0);

#endif

    /* performing context establishment loop */
    tokenPtr = GSS_C_NO_BUFFER;
    context[fd] = GSS_C_NO_CONTEXT;
    flags = GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG;
    if (delegFlag)
        flags |= GSS_C_DELEG_FLAG;

    do {
        if (igsiDebugFlag > 0)
            fprintf(stderr, "--> calling gss_init_sec_context\n");

        majorStatus = gss_init_sec_context(&minorStatus,
                       /* was: GSS_C_NO_CREDENTIAL,  */
                       myCreds, &context[fd], 
                       GSS_C_NO_NAME,  /* no Server DN specified */
                       oid, flags, 0, NULL,    /* no channel bindings */
                       tokenPtr, NULL,        /* ignore mech type */
                       &send_tok, &context_flags, 
                       NULL);       /* ignore time_rec */

        /* since recv_tok is not malloc'ed, don't need to call
           gss_release_buffer, instead clear it. */
	memset (igsiScratchBuffer, 0, SCRATCH_BUFFER_SIZE);

        if (majorStatus != GSS_S_COMPLETE
            && majorStatus != GSS_S_CONTINUE_NEEDED) {
	   _igsiLogError("initializing context", majorStatus, minorStatus);
	   return -1;
        }

        if (send_tok.length != 0) {
            if (igsiDebugFlag > 0)
                fprintf(stderr,
                        "Sending init_sec_context token (size=%d)\n",
                        send_tok.length);
            status =  _gsiSendToken(fd, &send_tok);
            if (status < 0) {
                (void) gss_release_buffer(&minorStatus, &send_tok);
                return status;
            }
        }

        (void) gss_release_buffer(&minorStatus, &send_tok);

        if (majorStatus == GSS_S_CONTINUE_NEEDED) {
            if (igsiDebugFlag > 0)
                fprintf(stderr, "continue needed...\n");
            recv_tok.value = &igsiScratchBuffer;
            recv_tok.length = SCRATCH_BUFFER_SIZE;
            if (_igsiRcvToken(fd, &recv_tok) <= 0) {

                return -1;
            }
            tokenPtr = &recv_tok;
        }
    } while (majorStatus == GSS_S_CONTINUE_NEEDED);

    /* reading Server DN from context */
    majorStatus = gss_inquire_context(&minorStatus,   /*minor_status */
                                   context[fd], /* context_handle_P */
                                   NULL,        /* gss_name_t * src_name_P, */
                                   &server_name,/* targ_name_P, */
                                   0,   /*OM_uint32 * lifetime_rec, */
                                   NULL,        /* gss_OID *  mech_type, */
                                   NULL,        /* OM_uint32 * ctx_flags, */
                                   NULL,        /* int * locally_initiated, */
                                   NULL);       /* int * open) */

    if (majorStatus != GSS_S_COMPLETE) {
        fprintf(stderr,
                "igsiEstablishContextClientsideWithoutServerDN() - unable to inquire context \n");
        (void) gss_release_name(&minorStatus, &server_name);
        return -1;
    }

    /* after successful completion
     * convert name (internally represented) to a string */

    majorStatus = gss_display_name(&minorStatus, server_name, &server_dn, NULL);

    if (majorStatus != GSS_S_COMPLETE) {
        fprintf(stderr,
                "igsiEstablishContextClientsideWithoutServerDN() - unable to read server name \n");
        (void) gss_release_name(&minorStatus, &server_name);
        return -1;
    }

    i = server_dn.length;
    if (maxLenServerName < i)
        i = maxLenServerName;

    strncpy(serverName, (char *)server_dn.value, i);
    if (maxLenServerName > server_dn.length)
        serverName[server_dn.length] = '\0';

    /* release the name structure */
    majorStatus = gss_release_name(&minorStatus, &server_name);

    if (majorStatus != GSS_S_COMPLETE) {
        fprintf(stderr,
                " igsiEstablishContextClientsideWithoutServerDN() - unable to release name \n");
        return -1;
    }

    /* release buffer */

    (void) gss_release_buffer(&minorStatus, &server_dn);
    if (majorStatus != GSS_S_COMPLETE) {
        fprintf(stderr,
                " igsiEstablishContextClientsideWithoutServerDN() - unable to release buffer \n");
        return -1;
    }

#if defined(IGSI_TIMING)
    (void) gettimeofday(&endTimeFunc, (struct timezone *) 0);
    (void) _igsiTime(&sTimeDiff, &endTimeFunc, &startTimeFunc);
    fSec =
        (float) sTimeDiff.tv_sec + ((float) sTimeDiff.tv_usec / 1000000.0);

    /* for IGSI_TIMING  print time */
    fprintf(stdout, " ---  %.5f sec time taken for executing "
            "igsiEstablishContextClientsideWithoutServerDN() ---  \n",
            fSec);
#endif
    return 0;

#else
    if (ProcessType==CLIENT_PT) {
       return(GSI_NOT_BUILT_INTO_CLIENT);
    }
    else {
       return(GSI_NOT_BUILT_INTO_SERVER);
    }
#endif
}
