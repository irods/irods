/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
These functions provide a relatively simple interface to the Kerberos
authentication system, similar to the igsi.c GSS-API interface to GSI.

Normally, we call the gss* functions which have the same names as
those used by GSI (they are both using the Generic Security Services
API).  To be able to link with both kerberos and GSI at the same time,
the gss* functions are mapped to krb_gss* functions which are defined
in ikrbGSSAPIWrapper.c which in turn explicitly find the functions in
the DLL and call them.

All functions called from outside have names that start with 'ikrb'.
Internal routines start with '_ikrb'.

Also see rsKrbAuthRequest.c, which includes the ikrbServersideAuth
function which calls ikrbEstablishContextServerside, and then queries and
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

#if defined(KRB_AUTH)
#include <gssapi.h>
#if defined(GSI_AUTH)
#include "ikrbGSSAPIWrapper.h"
#endif
#endif

#include <stdio.h>
#include <sys/time.h>           /* for IKRB_TIMING only */
#include <string.h>

#include "rodsErrorTable.h"

#if defined(KRB_AUTH) && defined(GSI_AUTH)
/* When both KRB and GSI are used at the same time, redefine the
   function names to be our KRB wrapper functions that interface
   differently (see ikrbGSSAPIWrapper.c */
#define GSS_DISPLAY_STATUS     krb_gss_display_status
#define GSS_RELEASE_BUFFER     krb_gss_release_buffer
#define GSS_IMPORT_NAME        krb_gss_import_name
#define GSS_ACQUIRE_CRED       krb_gss_acquire_cred
#define GSS_RELEASE_NAME       krb_gss_release_name
#define GSS_INQUIRE_CRED       krb_gss_inquire_cred
#define GSS_DISPLAY_NAME       krb_gss_display_name
#define GSS_ACCEPT_SEC_CONTEXT krb_gss_accept_sec_context
#define GSS_DELETE_SEC_CONTEXT krb_gss_delete_sec_context
#define GSS_RELEASE_CRED       krb_gss_release_cred
#define GSS_INIT_SEC_CONTEXT   krb_gss_init_sec_context
#else
#define GSS_DISPLAY_STATUS     gss_display_status
#define GSS_RELEASE_BUFFER     gss_release_buffer
#define GSS_IMPORT_NAME        gss_import_name
#define GSS_ACQUIRE_CRED       gss_acquire_cred
#define GSS_RELEASE_NAME       gss_release_name
#define GSS_INQUIRE_CRED       gss_inquire_cred
#define GSS_DISPLAY_NAME       gss_display_name
#define GSS_ACCEPT_SEC_CONTEXT gss_accept_sec_context
#define GSS_DELETE_SEC_CONTEXT gss_delete_sec_context
#define GSS_RELEASE_CRED       gss_release_cred
#define GSS_INIT_SEC_CONTEXT   gss_init_sec_context
#endif

rError_t *ikrb_rErrorPtr;

/* #define IKRB_TIMING 1  (uncomment this out for timing) */


void _ikrbTime(struct timeval *tdiff, struct timeval *t1,
                struct timeval *t0); 
#if defined(KRB_AUTH)
extern gss_OID gss_nt_service_name;
extern int OBJ_create(char *input1, char *input2, char *input3);
#endif

#define gss_nt_service_name_krb 0

#define SCRATCH_BUFFER_SIZE 20000
char ikrbScratchBuffer[SCRATCH_BUFFER_SIZE];

static int ikrbDebugFlag = 0;

#define MAX_FDS 32
#if defined(KRB_AUTH)
static int ikrbTokenHeaderMode = 1;  /* 1 is the normal mode, 
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

#endif  /* end of #if defined(KRB_AUTH) */

OM_uint32 context_flags;

/* Function for time test 
 * Returns the difference between start time and end time in tdiff 
 * struct in seconds */
void _ikrbTime(struct timeval *tdiff, struct timeval *endTime,
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
#if defined(KRB_AUTH)
static int _ikrbWriteAll(int fd, char *buf, unsigned int nbyte)
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
    if (ikrbDebugFlag > 0)
        fprintf(stderr, "_ikrbWriteAll, wrote=%d\n", ptr - buf);
    return (ptr - buf);
}
#endif

/* 
 Read a buffer, continuing until it is filled.
 */
static int _ikrbReadAll(int fd, char *buf, unsigned int nbyte)
{
    int ret;
    char *ptr;

    for (ptr = buf; nbyte; ptr += ret, nbyte -= ret) {
        ret = read(fd, ptr, nbyte);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            return (KRB_SOCKET_READ_ERROR);
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
#if defined(KRB_AUTH)
int _krbSendToken(int fd, gss_buffer_t tok)
{
    unsigned int len, ret;
    char *cp;
    int status;

    if (ikrbDebugFlag > 0)
        fprintf(stderr, "sending tok->length=%d\n", tok->length);

    if (ikrbTokenHeaderMode) {
        len = htonl(tok->length);

        cp = (char *) &len;
        if (sizeof(len) > 4)
            cp += sizeof(len) - 4;
        ret = _ikrbWriteAll(fd, cp, 4);
        if (ret < 0) {
            return KRB_ERROR_SENDING_TOKEN_LENGTH;
        } else if (ret != 4) {
            status = KRB_ERROR_SENDING_TOKEN_LENGTH;
	    rodsLogAndErrorMsg( LOG_ERROR, ikrb_rErrorPtr, status,
				"sending token data: %d of %d bytes written",
				ret, tok->length);
	    return status;
        }
    }

    ret = _ikrbWriteAll(fd, (char *)tok->value, tok->length);
    if (ret < 0) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    } else if (ret != tok->length) {
       status = KRB_ERROR_SENDING_TOKEN_LENGTH;
       rodsLogAndErrorMsg( LOG_ERROR, ikrb_rErrorPtr, status,
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
int _ikrbRcvTokenHeader(int fd)
{
    int length;
    char *cp;
    int ret;
    int status;

    length = 0;
    cp = (char *) &length;
    if (sizeof(length) > 4)
        cp += sizeof(length) - 4;
    ret = _ikrbReadAll(fd, cp, 4);
    if (ret < 0) {
        return  KRB_ERROR_READING_TOKEN_LENGTH;
    } else if (ret != 4) {
        if (ret == 0)
            return 0;
        status = KRB_ERROR_READING_TOKEN_LENGTH;
	rodsLogAndErrorMsg( LOG_ERROR, ikrb_rErrorPtr, status,
			    "reading token length: %d of %d bytes written",
			    ret, 4);
	return status;
    }

    length = ntohl(length);

    if (ikrbDebugFlag > 0)
        fprintf(stderr, "token length = %d\n", length);

    return length;
}

/*
 read the token body
 */
#if defined(KRB_AUTH)
int _ikrbRcvTokenBody(int fd, gss_buffer_t tok, int length)
{
    unsigned int ret;
    int status;
    int i;

    i = tok->length;
    if (i < length) {
        status = KRB_ERROR_TOKEN_TOO_LARGE;
	rodsLogAndErrorMsg( LOG_ERROR, ikrb_rErrorPtr, status,
			    "_ikrbRcvTokenBody error, token is too large for buffer, %d bytes in token, buffer is %d bytes",
			    length, tok->length);
	return status;
    }

    if (tok->value == NULL) {
        return KRB_ERROR_BAD_TOKEN_RCVED;
    }

    tok->length = length;

    ret = _ikrbReadAll(fd, (char *) tok->value, tok->length);
    if (ret < 0) {
        return ret;
    } else if (ret != tok->length) {
        status = KRB_PARTIAL_TOKEN_READ;
	rodsLogAndErrorMsg( LOG_ERROR, ikrb_rErrorPtr, status,
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

 Divided into two routines so that each can be called from ikrb_read.
 */
#if defined(KRB_AUTH)
int _ikrbRcvToken(int fd, gss_buffer_t tok)
{
    int length;
    int i;

    int tmpLength;
    char *cp;

    if (ikrbTokenHeaderMode) {
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
        if (ikrbDebugFlag > 0)
            fprintf(stderr, "peek length = %d\n", tmpLength);
        if (tmpLength > 100000) {
            ikrbTokenHeaderMode = 0;
            if (ikrbDebugFlag > 0)
                fprintf(stderr, "switching to non-hdr mode\n");
        }
    }

    if (ikrbTokenHeaderMode) {
        length = _ikrbRcvTokenHeader(fd);
        if (length <= 0)
            return length;

        return (_ikrbRcvTokenBody(fd, tok, length));
    } else {
        i = read(fd, (char *) tok->value, tok->length);
        if (ikrbDebugFlag > 0)
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
#if defined(KRB_AUTH)
static void _ikrbLogError_1(char *callerMsg, OM_uint32 code, int type)
{
    OM_uint32 majorStatus, minorStatus;
    gss_buffer_desc msg;
    OM_uint32 msg_ctx;
    int status;
    char *whichSide;

    if (ProcessType==CLIENT_PT) {
       whichSide="Client side:";
    }
    else {
       whichSide="On iRODS-Server side:";
    }

    msg_ctx = 0;
    status = KRB_ERROR_FROM_KRB_LIBRARY;
    while (1) {
        majorStatus =    GSS_DISPLAY_STATUS(&minorStatus, code,
				     type, GSS_C_NULL_OID,
				     &msg_ctx, &msg);
	rodsLogAndErrorMsg( LOG_ERROR, ikrb_rErrorPtr, status,
			    "%sGSS-API error %s: %s", whichSide, callerMsg,
			    (char *) msg.value);
	(void)    GSS_RELEASE_BUFFER(&minorStatus, &msg);

        if (!msg_ctx)
            break;
    }
}
#endif

/*

Display an error message followed by GSS-API messages.
 */
#if defined(KRB_AUTH)
void _ikrbLogError(char *msg, OM_uint32 majorStatus, OM_uint32 minorStatus)
{
    _ikrbLogError_1(msg, majorStatus, GSS_C_GSS_CODE);
    _ikrbLogError_1(msg, minorStatus, GSS_C_MECH_CODE);
}
#endif

/*
Display the context flags.
 */
#if defined(KRB_AUTH)
void _ikrbDisplayCtxFlags()
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
#if defined(KRB_AUTH)
void _ikrbPrintToken(gss_buffer_t tok)
{
    unsigned int i, j;
    unsigned char *p = (unsigned char *)tok->value;
    fprintf(stderr, "_ikrbPrintToken, length=%d\n", tok->length);
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
int ikrbSetupCreds(rcComm_t *Comm, rsComm_t *rsComm, char *specifiedName,
		   char **returnedName)
{
#if defined(KRB_AUTH)
    gss_buffer_desc name_buf;
    gss_name_t myName = GSS_C_NO_NAME;
    gss_name_t myName2 = GSS_C_NO_NAME;
    OM_uint32 majorStatus, minorStatus;
    gss_OID doid2;
    gss_buffer_desc client_name2;

#if defined(IKRB_TIMING)
    float fSec;
    struct timeval startTimeFunc, endTimeFunc, sTimeDiff;
    /* Starting timer for function */
    (void) gettimeofday(&startTimeFunc, (struct timezone *) 0);
#endif

    if (Comm != NULL) ikrb_rErrorPtr = Comm->rError;
    if (rsComm != NULL) ikrb_rErrorPtr = &rsComm->rError;

    if (specifiedName != NULL) {
        name_buf.value = specifiedName;
        name_buf.length = strlen((char *)name_buf.value) + 1;
	majorStatus =    GSS_IMPORT_NAME(&minorStatus, &name_buf,
				   (gss_OID) gss_nt_service_name_krb,
				   &myName);
        if (majorStatus != GSS_S_COMPLETE) {
            _ikrbLogError("importing name", majorStatus,
                                minorStatus);
            return KRB_ERROR_IMPORTING_NAME;
        }
    }

    if (myCreds == GSS_C_NO_CREDENTIAL) {
       if (specifiedName != NULL) {
	  majorStatus =    GSS_ACQUIRE_CRED(&minorStatus, myName, 0,
					    GSS_C_NULL_OID_SET, GSS_C_ACCEPT,
					    &myCreds, NULL, NULL);
       }
       else {
	  majorStatus =    GSS_ACQUIRE_CRED(&minorStatus, myName, 0,
					    GSS_C_NULL_OID_SET, GSS_C_INITIATE,
					    &myCreds, NULL, NULL);
       }
    }
    else {
       /* already set up, presumably via a call to ikrb_import_credential */
       majorStatus = GSS_S_COMPLETE;  /* OK */
    }
    if (majorStatus != GSS_S_COMPLETE) {
       _ikrbLogError("acquiring credentials", majorStatus, minorStatus);
       return KRB_ERROR_ACQUIRING_CREDS;
    }
    (void)    GSS_RELEASE_NAME(&minorStatus, &myName);

    majorStatus =    GSS_INQUIRE_CRED(&minorStatus, myCreds, &myName2, 
				      NULL, NULL, NULL);
    if (majorStatus != GSS_S_COMPLETE) {
       _ikrbLogError("inquire_cred", majorStatus, minorStatus);
       return KRB_ERROR_ACQUIRING_CREDS;
    }

    majorStatus =
          GSS_DISPLAY_NAME(&minorStatus, myName2, &client_name2, &doid2);
    if (majorStatus != GSS_S_COMPLETE) {
       _ikrbLogError("displaying name", majorStatus, minorStatus);
       return KRB_ERROR_DISPLAYING_NAME;
    }
    if (client_name2.value != 0 && client_name2.length>0) {
       *returnedName = (char *)malloc(client_name2.length+10);
       if (*returnedName != 0) {
	  memset(*returnedName,  0, sizeof(client_name2.length));
	  strncpy(*returnedName, (char *)client_name2.value, client_name2.length+1);
       }
    }

    /* release the name structure */
    majorStatus =    GSS_RELEASE_NAME(&minorStatus, &myName2);

    if (majorStatus != GSS_S_COMPLETE) {
       _ikrbLogError("releasing name", majorStatus, minorStatus);
       return KRB_ERROR_RELEASING_NAME;
       }
    
    (void)    GSS_RELEASE_BUFFER(&minorStatus, &client_name2);


#if defined(IKRB_TIMING)
    (void) gettimeofday(&endTimeFunc, (struct timezone *) 0);
    (void) _ikrbTime(&sTimeDiff, &endTimeFunc, &startTimeFunc);
    fSec =
        (float) sTimeDiff.tv_sec + ((float) sTimeDiff.tv_usec / 1000000.0);

    /* for IKRB_TIMING  print time */
    fprintf(stdout, " ---  %.5f sec time taken for executing"
            "ikrbSetupCreds() ---  \n", fSec);
#endif
    return 0;

#else
    if (ProcessType==CLIENT_PT) {
       return(KRB_NOT_BUILT_INTO_CLIENT);
    }
    else {
       return(KRB_NOT_BUILT_INTO_SERVER);
    }
#endif
}

/*
 Set up an session between this server and a connected new client.
 
 Establishses a GSS-API context (as the service specified in
 ikrbSetupCreds) with an incoming client, and returns the
 authenticated client name (the id on the other side of the socket).
 The other side (the client) must call ikrbEstablishContextClientside at
 about the same time for the exchanges across the network to work
 (each side will block waiting for the other).

 If successful, the context handle is set in the global context array.
 If unsuccessful, an error message is displayed and -1 is returned.

 */
int ikrbEstablishContextServerside(rsComm_t *rsComm, char *clientName, 
				   int maxLen_clientName)
{
#if defined(KRB_AUTH)
    int fd;
    int status;

    fd = rsComm->sock;
    ikrb_rErrorPtr = &rsComm->rError;

    gss_buffer_desc send_buffer, recv_buffer;
    gss_buffer_desc client_name;
    gss_name_t client;
    gss_OID doid;
    OM_uint32 majorStatus, minorStatus;

    int i, j;

#if defined(IKRB_TIMING)
    struct timeval startTimeFunc, endTimeFunc, sTimeDiff;
    float fSec;
    /* Starting timer for function */
    (void) gettimeofday(&startTimeFunc, (struct timezone *) 0);

#endif

    context[fd] = GSS_C_NO_CONTEXT;

    recv_buffer.value = &ikrbScratchBuffer;

    do {
        recv_buffer.length = SCRATCH_BUFFER_SIZE;
        status =  _ikrbRcvToken(fd, &recv_buffer);
        if (status <= 0) {
	    rodsLogAndErrorMsg( LOG_ERROR, ikrb_rErrorPtr, status,
				"ikrbEstablishContextServerside");
            return status;
        }
        if (ikrbDebugFlag > 0) {
            fprintf(stderr, "Received token (size=%d): \n",
                    recv_buffer.length);
            _ikrbPrintToken(&recv_buffer);
        }
	
        majorStatus =    GSS_ACCEPT_SEC_CONTEXT(&minorStatus, 
                           &context[fd], myCreds, &recv_buffer, 
                           GSS_C_NO_CHANNEL_BINDINGS, &client, &doid, 
                           &send_buffer, &context_flags, 
                           NULL,     /* ignore time_rec */
                           NULL);    /* ignore del_cred_handle */

        if (majorStatus != GSS_S_COMPLETE
            && majorStatus != GSS_S_CONTINUE_NEEDED) {
                _ikrbLogError("accepting context", majorStatus, minorStatus);
	    memset (ikrbScratchBuffer, 0, SCRATCH_BUFFER_SIZE);
            return KRB_ACCEPT_SEC_CONTEXT_ERROR;
        }

        /* since buffer is not malloc'ed, don't need to call
           gss_release_buffer, instead clear it. */
	memset (ikrbScratchBuffer, 0, SCRATCH_BUFFER_SIZE);

        if (send_buffer.length != 0) {
            if (ikrbDebugFlag > 0) {
                fprintf(stderr,
                        "Sending accept_sec_context token (size=%d):\n",
                        send_buffer.length);
                _ikrbPrintToken(&send_buffer);
            }
            status = _krbSendToken(fd, &send_buffer);
	    if (status < 0) {
                return status;
            }
        }
        if (ikrbDebugFlag > 0) {
            if (majorStatus == GSS_S_CONTINUE_NEEDED)
                fprintf(stderr, "continue needed...\n");
        }
    } while (majorStatus == GSS_S_CONTINUE_NEEDED);

    /* convert name (internally represented) to a string */
    majorStatus =
               GSS_DISPLAY_NAME(&minorStatus, client, &client_name, &doid);
    if (majorStatus != GSS_S_COMPLETE) {
        _ikrbLogError("displaying name", majorStatus, minorStatus);
        return KRB_ERROR_DISPLAYING_NAME;
    }

    i = client_name.length;
    if (maxLen_clientName < i)
        i = maxLen_clientName;

    strncpy(clientName, (char *)client_name.value, i);
    j = client_name.length;
    if (maxLen_clientName > j)
        clientName[client_name.length] = '\0';

    /* release the name structure */
    majorStatus =    GSS_RELEASE_NAME(&minorStatus, &client);

    if (majorStatus != GSS_S_COMPLETE) {
        _ikrbLogError("releasing name", majorStatus, minorStatus);
        return KRB_ERROR_RELEASING_NAME;
    }

    (void)    GSS_RELEASE_BUFFER(&minorStatus, &client_name);

#if defined(IKRB_TIMING)
    (void) gettimeofday(&endTimeFunc, (struct timezone *) 0);
    (void) _ikrbTime(&sTimeDiff, &endTimeFunc, &startTimeFunc);
    fSec =
        (float) sTimeDiff.tv_sec + ((float) sTimeDiff.tv_usec / 1000000.0);

    /* for IKRB_TIMING  print time */
    fprintf(stdout, " ---  %.5f sec time taken for executing "
            "ikrbEstablishContextServerside() ---  \n", fSec);
#endif

    return 0;

#else
    if (ProcessType==CLIENT_PT) {
       return(KRB_NOT_BUILT_INTO_CLIENT);
    }
    else {
       return(KRB_NOT_BUILT_INTO_SERVER);
    }
#endif
}


/*
 Close out the context for the associated fd (socket).
 */
void ikrbClose(int fd)
{
#if defined(KRB_AUTH)
    OM_uint32 majorStatus, minorStatus;

    majorStatus =    GSS_DELETE_SEC_CONTEXT(&minorStatus, 
					    &context[fd], NULL);

    if (majorStatus != GSS_S_COMPLETE && ikrbDebugFlag > 0) {
       _ikrbLogError("deleting context", majorStatus, minorStatus);
       return;
    }
#endif
    return;
}

/*
 Close out all existing contextes and delete (erase) our credentials.
 */
void ikrbCloseAll()
{
#if defined(KRB_AUTH)
    OM_uint32 majorStatus, minorStatus;
    int i;

    if (myCreds != 0) {
        majorStatus =    GSS_RELEASE_CRED(&minorStatus, &myCreds);
        if (majorStatus != GSS_S_COMPLETE) {
            _ikrbLogError("releasing cred", majorStatus, minorStatus);
        }
    }

    for (i = 0; i < MAX_FDS; i++) {
        if (context[i] != GSS_C_NO_CONTEXT) {
            ikrbClose(i);
        }
    }
#endif
}

/*
 Set up an session between this client and a connected server.
 
 Establishses a GSS-API context, confirming the identity of the server
 as matching the input service_name string.  Confirms the this
 process's (the client's) identity to the server (based on the id
 specified in ikrbSetupCreds).

 The other side (the client) must call ikrbEstablishContextServerside
 at about the same time for the exchanges across the network to work
 (each side will block waiting for the other).  If unsuccessful, an
 error message is displayed and errorcode returned.

 The service_name is imported as a GSS-API name (translated to
 internal format) and a GSS-API context is established with the
 corresponding service The default GSS-API mechanism is used, and
 mutual authentication and replay detection are requested.
 
 If successful, the context handle is set in the global context array.
*/

int ikrbEstablishContextClientside(rcComm_t *Comm, char *serviceName, 
			       int delegFlag)
{
#if defined(KRB_AUTH)
    int fd;
    int status;

    fd = Comm->sock;

    ikrb_rErrorPtr = Comm->rError;

    gss_OID oid = GSS_C_NULL_OID;
    gss_buffer_desc send_tok, recv_tok, *tokenPtr, name_buffer;
    gss_name_t target_name;
    OM_uint32 majorStatus, minorStatus;
    OM_uint32 flags = 0;

#if defined(IKRB_TIMING)
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

       majorStatus =    GSS_IMPORT_NAME(&minorStatus, &name_buffer,
			       (gss_OID) gss_nt_service_name_krb,
			       &target_name);

       if (majorStatus != GSS_S_COMPLETE) {
       /* could use "if (GSS_ERROR(majorStatus))" but I believe it should
          always be GSS_S_COMPLETE if successful  */
          _ikrbLogError
            ("importing name (ikrbEstablishContextClientside)", majorStatus,
             minorStatus);
	  return KRB_ERROR_IMPORT_NAME; 
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
        if (ikrbDebugFlag > 0) 
            fprintf(stderr, "--> calling gss_init_sec_context\n");

	if (serviceName != 0 && strlen(serviceName)>0) {
	   majorStatus =    GSS_INIT_SEC_CONTEXT(&minorStatus,
                       myCreds, &context[fd], target_name, oid, 
                       flags, 0, 
                       NULL,   /* no channel bindings */
                       tokenPtr, NULL,    /* ignore mech type */
                       &send_tok, &context_flags, 
                       NULL);   /* ignore time_rec */
	}
	else {
	   majorStatus =    GSS_INIT_SEC_CONTEXT(&minorStatus,
                       myCreds, &context[fd], GSS_C_NO_NAME, oid, 
                       flags, 0, 
                       NULL,   /* no channel bindings */
                       tokenPtr, NULL,    /* ignore mech type */
                       &send_tok, &context_flags, 
                       NULL);   /* ignore time_rec */
	}

        /* since recv_tok is not malloc'ed, don't need to call
           gss_release_buffer, instead clear it. */
	memset (ikrbScratchBuffer, 0, SCRATCH_BUFFER_SIZE);

        if (majorStatus != GSS_S_COMPLETE
            && majorStatus != GSS_S_CONTINUE_NEEDED) {
	    _ikrbLogError("initializing context", 
			       majorStatus, minorStatus);
	    (void)    GSS_RELEASE_NAME(&minorStatus, &target_name);
            return KRB_ERROR_INIT_SECURITY_CONTEXT;
        }

        if (send_tok.length != 0) {
	    status = _krbSendToken(fd, &send_tok);
            if (status < 0) {
	        rodsLogAndErrorMsg( LOG_ERROR, ikrb_rErrorPtr, status,
                        "Sending init_sec_context token (size=%d)\n",
                        send_tok.length);
		(void)    GSS_RELEASE_BUFFER(&minorStatus, &send_tok);
		(void)    GSS_RELEASE_NAME(&minorStatus, &target_name);
		return status; /* KRB_SOCKET_WRITE_ERROR */
	    }
	}

        (void)    GSS_RELEASE_BUFFER(&minorStatus, &send_tok);

        if (majorStatus == GSS_S_CONTINUE_NEEDED) {
            if (ikrbDebugFlag > 0)
                fprintf(stderr, "continue needed...\n");
            recv_tok.value = &ikrbScratchBuffer;
            recv_tok.length = SCRATCH_BUFFER_SIZE;
            if (_ikrbRcvToken(fd, &recv_tok) <= 0) {
	       (void)    GSS_RELEASE_NAME(&minorStatus, &target_name);
                return KRB_ERROR_RELEASING_NAME;
            }
            tokenPtr = &recv_tok;
        }
    } while (majorStatus == GSS_S_CONTINUE_NEEDED);

    if (serviceName != 0 && strlen(serviceName)>0) {
       (void)    GSS_RELEASE_NAME(&minorStatus, &target_name);
    }

    if (ikrbDebugFlag > 0)
        _ikrbDisplayCtxFlags();

#if defined(IKRB_TIMING)
    (void) gettimeofday(&endTimeFunc, (struct timezone *) 0);
    (void) _ikrbTime(&sTimeDiff, &endTimeFunc, &startTimeFunc);
    fSec =
        (float) sTimeDiff.tv_sec + ((float) sTimeDiff.tv_usec / 1000000.0);

    /* for IKRB_TIMING  print time */
    fprintf(stdout, " ---  %.5f sec time taken for executing "
            "ikrbEstablishContextClientside() ---  \n", fSec);
#endif
    return 0;

#else
    if (ProcessType==CLIENT_PT) {
       return(KRB_NOT_BUILT_INTO_CLIENT);
    }
    else {
       return(KRB_NOT_BUILT_INTO_SERVER);
    }
#endif

}

/* 
 Set a the ikrbDebugFlag for displaying various levels of messages.
  0 - none (default)
  1 - some
  2 - more
 */
int ikrb_debug(int val)
{
    ikrbDebugFlag = val;
    return(0);
}
