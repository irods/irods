/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* miscServerFunct.c - misc server functions
 */


#ifndef windows_platform
#include <sys/wait.h>
#endif


#include "miscServerFunct.h"
#include "dataObjOpen.h"
#include "dataObjLseek.h"	
#include "dataObjOpr.h"
#include "dataObjClose.h"
#include "dataObjWrite.h"
#include "dataObjRead.h"
#include "rcPortalOpr.h"
#include "initServer.h"
#ifdef PARA_OPR
#ifdef USE_BOOST
#include <boost/thread/thread.hpp>
#else
#include <pthread.h>
#endif	/* USE_BOOST */
#endif
#if !defined(solaris_platform)
char *__loc1;
#endif /* linux_platform */
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_stacktrace.h"
#include "eirods_network_factory.h"

int
svrToSvrConnectNoLogin (rsComm_t *rsComm, rodsServerHost_t *rodsServerHost)
{
    rErrMsg_t errMsg;
    int reconnFlag;


    if (rodsServerHost->conn == NULL) { /* a connection already */
	if (getenv (RECONNECT_ENV) != NULL) {
	    reconnFlag = RECONN_TIMEOUT;
	} else {
	    reconnFlag = NO_RECONN;
	}
        rodsServerHost->conn = _rcConnect (rodsServerHost->hostName->name,
          ((zoneInfo_t *) rodsServerHost->zoneInfo)->portNum,
          rsComm->myEnv.rodsUserName, rsComm->myEnv.rodsZone,
          rsComm->clientUser.userName, rsComm->clientUser.rodsZone, &errMsg,
	   rsComm->connectCnt, reconnFlag);

        if (rodsServerHost->conn == NULL) {
            if (errMsg.status < 0) {
                return (errMsg.status);
            } else {
                return (SYS_SVR_TO_SVR_CONNECT_FAILED - errno);
            }
        }
    }

    return (rodsServerHost->localFlag);
}

int
svrToSvrConnect (rsComm_t *rsComm, rodsServerHost_t *rodsServerHost)
{
    int status;

    status = svrToSvrConnectNoLogin (rsComm, rodsServerHost);

    if (status < 0) {
	return status;
    }
 
    status = clientLogin (rodsServerHost->conn);
    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "svrToSvrConnect: clientLogin to %s failed",
          rodsServerHost->hostName->name);
        return (status);
    } else {
        return (rodsServerHost->localFlag);
    }
}

/* setupSrvPortalForParaOpr - Setup the portal on this server for
 * parallel or RBUDP transfer. It call createSrvPortal to create
 * the portal socket, malloc the portalOprOut struct, put the
 * server address, portNumber, etc in this struct. Aslo malloc
 * the rsComm->portalOpr struct and fill in the struct. This struct
 * will be used later by the server after sending reply to the client.
 * If RBUDP_TRANSFER_KW is set in dataOprInp->condInput, RBUDP transfer
 * is assumed.
 */

int
setupSrvPortalForParaOpr (rsComm_t *rsComm, dataOprInp_t *dataOprInp,
int oprType, portalOprOut_t **portalOprOut)
{
    portalOprOut_t *myDataObjPutOut;
    int portalSock;
    int proto;

#ifdef RBUDP_TRANSFER
    if (getValByKey (&dataOprInp->condInput, RBUDP_TRANSFER_KW) != NULL) {
        proto = SOCK_DGRAM;
    } else {
        proto = SOCK_STREAM;
    }
#else
    proto = SOCK_STREAM;
#endif  /* RBUDP_TRANSFER */

    myDataObjPutOut = (portalOprOut_t *) malloc (sizeof (portalOprOut_t));
    memset (myDataObjPutOut, 0, sizeof (portalOprOut_t));

    *portalOprOut = myDataObjPutOut;

    if (getValByKey (&dataOprInp->condInput, STREAMING_KW) != NULL ||
      proto == SOCK_DGRAM) {
        /* streaming or udp - use only one thread */
        myDataObjPutOut->numThreads = 1;
    } else {
        myDataObjPutOut->numThreads = getNumThreads (rsComm,
           dataOprInp->dataSize, dataOprInp->numThreads,
           &dataOprInp->condInput, 
           //getValByKey (&dataOprInp->condInput, RESC_NAME_KW), NULL);
           getValByKey (&dataOprInp->condInput,  RESC_HIER_STR_KW), NULL);
    }

    if (myDataObjPutOut->numThreads == 0) {
        return 0;
    } else {
        portalOpr_t *myPortalOpr;

        /* setup the portal */
        portalSock = createSrvPortal (rsComm, &myDataObjPutOut->portList,
          proto);
        if (portalSock < 0) {
            rodsLog (LOG_NOTICE,
              "setupSrvPortalForParaOpr: createSrvPortal error, ststus = %d", 
	      portalSock);
              myDataObjPutOut->status = portalSock;
              return portalSock;
        }
        myPortalOpr = rsComm->portalOpr =
          (portalOpr_t *) malloc (sizeof (portalOpr_t));
        myPortalOpr->oprType = oprType;
        myPortalOpr->portList = myDataObjPutOut->portList;
        myPortalOpr->dataOprInp = *dataOprInp;
        memset (&dataOprInp->condInput, 0, sizeof (dataOprInp->condInput));
        myPortalOpr->dataOprInp.numThreads = myDataObjPutOut->numThreads;
    }
    return (0);
}

/* createSrvPortal - create a server socket portal.
 * proto can be SOCK_STREAM or SOCK_DGRAM.
 * if proto == SOCK_DGRAM, create a tcp (control) and a udp socket
 */

int
createSrvPortal (rsComm_t *rsComm, portList_t *thisPortList, int proto)
{
    int lsock = -1;
    int lport = 0;
    char *laddr = NULL;
    int udpsock = -1;
    int udpport = 0;
    char *udpaddr = NULL;

    
    if (proto != SOCK_DGRAM && proto != SOCK_STREAM) {
        rodsLog (LOG_ERROR,
         "createSrvPortal: invalid input protocol %d", proto);
        return SYS_INVALID_PROTOCOL_TYPE;
    }

    if ((lsock = svrSockOpenForInConn (rsComm, &lport, &laddr, 
      SOCK_STREAM)) < 0) {
        rodsLog (LOG_ERROR,
         "createSrvPortal: svrSockOpenForInConn failed: status=%d",
          lsock);
        return lsock;
    }

    thisPortList->sock = lsock;
    thisPortList->cookie = random ();
    if (ProcessType == CLIENT_PT) {
        rstrcpy (thisPortList->hostAddr, laddr, LONG_NAME_LEN);
    } else {
        struct hostent *hostEnt;
        /* server. try to use what is configured */
        if (LocalServerHost != NULL &&
	 strcmp (LocalServerHost->hostName->name, "localhost") != 0 &&
         (hostEnt = gethostbyname (LocalServerHost->hostName->name)) != NULL){
            rstrcpy (thisPortList->hostAddr, hostEnt->h_name, LONG_NAME_LEN);
        } else {
             rstrcpy (thisPortList->hostAddr, laddr, LONG_NAME_LEN);
        }
    }
    free (laddr);
    thisPortList->portNum = lport;
    thisPortList->windowSize = rsComm->windowSize;

    listen (lsock, SOMAXCONN);

    if (proto == SOCK_DGRAM) {
        if ((udpsock = svrSockOpenForInConn (rsComm, &udpport, &udpaddr, 
          SOCK_DGRAM)) < 0) {
            rodsLog (LOG_ERROR,
             "setupSrvPortal- sockOpenForInConn of SOCK_DGRAM failed: stat=%d",
              udpsock);
	    CLOSE_SOCK (lsock);
            return udpsock;
	} else {
            addUdpPortToPortList (thisPortList, udpport);
            addUdpSockToPortList (thisPortList, udpsock);
	}
    }
    free (udpaddr);
 
    return (lsock);
}

int
acceptSrvPortal (rsComm_t *rsComm, portList_t *thisPortList)
{
    int myFd = -1;
    int myCookie;
    int nbytes;
    fd_set basemask;
    int nSockets, nSelected;
    int lsock = getTcpSockFromPortList (thisPortList);
	struct timeval selectTimeout;

    nSockets = lsock + 1;
    FD_ZERO(&basemask);
    FD_SET(lsock, &basemask);
    


    selectTimeout.tv_sec = SELECT_TIMEOUT_FOR_CONN;
    selectTimeout.tv_usec = 0;

    while ((nSelected = select(nSockets, &basemask,
      (fd_set *) NULL, (fd_set *) NULL, &selectTimeout)) < 0) {
        if (errno == EINTR) {
            rodsLog (LOG_ERROR, "acceptSrvPortal: select interrupted\n");
            continue;
        }
        rodsLog (LOG_ERROR, "acceptSrvPortal: select select failed, errno = %d",
          errno);
    }
    myFd = accept (lsock, 0, 0);
    if (myFd < 0) {
        rodsLog (LOG_NOTICE,
         "acceptSrvPortal() -- accept() failed: errno=%d",
          errno);
        return SYS_SOCK_ACCEPT_ERR - errno;
    } else {
	rodsSetSockOpt (myFd, rsComm->windowSize);
    }
#ifdef _WIN32
    nbytes = recv (myFd,&myCookie,sizeof(myCookie),0);
#else
    nbytes = read (myFd, &myCookie,sizeof (myCookie));
#endif
    myCookie = ntohl (myCookie);
    if (nbytes != sizeof (myCookie) || myCookie != thisPortList->cookie) {
        rodsLog (LOG_NOTICE,
         "acceptSrvPortal: cookie err, bytes read=%d,cookie=%d,inCookie=%d",
          nbytes, thisPortList->cookie, myCookie);
	CLOSE_SOCK (myFd);
        return SYS_PORT_COOKIE_ERR;
    }
    return (myFd);
}

int
svrPortalPutGet (rsComm_t *rsComm)
{
    portalOpr_t *myPortalOpr;
    dataOprInp_t *dataOprInp;
    portList_t *thisPortList;
    rodsLong_t size0, size1, offset0;
    int lsock, portalFd;
    int i;
    int numThreads;
    portalTransferInp_t myInput[MAX_NUM_CONFIG_TRAN_THR];
#ifdef PARA_OPR
	#ifdef USE_BOOST
	    boost::thread* tid[MAX_NUM_CONFIG_TRAN_THR];
	#else
	    pthread_t tid[MAX_NUM_CONFIG_TRAN_THR];
	#endif
#endif
    int oprType;
    int flags = 0;
    int retVal = 0;
    
    myPortalOpr = rsComm->portalOpr;

    if (myPortalOpr == NULL) {
        rodsLog (LOG_NOTICE, "svrPortalPut: NULL myPortalOpr");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    thisPortList = &myPortalOpr->portList;
    if (thisPortList == NULL) {
        rodsLog (LOG_NOTICE, "svrPortalPut: NULL portList");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

   if (getUdpPortFromPortList (thisPortList) != 0) {
#ifdef RBUDP_TRANSFER
        /* rbudp transfer */
        retVal = svrPortalPutGetRbudp (rsComm);
	return retVal;
#else
	return SYS_UDP_NO_SUPPORT_ERR;
#endif  /* RBUDP_TRANSFER */
    }

    oprType = myPortalOpr->oprType;
    dataOprInp = &myPortalOpr->dataOprInp;

    if (getValByKey (&dataOprInp->condInput, STREAMING_KW)!= NULL) {
	flags |= STREAMING_FLAG;
    }

    numThreads = dataOprInp->numThreads;

    if (numThreads <= 0 || numThreads > MAX_NUM_CONFIG_TRAN_THR) {
        rodsLog (LOG_NOTICE, 
	  "svrPortalPut: numThreads %d out of range");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    memset (myInput, 0, sizeof (myInput));
#ifdef PARA_OPR
    memset (tid, 0, sizeof (tid));
#endif

    size0 = dataOprInp->dataSize / numThreads;

    size1 = dataOprInp->dataSize - size0 * (numThreads - 1);
    offset0 = dataOprInp->offset;

    lsock = getTcpSockFromPortList (thisPortList);

    /* accept the first connection */
    portalFd = acceptSrvPortal (rsComm, thisPortList);
    if (portalFd < 0) {
	rodsLog (LOG_NOTICE, 
	  "svrPortalPut: acceptSrvPortal error. errno = %d", 
	  errno);
	
        CLOSE_SOCK (lsock);

        return (portalFd);
    }

    if (oprType == PUT_OPR) {
        fillPortalTransferInp (&myInput[0], rsComm,
         portalFd, dataOprInp->destL3descInx, 0, dataOprInp->destRescTypeInx,
          0, size0, offset0, flags);
    } else {
        fillPortalTransferInp (&myInput[0], rsComm,
         dataOprInp->srcL3descInx, portalFd, dataOprInp->srcRescTypeInx, 0,
          0, size0, offset0, flags);
    }

        if (numThreads == 1) {
            if (oprType == PUT_OPR) {
                partialDataPut (&myInput[0]);
        } else {
                partialDataGet (&myInput[0]);
        }
        CLOSE_SOCK (lsock);

	    return (myInput[0].status);
    } else {
#ifdef PARA_OPR
        rodsLong_t mySize = 0;
        rodsLong_t myOffset = 0;

        for (i = 1; i < numThreads; i++) {
            int l3descInx;

            portalFd = acceptSrvPortal (rsComm, thisPortList);
            if (portalFd < 0) {
                rodsLog (LOG_NOTICE,
                "svrPortalPut: acceptSrvPortal error. errno = %d",
                 errno);

                CLOSE_SOCK (lsock);

                return (portalFd);
            }

            myOffset += size0;

            if (i < numThreads - 1) {
                mySize = size0;
            } else {
                mySize = size1;
            }

            if (oprType == PUT_OPR) {
                /* open the file */ 
                l3descInx = l3OpenByHost (rsComm, dataOprInp->destRescTypeInx, 
                 dataOprInp->destL3descInx, O_WRONLY); 
                    fillPortalTransferInp (&myInput[i], rsComm,
             portalFd, l3descInx, 0, dataOprInp->destRescTypeInx,
                  i, mySize, myOffset, flags);
            #ifdef USE_BOOST
            tid[i] = new boost::thread( partialDataPut, &myInput[i] );
            #else
                    pthread_create (&tid[i], pthread_attr_default,
                     (void *(*)(void *)) partialDataPut, (void *) &myInput[i]);
                #endif             

            } else {	/* a get */
                    l3descInx = l3OpenByHost (rsComm, dataOprInp->srcRescTypeInx,
                     dataOprInp->srcL3descInx, O_RDONLY);
                    fillPortalTransferInp (&myInput[i], rsComm,
             l3descInx, portalFd, dataOprInp->srcRescTypeInx, 0,
                      i, mySize, myOffset, flags);
            #ifdef USE_BOOST
            tid[i] = new boost::thread( partialDataGet, &myInput[i] );
            #else
                    pthread_create (&tid[i], pthread_attr_default,
                     (void *(*)(void *)) partialDataGet, (void *) &myInput[i]);
            #endif
            }
        } // for i

            /* spawn the first thread. do this last so the file will not be
         * closed */
        if (oprType == PUT_OPR) {
            #ifdef USE_BOOST
            tid[0] = new boost::thread( partialDataPut, &myInput[0] );
                #else
                pthread_create (&tid[0], pthread_attr_default,
                 (void *(*)(void *)) partialDataPut, (void *) &myInput[0]);
            #endif
        } else {
            #ifdef USE_BOOST
            tid[0] = new boost::thread( partialDataGet, &myInput[0] );
                #else
                pthread_create (&tid[0], pthread_attr_default,
                 (void *(*)(void *)) partialDataGet, (void *) &myInput[0]);
            #endif
        }

        for ( i = 0; i < numThreads; i++) { 
            if (tid[i] != 0)
            #ifdef USE_BOOST
            tid[i]->join();
            #else
                    pthread_join (tid[i], NULL);
            #endif
            if (myInput[i].status < 0) {
                retVal = myInput[i].status;
            }
        } // for i
        CLOSE_SOCK (lsock);
	    return (retVal);

#else	/* PARA_OPR */
        CLOSE_SOCK (lsock);
	    return (SYS_PARA_OPR_NO_SUPPORT);
#endif	/* PARA_OPR */

    } // else
}

int
fillPortalTransferInp (portalTransferInp_t *myInput, rsComm_t *rsComm,
int srcFd, int destFd, int srcRescTypeInx, int destRescTypeInx,
int threadNum, rodsLong_t size, rodsLong_t offset, int flags)
{
    if (myInput == NULL) 
        return (SYS_INTERNAL_NULL_INPUT_ERR);

    myInput->rsComm = rsComm;
    myInput->destFd = destFd;
    myInput->srcFd = srcFd;
    myInput->destRescTypeInx = destRescTypeInx;
    myInput->srcRescTypeInx = srcRescTypeInx;

    myInput->threadNum = threadNum;
    myInput->size = size;
    myInput->offset = offset;
    myInput->flags = flags;

    return (0);
}


void
partialDataPut (portalTransferInp_t *myInput)
{
    int destL3descInx, srcFd, destRescTypeInx;
    char *buf;
    int bytesWritten;
    rodsLong_t bytesToGet;
    rodsLong_t myOffset = 0;

#ifdef PARA_TIMING
    time_t startTime, afterSeek, afterTransfer,
      endTime;
    startTime=time (0);
#endif

    if (myInput == NULL) {
	rodsLog (LOG_SYS_FATAL, "partialDataPut: NULL myInput");
	return;
    }
 
    myInput->status = 0;
    destL3descInx = myInput->destFd;
    srcFd = myInput->srcFd;
    destRescTypeInx = myInput->destRescTypeInx;

    if (myInput->offset != 0) {
        myOffset = _l3Lseek (myInput->rsComm, destRescTypeInx, 
	  destL3descInx, myInput->offset, SEEK_SET);
        if (myOffset < 0) {
	    myInput->status = myOffset;
            rodsLog (LOG_NOTICE,
	      "_partialDataPut: _objSeek error, status = %d ",
              myInput->status);
	    if (myInput->threadNum > 0)
                _l3Close (myInput->rsComm, destRescTypeInx, destL3descInx);
            CLOSE_SOCK (srcFd);
            return;
        }
    }
    buf = (char*)malloc (TRANS_BUF_SZ);

#ifdef PARA_TIMING
    afterSeek=time(0);
#endif

    bytesToGet = myInput->size;

    while (bytesToGet > 0) {
        int toread0;
        int bytesRead;

#ifdef PARA_TIMING
        time_t tstart, tafterRead, tafterWrite;
        tstart=time(0);
#endif
	if (myInput->flags & STREAMING_FLAG) {
	    toread0 = bytesToGet;
	} else if (bytesToGet > TRANS_SZ) {
	    toread0 = TRANS_SZ;
        } else {
            toread0 = bytesToGet;
        }

	myInput->status = sendTranHeader (srcFd, PUT_OPR, myInput->flags,
	  myOffset, toread0);

	if (myInput->status < 0) {
	    rodsLog (LOG_NOTICE, 
	      "partialDataPut: sendTranHeader error. status = %d", 
	      myInput->status);
	    if (myInput->threadNum > 0)
                _l3Close (myInput->rsComm, destRescTypeInx, destL3descInx);
            CLOSE_SOCK (srcFd);
	    free (buf);
	    return;
	} 

	while (toread0 > 0) {
	    int toread1;

	    if (toread0 > TRANS_BUF_SZ) {
		toread1 = TRANS_BUF_SZ;
	    } else {
		toread1 = toread0;
	    }
            bytesRead = myRead (srcFd, buf, toread1, SOCK_TYPE, NULL, NULL);

#ifdef PARA_TIMING
            tafterRead=time(0);
#endif
            if (bytesRead == toread1) {
                if ((bytesWritten = _l3Write (myInput->rsComm, destRescTypeInx,
		  destL3descInx, buf, bytesRead)) != bytesRead) {
		    rodsLog (LOG_NOTICE,
                     "_partialDataPut:Bytes written %d don't match read %d",
                      bytesWritten, bytesRead);

                    if (bytesWritten < 0) {
                        myInput->status = bytesWritten;
                    } else {
                        myInput->status = SYS_COPY_LEN_ERR;
                    }
                    break;
                }
                bytesToGet -= bytesWritten;
		toread0 -= bytesWritten;
                myOffset += bytesWritten;
            } else if (bytesRead < 0) {
                myInput->status = bytesRead;
                break;
            } else {        /* toread > 0 */
		rodsLog (LOG_NOTICE,
                 "_partialDataPut: toread %d bytes, %d bytes read, errno = %d",
                   toread1, bytesRead, errno);
		myInput->status = SYS_COPY_LEN_ERR;
                break;
            }
#ifdef PARA_TIMING
            tafterWrite=time(0);
	    rodsLog (LOG_NOTICE,
              "Thr %d: sz=%d netReadTm=%d diskWriteTm=%d",
              myInput->threadNum, bytesWritten, tafterRead-tstart,
              tafterWrite-tafterRead);
#endif
	}	/* while loop toread0 */
	if (myInput->status < 0)
            break;
    }           /* while loop bytesToGet */
#ifdef PARA_TIMING
    afterTransfer=time(0);
#endif
    free (buf);
    sendTranHeader (srcFd, DONE_OPR, 0, 0, 0);
    if (myInput->threadNum > 0)
        _l3Close (myInput->rsComm, destRescTypeInx, destL3descInx);
    mySockClose (srcFd);
#ifdef PARA_TIMING
    endTime=time(0);
    rodsLog (LOG_NOTICE,
      "Thr %d: seekTm=%d transTm=%d endTm=%d",
      myInput->threadInx,
      afterSeek-afterConn, afterTransfer-afterSeek, endTime-afterTransfer);
#endif
    return;
}

void
partialDataGet (portalTransferInp_t *myInput)
{
    int srcL3descInx, destFd, srcRescTypeInx;
    char *buf;
    int bytesWritten;
    rodsLong_t bytesToGet;
    rodsLong_t myOffset = 0;

#ifdef PARA_TIMING
    time_t startTime, afterSeek, afterTransfer,
      endTime;
    startTime=time (0);
#endif

    if (myInput == NULL) {
        rodsLog (LOG_SYS_FATAL, "partialDataGet: NULL myInput");
        return;
    }

    myInput->status = 0;
    srcL3descInx = myInput->srcFd;
    destFd = myInput->destFd;
    srcRescTypeInx = myInput->srcRescTypeInx;

    if (myInput->offset != 0) {
        myOffset = _l3Lseek (myInput->rsComm, srcRescTypeInx,
          srcL3descInx, myInput->offset, SEEK_SET);
        if (myOffset < 0) {
            myInput->status = myOffset;
            rodsLog (LOG_NOTICE,
              "_partialDataGet: _objSeek error, status = %d ",
              myInput->status);
            if (myInput->threadNum > 0)
                _l3Close (myInput->rsComm, srcRescTypeInx, srcL3descInx);
            CLOSE_SOCK (destFd);
            return;
        }
    }
    buf = (char*)malloc (TRANS_BUF_SZ);

#ifdef PARA_TIMING
    afterSeek=time(0);
#endif

    bytesToGet = myInput->size;

    while (bytesToGet > 0) {
        int toread0;
        int bytesRead;

#ifdef PARA_TIMING
        time_t tstart, tafterRead, tafterWrite;
        tstart=time(0);
#endif
        if (myInput->flags & STREAMING_FLAG) {
            toread0 = bytesToGet;
        } else if (bytesToGet > TRANS_SZ) {
            toread0 = TRANS_SZ;
        } else {
            toread0 = bytesToGet;
        }

        myInput->status = sendTranHeader (destFd, GET_OPR, myInput->flags,
          myOffset, toread0);

        if (myInput->status < 0) {
            rodsLog (LOG_NOTICE,
              "partialDataGet: sendTranHeader error. status = %d",
              myInput->status);
            if (myInput->threadNum > 0)
                _l3Close (myInput->rsComm, srcRescTypeInx, srcL3descInx);
            CLOSE_SOCK (destFd);
            free (buf);
            return;
        }

        while (toread0 > 0) {
            int toread1;

            if (toread0 > TRANS_BUF_SZ) {
                toread1 = TRANS_BUF_SZ;
            } else {
                toread1 = toread0;
            }
	    bytesRead = _l3Read (myInput->rsComm, srcRescTypeInx,
             srcL3descInx, buf, toread1);

#ifdef PARA_TIMING
            tafterRead=time(0);
#endif
            if (bytesRead == toread1) {
                if ((bytesWritten = myWrite (destFd, buf, bytesRead,
		  SOCK_TYPE, NULL))
                  != bytesRead) {
                    rodsLog (LOG_NOTICE,
                     "_partialDataGet:Bytes written %d don't match read %d",
                      bytesWritten, bytesRead);

                    if (bytesWritten < 0) {
                        myInput->status = bytesWritten;
                    } else {
                        myInput->status = SYS_COPY_LEN_ERR;
                    }
                    break;
                }
                bytesToGet -= bytesWritten;
                toread0 -= bytesWritten;
                myOffset += bytesWritten;
            } else if (bytesRead < 0) {
                myInput->status = bytesRead;
                break;
            } else {        /* toread > 0 */
                rodsLog (LOG_NOTICE,
                 "_partialDataGet: toread %d bytes, %d bytes read",
                   toread1, bytesRead);
                myInput->status = SYS_COPY_LEN_ERR;
                break;
            }
#ifdef PARA_TIMING
            tafterWrite=time(0);
            rodsLog (LOG_NOTICE,
              "Thr %d: sz=%d netReadTm=%d diskWriteTm=%d",
              myInput->threadNum, bytesWritten, tafterRead-tstart,
              tafterWrite-tafterRead);
#endif
        }       /* while loop toread0 */
        if (myInput->status < 0)
            break;
    }           /* while loop bytesToGet */
#ifdef PARA_TIMING
    afterTransfer=time(0);
#endif
    free (buf);
    sendTranHeader (destFd, DONE_OPR, 0, 0, 0);
    if (myInput->threadNum > 0)
        _l3Close (myInput->rsComm, srcRescTypeInx, srcL3descInx);
    CLOSE_SOCK (destFd);
#ifdef PARA_TIMING
    endTime=time(0);
    rodsLog (LOG_NOTICE,
      "Thr %d: seekTm=%d transTm=%d endTm=%d",
      myInput->threadInx,
      afterSeek-afterConn, afterTransfer-afterSeek, endTime-afterTransfer);
#endif
    return;
}

void
remToLocPartialCopy (portalTransferInp_t *myInput)
{
    transferHeader_t myHeader;
    int destL3descInx, srcFd, destRescTypeInx;
    void *buf;
    rodsLong_t curOffset = 0;
    rodsLong_t myOffset = 0;
    int toRead, bytesRead, bytesWritten;

    if (myInput == NULL) {
        rodsLog (LOG_NOTICE,
         "remToLocPartialCopy: NULL input");
        return;
    }
#ifdef PARA_DEBUG
    printf ("remToLocPartialCopy: thread %d at start\n", myInput->threadNum);
#endif

    myInput->status = 0;
    destL3descInx = myInput->destFd;
    srcFd = myInput->srcFd;
    destRescTypeInx = myInput->destRescTypeInx;
    myInput->bytesWritten = 0;

    buf = malloc (TRANS_BUF_SZ);

    while (myInput->status >= 0) {
        rodsLong_t toGet;

        myInput->status = rcvTranHeader (srcFd, &myHeader);

#ifdef PARA_DEBUG
        printf ("remToLocPartialCopy: thread %d after rcvTranHeader\n",
          myInput->threadNum);
        printf ("remToLocPartialCopy: thread %d header offset %lld, len %lld\n",
          myInput->threadNum, myHeader.offset, myHeader.length);

#endif

        if (myInput->status < 0) {
            break;
        }

        if (myHeader.oprType == DONE_OPR) {
            break;
        }
        if (myHeader.offset != curOffset) {
            curOffset = myHeader.offset;
            myOffset = _l3Lseek (myInput->rsComm, destRescTypeInx,
              destL3descInx, myHeader.offset, SEEK_SET);
            if (myOffset < 0) {
                myInput->status = myOffset;
                rodsLog (LOG_NOTICE,
                  "remToLocPartialCopy: _objSeek error, status = %d ",
                  myInput->status);
                break;
	    }
        }

        toGet = myHeader.length;
        while (toGet > 0) {

            if (toGet > TRANS_BUF_SZ) {
                toRead = TRANS_BUF_SZ;
            } else {
                toRead = toGet;
            }

            bytesRead = myRead (srcFd, buf, toRead,
		  SOCK_TYPE, NULL, NULL);
            if (bytesRead != toRead) {
		if (bytesRead < 0) {
		    myInput->status = bytesRead;
		    rodsLogError (LOG_ERROR, bytesRead,
                  "remToLocPartialCopy: copy error for %lld", bytesRead);
		} else if ((myInput->flags & NO_CHK_COPY_LEN_FLAG) == 0) {
                    myInput->status = SYS_COPY_LEN_ERR - errno;
                    rodsLog (LOG_ERROR,
                      "remToLocPartialCopy: toGet %lld, bytesRead %d",
                      toGet, bytesRead);
		}
                break;
            }

	    bytesWritten = _l3Write (myInput->rsComm, destRescTypeInx,
              destL3descInx, buf, bytesRead);

            if (bytesWritten != bytesRead) {
                rodsLog (LOG_NOTICE,
                 "_partialDataPut:Bytes written %d don't match read %d",
                  bytesWritten, bytesRead);

                if (bytesWritten < 0) {
                    myInput->status = bytesWritten;
                } else {
                    myInput->status = SYS_COPY_LEN_ERR;
                }
                break;
            }

            toGet -= bytesWritten;
        }
        curOffset += myHeader.length;
        myInput->bytesWritten += myHeader.length;
    }

    free (buf);
    if (myInput->threadNum > 0)
        _l3Close (myInput->rsComm, destRescTypeInx, destL3descInx);
    CLOSE_SOCK (srcFd);
}

/* rbudpRemLocCopy - The rbudp version of remLocCopy.
 */

#ifdef RBUDP_TRANSFER
int
rbudpRemLocCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp)
{
    portalOprOut_t *portalOprOut;
    dataOprInp_t *dataOprInp;
    rodsLong_t dataSize;
    int oprType;
    int veryVerbose, sendRate, packetSize;
    char *tmpStr;
    int status;

    if (dataCopyInp == NULL) {
        rodsLog (LOG_NOTICE,
          "rbudpRemLocCopy: NULL dataCopyInp input");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    portalOprOut = &dataCopyInp->portalOprOut;
    dataOprInp = &dataCopyInp->dataOprInp;
    oprType = dataOprInp->oprType;
    dataSize = dataOprInp->dataSize;

    if (getValByKey (&dataOprInp->condInput, VERY_VERBOSE_KW) != NULL) {
        veryVerbose = 2;
    } else {
        veryVerbose = 0;
    }

    if ((tmpStr = getValByKey (&dataOprInp->condInput,
      RBUDP_PACK_SIZE_KW)) != NULL) {
        packetSize = atoi (tmpStr);
    } else {
        packetSize = DEF_UDP_PACKET_SIZE;
    }

    if (oprType == COPY_TO_LOCAL_OPR) {
	int destL3descInx = dataOprInp->destL3descInx;

        status = getFileToPortalRbudp (portalOprOut, NULL, 
	  FileDesc[destL3descInx].fd, dataSize, 
	  veryVerbose, packetSize);
    } else {
	int srcL3descInx = dataOprInp->srcL3descInx;

        if ((tmpStr = getValByKey (&dataOprInp->condInput,
          RBUDP_SEND_RATE_KW)) != NULL) {
            sendRate = atoi (tmpStr);
        } else {
            sendRate = DEF_UDP_SEND_RATE;
        }
        status = putFileToPortalRbudp (portalOprOut, NULL, NULL,
	  FileDesc[srcL3descInx].fd, dataSize, 
	  veryVerbose, sendRate, packetSize);
    }
    return (status);
}
#endif	/* RBUDP_TRANSFER */

/* remLocCopy - This routine is very similar to rcPartialDataGet.
 */

int
remLocCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp)
{
    portalOprOut_t *portalOprOut;
    dataOprInp_t *dataOprInp;
    portList_t *myPortList;
    int i, sock, myFd;
    int numThreads;
    portalTransferInp_t myInput[MAX_NUM_CONFIG_TRAN_THR];
#ifndef windows_platform
    #ifdef USE_BOOST
    boost::thread* tid[MAX_NUM_CONFIG_TRAN_THR];
    #else
    pthread_t tid[MAX_NUM_CONFIG_TRAN_THR];
    #endif
#endif
    int retVal = 0;
    rodsLong_t dataSize;
    int oprType;

    if (dataCopyInp == NULL) {
        rodsLog (LOG_NOTICE,
          "remLocCopy: NULL dataCopyInp input");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    portalOprOut = &dataCopyInp->portalOprOut;
    numThreads = portalOprOut->numThreads;
    if (numThreads == 0) {
	retVal = singleRemLocCopy (rsComm, dataCopyInp);
	return retVal;
    }

    dataOprInp = &dataCopyInp->dataOprInp;
    oprType = dataOprInp->oprType;
    dataSize = dataOprInp->dataSize;

    if (getUdpPortFromPortList (&portalOprOut->portList) != 0) {
        /* rbudp transfer */
#ifdef RBUDP_TRANSFER
	retVal = rbudpRemLocCopy (rsComm, dataCopyInp);
	return (retVal);
#else
	return (SYS_UDP_NO_SUPPORT_ERR);
#endif
    }

    if (numThreads > MAX_NUM_CONFIG_TRAN_THR || numThreads <= 0) {
       rodsLog (LOG_NOTICE,
         "remLocCopy: numThreads %d out of range",
         numThreads);
        return (SYS_INVALID_PORTAL_OPR);
    }


    myPortList = &portalOprOut->portList;

#ifndef windows_platform
    memset (tid, 0, sizeof (tid));
#endif
    memset (myInput, 0, sizeof (myInput));

    sock = connectToRhostPortal (myPortList->hostAddr,
      myPortList->portNum, myPortList->cookie, rsComm->windowSize);
    if (sock < 0) {
        return (sock);
    }

    if (oprType == COPY_TO_LOCAL_OPR) {
        fillPortalTransferInp (&myInput[0], rsComm,
         sock, dataOprInp->destL3descInx, 0, dataOprInp->destRescTypeInx,
          0, 0, 0, 0);
    } else {
        fillPortalTransferInp (&myInput[0], rsComm,
         dataOprInp->srcL3descInx, sock, dataOprInp->srcRescTypeInx, 0,
          0, 0, 0, 0);
    }

   if (numThreads == 1) {
        if (getValByKey (&dataOprInp->condInput,
          NO_CHK_COPY_LEN_KW) != NULL) {
            myInput[0].flags = NO_CHK_COPY_LEN_FLAG;
        }
	if (oprType == COPY_TO_LOCAL_OPR) {
            remToLocPartialCopy (&myInput[0]);
	} else {
	    locToRemPartialCopy (&myInput[0]);
	}
        if (myInput[0].status < 0) {
            return (myInput[0].status);
        } else {
            if (myInput[0].bytesWritten == dataSize) {
                return (0);
            } else {
                rodsLog (LOG_NOTICE,
                  "remLocCopy:bytesWritten %lld dataSize %lld mismatch",
                  myInput[0].bytesWritten, dataSize);
                return (SYS_COPY_LEN_ERR);
            }
        }
    } else {
#ifdef PARA_OPR
        rodsLong_t totalWritten = 0;

        for (i = 1; i < numThreads; i++) {
            sock = connectToRhostPortal (myPortList->hostAddr,
              myPortList->portNum, myPortList->cookie, rsComm->windowSize);
            if (sock < 0) {
                return (sock);
            }
	    if (oprType == COPY_TO_LOCAL_OPR) {
                myFd = l3OpenByHost (rsComm, dataOprInp->destRescTypeInx,
                 dataOprInp->destL3descInx, O_WRONLY);
                if (myFd < 0) {    /* error */
                    retVal = myFd;
                    rodsLog (LOG_NOTICE,
                    "remLocCopy: cannot open file, status = %d", 
	             myFd);
                    CLOSE_SOCK (sock);
                    continue;
                }

                fillPortalTransferInp (&myInput[i], rsComm,
                 sock, myFd, 0, dataOprInp->destRescTypeInx,
                 i, 0, 0, 0);

                #ifdef USE_BOOST
                tid[i] = new boost::thread( remToLocPartialCopy, &myInput[i] );
                #else
                pthread_create (&tid[i], pthread_attr_default,
                 (void *(*)(void *)) remToLocPartialCopy, (void *) &myInput[i]);
                #endif
	    } else {
                myFd = l3OpenByHost (rsComm, dataOprInp->srcRescTypeInx,
                 dataOprInp->srcL3descInx, O_RDONLY);
                if (myFd < 0) {    /* error */
                    retVal = myFd;
                    rodsLog (LOG_NOTICE,
                    "remLocCopy: cannot open file, status = %d",
                     myFd);
                    CLOSE_SOCK (sock);
                    continue;
                }

                fillPortalTransferInp (&myInput[i], rsComm,
                 myFd, sock, dataOprInp->destRescTypeInx, 0,
                 i, 0, 0, 0);

                #ifdef USE_BOOST
                tid[i] = new boost::thread( locToRemPartialCopy, &myInput[i] );
                #else
                pthread_create (&tid[i], pthread_attr_default,
                 (void *(*)(void *)) locToRemPartialCopy, (void *) &myInput[i]);
                #endif
            }
	}

	if (oprType == COPY_TO_LOCAL_OPR) {
	    #ifdef USE_BOOST
	    tid[0] = new boost::thread( remToLocPartialCopy,&myInput[0] );
	    #else
            pthread_create (&tid[0], pthread_attr_default,
             (void *(*)(void *)) remToLocPartialCopy, (void *) &myInput[0]);
            #endif
	} else {
	    #ifdef USE_BOOST
	    tid[0] = new boost::thread( locToRemPartialCopy, &myInput[0] );
	    #else
            pthread_create (&tid[0], pthread_attr_default,
             (void *(*)(void *)) locToRemPartialCopy, (void *) &myInput[0]);
            #endif
	}


        if (retVal < 0) {
            return (retVal);
        }

        for ( i = 0; i < numThreads; i++) {
            if (tid[i] != 0) {
    		#ifdef USE_BOOST
                tid[i]->join();
                #else
                pthread_join (tid[i], NULL);
                #endif
            }
            totalWritten += myInput[i].bytesWritten;
            if (myInput[i].status < 0) {
                retVal = myInput[i].status;
            }
        }
        if (retVal < 0) {
            return (retVal);
        } else {
            if (dataSize <= 0 || totalWritten == dataSize) {
                return (0);
            } else {
                rodsLog (LOG_NOTICE,
                 "remLocCopy: totalWritten %lld dataSize %lld mismatch",
                  totalWritten, dataSize);
                return (SYS_COPY_LEN_ERR);
            }
        }
#else   /* PARA_OPR */
        return (SYS_PARA_OPR_NO_SUPPORT);
#endif  /* PARA_OPR */
    }
}

int
sameHostCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp)
{
    dataOprInp_t *dataOprInp;
    int i, out_fd, in_fd;
    int numThreads;
    portalTransferInp_t myInput[MAX_NUM_CONFIG_TRAN_THR];
#ifndef windows_platform
    #ifdef USE_BOOST
    boost::thread* tid[MAX_NUM_CONFIG_TRAN_THR];
    #else
    pthread_t tid[MAX_NUM_CONFIG_TRAN_THR];
    #endif
#endif
    int retVal = 0;
    rodsLong_t dataSize;
    rodsLong_t size0, size1, offset0;

    if (dataCopyInp == NULL) {
        rodsLog (LOG_NOTICE,
          "sameHostCopy: NULL dataCopyInp input");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    dataOprInp = &dataCopyInp->dataOprInp;

    numThreads = dataOprInp->numThreads;

    dataSize = dataOprInp->dataSize;

    if (numThreads == 0) {
	numThreads = 1;
    } else if (numThreads > MAX_NUM_CONFIG_TRAN_THR || numThreads < 0) {
       rodsLog (LOG_NOTICE,
         "sameHostCopy: numThreads %d out of range",
         numThreads);
        return (SYS_INVALID_PORTAL_OPR);
    }

#ifndef windows_platform
    memset (tid, 0, sizeof (tid));
#endif
    memset (myInput, 0, sizeof (myInput));

    size0 = dataOprInp->dataSize / numThreads;
    size1 = dataOprInp->dataSize - size0 * (numThreads - 1);
    offset0 = dataOprInp->offset;

    fillPortalTransferInp (&myInput[0], rsComm,
     dataOprInp->srcL3descInx, dataOprInp->destL3descInx, 
     dataOprInp->srcRescTypeInx, dataOprInp->destRescTypeInx,
      0, size0, offset0, 0);

    if (numThreads == 1) {
	if (getValByKey (&dataOprInp->condInput,
          NO_CHK_COPY_LEN_KW) != NULL) {
	    myInput[0].flags = NO_CHK_COPY_LEN_FLAG;
	}
        sameHostPartialCopy (&myInput[0]);
        return (myInput[0].status);
    } else {
#ifdef PARA_OPR
        rodsLong_t totalWritten = 0;
        rodsLong_t mySize = 0;
        rodsLong_t myOffset = 0;

        for (i = 1; i < numThreads; i++) {
            myOffset += size0;
            if (i < numThreads - 1) {
                mySize = size0;
            } else {
                mySize = size1;
            }

            out_fd = l3OpenByHost (rsComm, dataOprInp->destRescTypeInx,
             dataOprInp->destL3descInx, O_WRONLY);
            if (out_fd < 0) {    /* error */
                retVal = out_fd;
                rodsLog (LOG_NOTICE,
                 "sameHostCopy: cannot open dest file, status = %d", 
		 out_fd);
                continue;
            }

            in_fd = l3OpenByHost (rsComm, dataOprInp->srcRescTypeInx,
             dataOprInp->srcL3descInx, O_RDONLY);
            if (in_fd < 0) {    /* error */
                retVal = out_fd;
                rodsLog (LOG_NOTICE,
                 "sameHostCopy: cannot open src file, status = %d", in_fd);
                continue;
            }
            fillPortalTransferInp (&myInput[i], rsComm,
             in_fd, out_fd, 
	     dataOprInp->srcRescTypeInx, dataOprInp->destRescTypeInx,
              i, mySize, myOffset, 0);

            #ifdef USE_BOOST
	    tid[i] = new boost::thread( sameHostPartialCopy, &myInput[i] );
            #else
            pthread_create (&tid[i], pthread_attr_default,
             (void *(*)(void *)) sameHostPartialCopy, (void *) &myInput[i]);
            #endif
        }

	#ifdef USE_BOOST
	tid[0] = new boost::thread( sameHostPartialCopy, &myInput[0] );
	#else
        pthread_create (&tid[0], pthread_attr_default, 
         (void *(*)(void *)) sameHostPartialCopy, (void *) &myInput[0]);
	#endif

        if (retVal < 0) {
            return (retVal);
        }

        for ( i = 0; i < numThreads; i++) {
            if (tid[i] != 0) {
	    #ifdef USE_BOOST
		tid[i]->join();
	    #else
                pthread_join (tid[i], NULL);
	    #endif
            }
            totalWritten += myInput[i].bytesWritten;
            if (myInput[i].status < 0) {
                retVal = myInput[i].status;
            }
        }
        if (retVal < 0) {
            return (retVal);
        } else {
            if (dataSize <= 0 || totalWritten == dataSize) {
                return (0);
            } else {
                rodsLog (LOG_NOTICE,
                 "sameHostCopy: totalWritten %lld dataSize %lld mismatch",
                  totalWritten, dataSize);
                return (SYS_COPY_LEN_ERR);
            }
        }
#else   /* PARA_OPR */
        return (SYS_PARA_OPR_NO_SUPPORT);
#endif  /* PARA_OPR */
    }
}

void
sameHostPartialCopy (portalTransferInp_t *myInput)
{
    int destL3descInx, srcL3descInx, destRescTypeInx, srcRescTypeInx;
    void *buf;
    rodsLong_t myOffset = 0;
    rodsLong_t toCopy;
    int bytesRead, bytesWritten;

    if (myInput == NULL) {
        rodsLog (LOG_NOTICE,
         "onsameHostPartialCopy: NULL input");
        return;
    }
#ifdef PARA_DEBUG
    printf ("onsameHostPartialCopy: thread %d at start\n", myInput->threadNum);
#endif

    myInput->status = 0;
    destL3descInx = myInput->destFd;
    srcL3descInx = myInput->srcFd;
    destRescTypeInx = myInput->destRescTypeInx;
    srcRescTypeInx = myInput->srcRescTypeInx;
    myInput->bytesWritten = 0;

    if (myInput->offset != 0) {
        myOffset = _l3Lseek (myInput->rsComm, destRescTypeInx,
          destL3descInx, myInput->offset, SEEK_SET);
        if (myOffset < 0) {
            myInput->status = myOffset;
            rodsLog (LOG_NOTICE,
              "sameHostPartialCopy: _objSeek error, status = %d ",
              myInput->status);
            if (myInput->threadNum > 0) {
                _l3Close (myInput->rsComm, destRescTypeInx, destL3descInx);
                _l3Close (myInput->rsComm, srcRescTypeInx, srcL3descInx);
            }
            return;
        }
        myOffset = _l3Lseek (myInput->rsComm, srcRescTypeInx,
          srcL3descInx, myInput->offset, SEEK_SET);
        if (myOffset < 0) {
            myInput->status = myOffset;
            rodsLog (LOG_NOTICE,
              "sameHostPartialCopy: _objSeek error, status = %d ",
              myInput->status);
            if (myInput->threadNum > 0) {
                _l3Close (myInput->rsComm, destRescTypeInx, destL3descInx);
                _l3Close (myInput->rsComm, srcRescTypeInx, srcL3descInx);
	    }
            return;
        }
    }

    buf = malloc (TRANS_BUF_SZ);

    toCopy = myInput->size;

    while (toCopy > 0) {
        int toRead;

        if (toCopy > TRANS_BUF_SZ) {
	    toRead = TRANS_BUF_SZ;
	} else {
	    toRead = toCopy;
	}

        bytesRead = _l3Read (myInput->rsComm, srcRescTypeInx,
         srcL3descInx, buf, toRead);

        if (bytesRead <= 0) {
            if (bytesRead < 0) {
                myInput->status = bytesRead;
                rodsLogError (LOG_ERROR, bytesRead,
              "sameHostPartialCopy: copy error for %lld", bytesRead);
            } else if ((myInput->flags & NO_CHK_COPY_LEN_FLAG) == 0) {
                myInput->status = SYS_COPY_LEN_ERR - errno;
                rodsLog (LOG_ERROR,
                  "sameHostPartialCopy: toCopy %lld, bytesRead %d",
                  toCopy, bytesRead);
            }
            break;
        }

	bytesWritten = _l3Write (myInput->rsComm, destRescTypeInx,
          destL3descInx, buf, bytesRead);

        if (bytesWritten != bytesRead) {
            rodsLog (LOG_NOTICE,
             "sameHostPartialCopy:Bytes written %d don't match read %d",
              bytesWritten, bytesRead);

            if (bytesWritten < 0) {
                myInput->status = bytesWritten;
            } else {
                myInput->status = SYS_COPY_LEN_ERR;
            }
            break;
        }

        toCopy -= bytesWritten;
        myInput->bytesWritten += bytesWritten;
    }

    free (buf);
    if (myInput->threadNum > 0) {
        _l3Close (myInput->rsComm, destRescTypeInx, destL3descInx);
        _l3Close (myInput->rsComm, srcRescTypeInx, srcL3descInx);
    }
}

void
locToRemPartialCopy (portalTransferInp_t *myInput)
{
    transferHeader_t myHeader;
    int srcL3descInx, destFd, srcRescTypeInx;
    void *buf;
    rodsLong_t curOffset = 0;
    rodsLong_t myOffset = 0;
    int toRead, bytesRead, bytesWritten;

    if (myInput == NULL) {
        rodsLog (LOG_NOTICE,
         "locToRemPartialCopy: NULL input");
        return;
    }
#ifdef PARA_DEBUG
    printf ("locToRemPartialCopy: thread %d at start\n", myInput->threadNum);
#endif

    myInput->status = 0;
    srcL3descInx = myInput->srcFd;
    destFd = myInput->destFd;
    srcRescTypeInx = myInput->srcRescTypeInx;
    myInput->bytesWritten = 0;

    buf = malloc (TRANS_BUF_SZ);

    while (myInput->status >= 0) {
        rodsLong_t toGet;

        myInput->status = rcvTranHeader (destFd, &myHeader);

#ifdef PARA_DEBUG
        printf ("locToRemPartialCopy: thread %d after rcvTranHeader\n",
          myInput->threadNum);
#endif

        if (myInput->status < 0) {
            break;
        }

        if (myHeader.oprType == DONE_OPR) {
            break;
        }
#ifdef PARA_DEBUG
	printf ("locToRemPartialCopy:thread %d header offset %lld, len %lld",
	  myInput->threadNum, myHeader.offset, myHeader.length); // JMC cppcheck - missing first parameter
#endif

        if (myHeader.offset != curOffset) {
            curOffset = myHeader.offset;
            myOffset = _l3Lseek (myInput->rsComm, srcRescTypeInx,
              srcL3descInx, myHeader.offset, SEEK_SET);
            if (myOffset < 0) {
                myInput->status = myOffset;
                rodsLog (LOG_NOTICE,
                  "locToRemPartialCopy: _objSeek error, status = %d ",
                  myInput->status);
                break;
            }
        }

        toGet = myHeader.length;
        while (toGet > 0) {

            if (toGet > TRANS_BUF_SZ) {
                toRead = TRANS_BUF_SZ;
            } else {
                toRead = toGet;
            }

	    bytesRead = _l3Read (myInput->rsComm, srcRescTypeInx,
              srcL3descInx, buf, toRead);

            if (bytesRead != toRead) {
                if (bytesRead < 0) {
                    myInput->status = bytesRead;
                    rodsLogError (LOG_ERROR, bytesRead,
                  "locToRemPartialCopy: copy error for %lld", bytesRead);
                } else if ((myInput->flags & NO_CHK_COPY_LEN_FLAG) == 0) {
                    myInput->status = SYS_COPY_LEN_ERR - errno;
                    rodsLog (LOG_ERROR,
                      "locToRemPartialCopy: toGet %lld, bytesRead %d",
                      toGet, bytesRead);
                }
                break;
            }

	    bytesWritten = myWrite (destFd, buf, bytesRead,
		  SOCK_TYPE, NULL);


            if (bytesWritten != bytesRead) {
                rodsLog (LOG_NOTICE,
                 "_partialDataPut:Bytes written %d don't match read %d",
                  bytesWritten, bytesRead);

                if (bytesWritten < 0) {
                    myInput->status = bytesWritten;
                } else {
                    myInput->status = SYS_COPY_LEN_ERR;
                }
                break;
            }

            toGet -= bytesWritten;
        }
        curOffset += myHeader.length;
        myInput->bytesWritten += myHeader.length;
    }

    free (buf);
    if (myInput->threadNum > 0)
        _l3Close (myInput->rsComm, srcRescTypeInx, srcL3descInx);
    CLOSE_SOCK (destFd);
}

/*
 Given a zoneName, return the Zone Server ID string (from the server.config
 file) if defined.  If the input zoneName is null, use the local zone.
 Input: zoneName
 Output: zoneSID
 */
void
getZoneServerId(char *zoneName, char *zoneSID) {
   zoneInfo_t *tmpZoneInfo;
   rodsServerHost_t *tmpRodsServerHost;
   int i;
   int zoneNameLen=0;
   char *localZoneName=NULL;
   char matchStr[MAX_NAME_LEN+2];

   if (zoneName!=NULL) zoneNameLen=strlen(zoneName);
   if (zoneNameLen==0) {
      strncpy(zoneSID, localSID, MAX_PASSWORD_LEN);
      return;
   }

   /* get our local zoneName */
   tmpZoneInfo = ZoneInfoHead;
   while (tmpZoneInfo != NULL) {
      tmpRodsServerHost = (rodsServerHost_t *) tmpZoneInfo->masterServerHost;
      if (tmpRodsServerHost->rcatEnabled == LOCAL_ICAT) {
	 localZoneName = tmpZoneInfo->zoneName;
      }
      tmpZoneInfo = tmpZoneInfo->next;
   }

   /* return the local SID if the local zone is the one requested */
   if (localZoneName!=NULL) {
      if (strncmp(localZoneName, zoneName, MAX_NAME_LEN)==0) {
	 strncpy(zoneSID, localSID, MAX_PASSWORD_LEN);
	 return;
      }
   }

   /* check the remoteSIDs; form is ZoneName-SID */
   strncpy(matchStr, zoneName, MAX_NAME_LEN);
   strncat(matchStr, "-", MAX_NAME_LEN);
   for (i=0;i<MAX_FED_RSIDS;i++) {
      if (strncmp(matchStr, remoteSID[i], zoneNameLen+1)==0) {
	 strncpy(zoneSID, (char*)&remoteSID[i][zoneNameLen+1],
		 MAX_PASSWORD_LEN);
	 return;
      }
   }

   zoneSID[0]='\0';
   return;
}

int
isUserPrivileged(rsComm_t *rsComm)
{

  if (rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
      return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
  }
  if (rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
      return(CAT_INSUFFICIENT_PRIVILEGE_LEVEL);
  }

  return(0);
}

#if !defined(solaris_platform)
char *regcmp (char *pat, char *end)
{
  return(NULL);
}

char *regex (char *rec, char *text, ...)
{
  return(NULL);
}
#endif  /* linux_platform */

/* generic functions to return SYS_NOT_SUPPORTED */

int
#ifdef  __cplusplus
intNoSupport( ... )
#else
intNoSupport()
#endif
{
    return SYS_NOT_SUPPORTED;
}

rodsLong_t
#ifdef  __cplusplus
longNoSupport( ... )
#else
longNoSupport()
#endif
{
    return (rodsLong_t) SYS_NOT_SUPPORTED;
}

#ifdef RBUDP_TRANSFER
int
svrPortalPutGetRbudp (rsComm_t *rsComm)
{
    portalOpr_t *myPortalOpr;
    portList_t *thisPortList;
    int lsock;
    int  tcpSock, udpSockfd;
    int udpPortBuf;
    int status;
#if defined(aix_platform)
    socklen_t      laddrlen = sizeof(struct sockaddr);
#elif defined(windows_platform)
        int laddrlen = sizeof(struct sockaddr);
#else
    uint         laddrlen = sizeof(struct sockaddr);
#endif
    int packetSize;
    char *tmpStr;
    int verbose;

    myPortalOpr = rsComm->portalOpr;

    if (myPortalOpr == NULL) {
        rodsLog (LOG_NOTICE, "svrPortalPutGetRbudp: NULL myPortalOpr");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    thisPortList = &myPortalOpr->portList;
    if (thisPortList == NULL) {
        rodsLog (LOG_NOTICE, "svrPortalPutGetRbudp: NULL portList");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    lsock = getTcpSockFromPortList (thisPortList);

    tcpSock =  acceptSrvPortal (rsComm, thisPortList);
    if (tcpSock < 0) {
        rodsLog (LOG_NOTICE,
          "svrPortalPutGetRbudp: acceptSrvPortal error. errno = %d",
          errno);
        CLOSE_SOCK (lsock);
        return (tcpSock);
    } else {
        CLOSE_SOCK (lsock);
    }
    status = readn (tcpSock, (char *) &udpPortBuf, sizeof (udpPortBuf));
    if (status != sizeof (udpPortBuf)) {
        rodsLog (LOG_ERROR,
         "svrPortalPutGetRbudp: readn error. toread %d, bytes read %d ",
          sizeof (udpPortBuf), status);
        return (SYS_UDP_CONNECT_ERR);
    }

    if ((tmpStr = getValByKey (&myPortalOpr->dataOprInp.condInput,
      RBUDP_PACK_SIZE_KW)) != NULL) {
        packetSize = atoi (tmpStr);
    } else {
        packetSize = DEF_UDP_PACKET_SIZE;
    }

    if (getValByKey (&myPortalOpr->dataOprInp.condInput, VERY_VERBOSE_KW) != 
      NULL) 
	verbose = 2;
    else
	verbose = 0;

    udpSockfd = getUdpSockFromPortList (thisPortList);

    checkbuf (udpSockfd, UDPSOCKBUF, verbose);
    if (myPortalOpr->oprType == PUT_OPR) {
        rbudpReceiver_t rbudpReceiver;
        bzero (&rbudpReceiver, sizeof (rbudpReceiver));
        int destL3descInx = myPortalOpr->dataOprInp.destL3descInx;

        rbudpReceiver.rbudpBase.verbose = verbose;
        rbudpReceiver.rbudpBase.udpSockBufSize = UDPSOCKBUF;
        rbudpReceiver.rbudpBase.tcpPort = getTcpPortFromPortList (thisPortList);
        rbudpReceiver.rbudpBase.tcpSockfd = tcpSock;
        rbudpReceiver.rbudpBase.udpSockfd = udpSockfd;
        rbudpReceiver.rbudpBase.hasTcpSock = 0;
        rbudpReceiver.rbudpBase.udpRemotePort = ntohl (udpPortBuf);
    /* use the addr of tcp sock */
        if (getpeername (tcpSock, 
          (struct sockaddr *) &rbudpReceiver.rbudpBase.udpServerAddr,
              &laddrlen) < 0) {
                rodsLog (LOG_NOTICE,
                  "svrPortalPutGetRbudp() - getpeername() failed: errno=%d", 
              errno);
                recvClose (&rbudpReceiver);
                return (USER_RODS_HOSTNAME_ERR);
        }

        rbudpReceiver.rbudpBase.udpServerAddr.sin_port = htons (rbudpReceiver.rbudpBase.udpRemotePort);
          
        status = getfileByFd (&rbudpReceiver, FileDesc[destL3descInx].fd,  packetSize);
        
        if (status < 0) {
            rodsLog (LOG_ERROR,
             "svrPortalPutGetRbudp: getfileByFd error for %s",
              FileDesc[destL3descInx].fileName);
	    status += SYS_UDP_TRANSFER_ERR; 
        }
        recvClose (&rbudpReceiver);
    } else if (myPortalOpr->oprType == GET_OPR) {

        int sendRate;
        rbudpSender_t rbudpSender;
        int srcL3descInx = myPortalOpr->dataOprInp.srcL3descInx;

        bzero (&rbudpSender, sizeof (rbudpSender));
        rbudpSender.rbudpBase.verbose = verbose;
        rbudpSender.rbudpBase.udpSockBufSize = UDPSOCKBUF;
        rbudpSender.rbudpBase.tcpPort = getTcpPortFromPortList (thisPortList);
        rbudpSender.rbudpBase.tcpSockfd = tcpSock;
        rbudpSender.rbudpBase.udpSockfd = udpSockfd;
        rbudpSender.rbudpBase.hasTcpSock = 0;
        rbudpSender.rbudpBase.udpRemotePort = ntohl (udpPortBuf);
        /* use the addr of tcp sock */
        if (getpeername (tcpSock,
          (struct sockaddr *) &rbudpSender.rbudpBase.udpServerAddr,
          &laddrlen) < 0) {
            rodsLog (LOG_NOTICE,
              "svrPortalPutGetRbudp() - getpeername() failed: errno=%d",
              errno);
            sendClose (&rbudpSender);
            return (USER_RODS_HOSTNAME_ERR);
        }
        rbudpSender.rbudpBase.udpServerAddr.sin_port = 
          htons (rbudpSender.rbudpBase.udpRemotePort);
        if ((tmpStr = getValByKey (&myPortalOpr->dataOprInp.condInput,
          RBUDP_SEND_RATE_KW)) != NULL) {
            sendRate = atoi (tmpStr);
        } else {
            sendRate = DEF_UDP_SEND_RATE;
        }

        status = sendfileByFd( &rbudpSender, sendRate, packetSize, FileDesc[srcL3descInx].fd );
          

        if (status < 0) {
            rodsLog (LOG_ERROR,
             "svrPortalPutGetRbudp: sendfile error for %s",
              FileDesc[srcL3descInx].fileName);
            status += SYS_UDP_TRANSFER_ERR;
        }
        sendClose (&rbudpSender);
    }

    return (status);
}
#endif  /* RBUDP_TRANSFER */
#ifndef windows_platform
void
reconnManager (rsComm_t *rsComm)
{
    fd_set basemask;
    int nSockets, nSelected;
    struct sockaddr_in  remoteAddr;
    socklen_t len;
    int newSock;
    reconnMsg_t *reconnMsg;
    int acceptFailCnt = 0;

    if (rsComm == NULL || rsComm->reconnSock <= 0) { 
        return;
    }

    listen (rsComm->reconnSock, 1);

    nSockets = rsComm->reconnSock + 1;
    FD_ZERO(&basemask);
    FD_SET(rsComm->reconnSock, &basemask);

    while (1) {
        while ((nSelected = select (nSockets, &basemask,
          (fd_set *) NULL, (fd_set *) NULL, NULL)) < 0) {
            if (errno == EINTR) {
                rodsLog (LOG_NOTICE, "reconnManager: select interrupted\n");
                continue;
            } else {
                rodsLog (LOG_ERROR, "reconnManager: select failed, errno = %d",
                  errno);
                #ifdef USE_BOOST
		        boost::unique_lock< boost::mutex > boost_lock( *rsComm->lock );
                #else
	            pthread_mutex_lock (&rsComm->lock); // JMC :: FIXME
                #endif
                close (rsComm->reconnSock);
                rsComm->reconnSock = 0;
                #ifdef USE_BOOST
		        boost_lock.unlock();
                #else
	            pthread_mutex_unlock (&rsComm->lock); // JMC :: FIXME
                #endif	
                return;
            
            } // else

        } // while select

        /* don't lock it yet until we are done with establishing a connection */
        len = sizeof (remoteAddr);
        bzero (&remoteAddr, sizeof (remoteAddr));
            newSock = accept (rsComm->reconnSock, (struct sockaddr *) &remoteAddr, 
          &len);
        if (newSock < 0) {
            acceptFailCnt++;
            rodsLog (LOG_ERROR, 
              "reconnManager: accept for sock %d failed, errno = %d",
                  rsComm->reconnSock, errno);
            if (acceptFailCnt > MAX_RECON_ERROR_CNT) {
                rodsLog (LOG_ERROR, 
              "reconnManager: accept failed cnt > 10, reconnManager exit");
            close (rsComm->reconnSock);
            rsComm->reconnSock = -1;
            rsComm->reconnPort = 0;
            return;
            } else {
                continue;
            }
        }
        
        // =-=-=-=-=-=-=-
        // create a network object
        eirods::network_object_ptr net_obj;
        eirods::error ret = eirods::network_factory( rsComm, net_obj );
        if( !ret.ok() ) {
            eirods::log( PASS( ret ) );
            return; 
        }

        // =-=-=-=-=-=-=-
        // repave sock handle with new socket
        net_obj->socket_handle( newSock );
 
        ret = readReconMsg( net_obj, &reconnMsg );
        if( !ret.ok() ) {
            eirods::log( PASS( ret ) );
            close (newSock);
            continue;
        } else if (reconnMsg->cookie != rsComm->cookie) {
            rodsLog (LOG_ERROR,
            "reconnManager: cookie mistach, got = %d vs %d",
              reconnMsg->cookie, rsComm->cookie);
            close (newSock);
            free (reconnMsg);
            continue;
        } 

#ifdef USE_BOOST
        boost::unique_lock<boost::mutex> boost_lock( *rsComm->lock );
#else
        pthread_mutex_lock (&rsComm->lock);
#endif
        rsComm->clientState = reconnMsg->procState;
        rsComm->reconnectedSock = newSock;
        /* need to check agentState */
        while (rsComm->agentState == SENDING_STATE) {
            /* have to wait until the agent stop sending */
            rsComm->reconnThrState = CONN_WAIT_STATE;
#ifdef USE_BOOST
            rsComm->cond->wait( boost_lock );
#else
            pthread_cond_wait (&rsComm->cond, &rsComm->lock);
#endif
        }
       
        rsComm->reconnThrState = PROCESSING_STATE;
        bzero (reconnMsg, sizeof (procState_t));
        reconnMsg->procState = rsComm->agentState;
        ret = sendReconnMsg ( net_obj, reconnMsg);
        free (reconnMsg);
        if( !ret.ok() ) {
            eirods::log( PASS( ret ) );          
            close (newSock);
            rsComm->reconnectedSock = 0;
#ifdef USE_BOOST
            boost_lock.unlock();
#else
            pthread_mutex_unlock (&rsComm->lock);
#endif
                continue;
            }
        if (rsComm->agentState == PROCESSING_STATE) {
                rodsLog (LOG_NOTICE,
                  "reconnManager: svrSwitchConnect. cliState = %d,agState=%d",
                  rsComm->clientState, rsComm->agentState);
            svrSwitchConnect (rsComm);
        }
#ifdef USE_BOOST
        boost_lock.unlock();
#else
        pthread_mutex_unlock (&rsComm->lock);
#endif
    } // while 1
}

int
svrChkReconnAtSendStart (rsComm_t *rsComm)
{
    if (rsComm->reconnSock > 0) {
        /* handle reconn */
#ifdef USE_BOOST
	boost::unique_lock<boost::mutex> boost_lock( *rsComm->lock );
#else
        pthread_mutex_lock (&rsComm->lock);
#endif
        if (rsComm->reconnThrState == CONN_WAIT_STATE) {
            /* should not be here */
            rodsLog (LOG_NOTICE,
              "svrChkReconnAtSendStart: ThrState = CONN_WAIT_STATE, agentState=%d",
              rsComm->agentState);
            rsComm->agentState = PROCESSING_STATE;
#ifdef USE_BOOST
	    rsComm->cond->notify_all();
#else
            pthread_cond_signal (&rsComm->cond);
#endif
        }
	svrSwitchConnect (rsComm);
        rsComm->agentState = SENDING_STATE;
#ifdef USE_BOOST
	boost_lock.unlock();
#else
        pthread_mutex_unlock (&rsComm->lock);
#endif
    }
    return 0;
}

int
svrChkReconnAtSendEnd (rsComm_t *rsComm)
{
    if (rsComm->reconnSock > 0) {
        /* handle reconn */
#ifdef USE_BOOST
	boost::unique_lock<boost::mutex> boost_lock( *rsComm->lock );
#else
        pthread_mutex_lock (&rsComm->lock);
#endif
        rsComm->agentState = PROCESSING_STATE;
        if (rsComm->reconnThrState == CONN_WAIT_STATE) {

#ifdef USE_BOOST
	    rsComm->cond->wait( boost_lock );
#else
            pthread_cond_signal (&rsComm->cond);
#endif
        }
#ifdef USE_BOOST
	boost_lock.unlock();
#else
        pthread_mutex_unlock (&rsComm->lock);
#endif
    }
    return 0;
}

int
svrChkReconnAtReadStart (rsComm_t *rsComm)
{
    if (rsComm->reconnSock > 0) {
        /* handle reconn */
#ifdef USE_BOOST
	boost::unique_lock< boost::mutex > boost_lock( *rsComm->lock );
#else
        pthread_mutex_lock (&rsComm->lock);
#endif
        if (rsComm->reconnThrState == CONN_WAIT_STATE) {
            /* should not be here */
            rodsLog (LOG_NOTICE,
              "svrChkReconnAtReadStart: ThrState = CONN_WAIT_STATE, agentState=%d",
              rsComm->agentState);
            rsComm->agentState = PROCESSING_STATE;
#ifdef USE_BOOST
	    rsComm->cond->wait( boost_lock );
#else
            pthread_cond_signal (&rsComm->cond);
#endif
        }
	svrSwitchConnect (rsComm);
        rsComm->agentState = RECEIVING_STATE;
#ifdef USE_BOOST
	boost_lock.unlock();
#else
        pthread_mutex_unlock (&rsComm->lock);
#endif
    }
    return 0;
}

int
svrChkReconnAtReadEnd (rsComm_t *rsComm)
{
    if (rsComm->reconnSock > 0) {
        /* handle reconn */
#ifdef USE_BOOST
        boost::unique_lock< boost::mutex > boost_lock( *rsComm->lock );
        rsComm->agentState = PROCESSING_STATE;
        if (rsComm->reconnThrState == CONN_WAIT_STATE) {
            rsComm->cond->notify_all();
        }
	boost_lock.unlock();
#else
        pthread_mutex_lock (&rsComm->lock);
        rsComm->agentState = PROCESSING_STATE;
        if (rsComm->reconnThrState == CONN_WAIT_STATE) {
            pthread_cond_signal (&rsComm->cond);
        }
        pthread_mutex_unlock (&rsComm->lock);
#endif
    }
    return 0;
}

#endif

int
svrSockOpenForInConn (rsComm_t *rsComm, int *portNum, char **addr, int proto)
{
    int status;

    status = sockOpenForInConn (rsComm, portNum, addr, proto);
    if (status < 0) return status;

    if (addr != NULL && *addr != NULL &&
     (strcmp (*addr, "127.0.0.1") == 0 || strcmp (*addr, "0.0.0.0") == 0 ||
      strcmp (*addr, "localhost") == 0)) { 
	/* localhost */
	char *myaddr;

	myaddr = getLocalSvrAddr ();
	if (myaddr != NULL) {
	    free (*addr);
	    *addr = strdup (myaddr);
	} else {
            rodsLog (LOG_NOTICE,
              "svrSockOpenForInConn: problem resolving local host addr %s",
              *addr);
	}
    }
    return status;
}

char *
getLocalSvrAddr ()
{
    char *myHost;
    myHost = _getSvrAddr (LocalServerHost);
    return myHost;
}

char *
_getSvrAddr (rodsServerHost_t *rodsServerHost)
{
    hostName_t *tmpHostName;

    if (rodsServerHost == NULL) return NULL;

    tmpHostName = rodsServerHost->hostName;
    while (tmpHostName != NULL) {
        if (strcmp (tmpHostName->name, "localhost") != 0 &&
          strcmp (tmpHostName->name, "127.0.0.1") != 0 &&
          strcmp (tmpHostName->name, "0.0.0.0") != 0 &&
          strchr (tmpHostName->name, '.') != NULL) {
            return (tmpHostName->name);
        }
        tmpHostName = tmpHostName->next;
    }
    return (NULL);
}

char *
getSvrAddr (rodsServerHost_t *rodsServerHost)
{
    char *myHost;

    myHost = _getSvrAddr (rodsServerHost);
    if (myHost == NULL) {
        /* use the first one */
	myHost = rodsServerHost->hostName->name;
    }
    return myHost;
}

int
setLocalSrvAddr (char *outLocalAddr)
{
    char *myHost;

    if (outLocalAddr == NULL) return USER__NULL_INPUT_ERR;

    myHost = getSvrAddr (LocalServerHost);

    if (myHost != NULL) {
        rstrcpy (outLocalAddr, myHost, NAME_LEN);
	return 0;
    } else {
        return SYS_INVALID_SERVER_HOST;
    }
}

int
forkAndExec (char *av[])
{
    int childPid = 0;
    int status = -1;
    int childStatus = 0;


#ifndef windows_platform   /* UNIX */
    childPid = RODS_FORK ();

    if (childPid == 0) {
        /* child */
        execv(av[0], av);
        /* gets here. must be bad */
        exit(1);
    } else if (childPid < 0) {
        rodsLog (LOG_ERROR,
         "exectar: RODS_FORK failed. errno = %d", errno);
        return (SYS_FORK_ERROR);
    }

    /* parent */

    status = waitpid (childPid, &childStatus, 0);
    if (status >= 0 && childStatus != 0) {
        rodsLog (LOG_ERROR,
         "forkAndExec: waitpid status = %d, childStatus = %d",
          status, childStatus);
        status = EXEC_CMD_ERROR;
    }
#else
    rodsLog (LOG_ERROR,
         "forkAndExec: fork and exec not supported");

    status = SYS_NOT_SUPPORTED;
#endif
    return status;
}

int
singleRemLocCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp)
{
    dataOprInp_t *dataOprInp;
    int status = 0;
    int oprType;

    if (dataCopyInp == NULL) {
        rodsLog (LOG_NOTICE,
          "remLocCopy: NULL dataCopyInp input");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    dataOprInp = &dataCopyInp->dataOprInp;
    oprType = dataOprInp->oprType;

    if (oprType == COPY_TO_LOCAL_OPR) {
        status = singleRemToLocCopy (rsComm, dataCopyInp);
    } else {
        status = singleLocToRemCopy (rsComm, dataCopyInp);
    }
    return status;
}

int
singleRemToLocCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp)
{
    dataOprInp_t *dataOprInp;
    rodsLong_t dataSize;
    int l1descInx;
    int destL3descInx;
    int destRescTypeInx;
    bytesBuf_t dataObjReadInpBBuf;
    openedDataObjInp_t dataObjReadInp;
    int bytesWritten, bytesRead;
    rodsLong_t totalWritten = 0;

    /* a GET type operation */
    if (dataCopyInp == NULL) {
        rodsLog (LOG_NOTICE,
          "singleRemToLocCopy: NULL dataCopyInp input");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    dataOprInp = &dataCopyInp->dataOprInp;
    l1descInx = dataCopyInp->portalOprOut.l1descInx;
    destL3descInx = dataOprInp->destL3descInx;
    destRescTypeInx = dataOprInp->destRescTypeInx;
    dataSize = dataOprInp->dataSize;

    bzero (&dataObjReadInp, sizeof (dataObjReadInp));
    dataObjReadInpBBuf.buf = malloc (TRANS_BUF_SZ);
    dataObjReadInpBBuf.len = dataObjReadInp.len = TRANS_BUF_SZ;
    dataObjReadInp.l1descInx = l1descInx;
    while ((bytesRead = rsDataObjRead (rsComm, &dataObjReadInp,
      &dataObjReadInpBBuf)) > 0) {
        bytesWritten = _l3Write (rsComm, destRescTypeInx,
          destL3descInx, dataObjReadInpBBuf.buf, bytesRead);

        if (bytesWritten != bytesRead) {
           rodsLog (LOG_ERROR,
            "singleRemToLocCopy: Read %d bytes, Wrote %d bytes.\n ",
            bytesRead, bytesWritten);
            free (dataObjReadInpBBuf.buf);
	    return (SYS_COPY_LEN_ERR);
	} else {
	    totalWritten += bytesWritten;
	}
    }
    free (dataObjReadInpBBuf.buf);
    if (dataSize <= 0 || totalWritten == dataSize ||
      getValByKey (&dataOprInp->condInput, NO_CHK_COPY_LEN_KW) != NULL) {
        return (0);
    } else {
        rodsLog (LOG_ERROR,
          "singleRemToLocCopy: totalWritten %lld dataSize %lld mismatch",
          totalWritten, dataSize);
        return (SYS_COPY_LEN_ERR);
    }
}

int
singleLocToRemCopy (rsComm_t *rsComm, dataCopyInp_t *dataCopyInp)
{
    dataOprInp_t *dataOprInp;
    rodsLong_t dataSize;
    int l1descInx;
    int srcL3descInx;
    int srcRescTypeInx;
    bytesBuf_t dataObjWriteInpBBuf;
    openedDataObjInp_t dataObjWriteInp;
    int bytesWritten, bytesRead;
    rodsLong_t totalWritten = 0;

    /* a PUT type operation */
    if (dataCopyInp == NULL) {
        rodsLog (LOG_NOTICE,
          "singleRemToLocCopy: NULL dataCopyInp input");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
    dataOprInp = &dataCopyInp->dataOprInp;
    l1descInx = dataCopyInp->portalOprOut.l1descInx;
    srcL3descInx = dataOprInp->srcL3descInx;
    srcRescTypeInx = dataOprInp->srcRescTypeInx;
    dataSize = dataOprInp->dataSize;

    bzero (&dataObjWriteInp, sizeof (dataObjWriteInp));
    dataObjWriteInpBBuf.buf = malloc (TRANS_BUF_SZ);
    dataObjWriteInpBBuf.len = 0;
    dataObjWriteInp.l1descInx = l1descInx;

    while ((bytesRead = _l3Read (rsComm, srcRescTypeInx,
      srcL3descInx, dataObjWriteInpBBuf.buf, TRANS_BUF_SZ)) > 0) {
        dataObjWriteInp.len =  dataObjWriteInpBBuf.len = bytesRead;
        bytesWritten = rsDataObjWrite (rsComm, &dataObjWriteInp,
          &dataObjWriteInpBBuf);
        if (bytesWritten != bytesRead) {
           rodsLog (LOG_ERROR,
            "singleLocToRemCopy: Read %d bytes, Wrote %d bytes.\n ",
            bytesRead, bytesWritten);
            free (dataObjWriteInpBBuf.buf);
	    return (SYS_COPY_LEN_ERR);
	} else {
	    totalWritten += bytesWritten;
	}
    }
    free (dataObjWriteInpBBuf.buf);
    if (dataSize <= 0 || totalWritten == dataSize || 
      getValByKey (&dataOprInp->condInput, NO_CHK_COPY_LEN_KW) != NULL) {
        return (0);
    } else {
        rodsLog (LOG_ERROR,
          "singleLocToRemCopy: totalWritten %lld dataSize %lld mismatch",
          totalWritten, dataSize);
        return (SYS_COPY_LEN_ERR);
    }
}

/* readStartupPack - Read the startup packet from client.
 * Note: initServerInfo must be called first because it calls getLocalZoneInfo.
 */

int
readStartupPack(
    eirods::network_object_ptr _ptr, 
    startupPack_t**     startupPack, 
    struct timeval*     tv ) {
    int status;
    msgHeader_t myHeader;
    bytesBuf_t inputStructBBuf, bsBBuf, errorBBuf;
    eirods::error ret = readMsgHeader( _ptr, &myHeader, tv );
   if( !ret.ok() ) {
        eirods::log( PASS( ret ) );  
        return ret.code();
    }

    if (myHeader.msgLen > (int) sizeof (startupPack_t) * 2 || 
      myHeader.msgLen <= 0) {
        rodsLog (LOG_NOTICE,
          "readStartupPack: problem with myHeader.msgLen = %d",
          myHeader.msgLen);
          return (SYS_HEADER_READ_LEN_ERR);
    }

    memset (&bsBBuf, 0, sizeof (bytesBuf_t));
    ret = readMsgBody( 
              _ptr, 
              &myHeader, 
              &inputStructBBuf, 
              &bsBBuf,
              &errorBBuf, 
              XML_PROT, 
              tv );
    if( !ret.ok() ) {
        eirods::log( PASS( ret ) ); 
        return ret.code();
    }

    /* some sanity check */

    if (strcmp (myHeader.type, RODS_CONNECT_T) != 0) {
        if (inputStructBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        if (bsBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        if (errorBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        rodsLog (LOG_NOTICE,
          "readStartupPack: wrong mag type - %s, expect %s",
          myHeader.type, RODS_CONNECT_T);
          return (SYS_HEADER_TYPE_LEN_ERR);
    }

    if (myHeader.bsLen != 0) {
        if (bsBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        rodsLog (LOG_NOTICE, "readStartupPack: myHeader.bsLen = %d is not 0",
          myHeader.bsLen);
    }

    if (myHeader.errorLen != 0) {
        if (errorBBuf.buf != NULL)
            free (inputStructBBuf.buf);
        rodsLog (LOG_NOTICE,
         "readStartupPack: myHeader.errorLen = %d is not 0",
          myHeader.errorLen);
    }

    /* always use XML_PROT for the startup pack */
    status = unpackStruct (inputStructBBuf.buf, (void **) startupPack,
      "StartupPack_PI", RodsPackTable, XML_PROT);

    clearBBuf (&inputStructBBuf);

    if (status >= 0) {
	if ((*startupPack)->clientUser[0] != '\0'  && 
	  (*startupPack)->clientRodsZone[0] == '\0') {
	    char *zoneName;
	    /* clientRodsZone is not defined */
	    if ((zoneName = getLocalZoneName ()) != NULL) {
                rstrcpy ((*startupPack)->clientRodsZone, zoneName, NAME_LEN);
            }
	}
        if ((*startupPack)->proxyUser[0] != '\0'  && 
          (*startupPack)->proxyRodsZone[0] == '\0') {
	    char *zoneName;
            /* proxyRodsZone is not defined */
            if ((zoneName = getLocalZoneName ()) != NULL) {
                rstrcpy ((*startupPack)->proxyRodsZone, zoneName, NAME_LEN);
            }
        }
    } else {
        rodsLogError (LOG_NOTICE,  status,
         "readStartupPack:unpackStruct error. status = %d",
         status);
    } 
	
    return (status);
}


#ifdef RUN_SERVER_AS_ROOT

/* initServiceUser - set the username/uid of the unix user to
 *      run the iRODS daemons as if configured using the 
 *      irodsServiceUser environment variable. Will also
 *      set effective uid to service user.
 */
int
initServiceUser() 
{
#ifndef windows_platform
    char *serviceUser;
    struct passwd *pwent;
    
    serviceUser = getenv("irodsServiceUser");
    if (serviceUser == NULL || getuid() != 0) {
        /* either the option is not set, or not running     */
        /* with the necessary root permission. Just return. */
        return (0);
    }
    
    /* clear errno before getpwnam to distinguish an error from user */
    /* not existing. pwent == NULL && errno == 0 means no entry      */
    errno = 0;
    pwent = getpwnam(serviceUser);
    if (pwent) {
        ServiceUid = pwent->pw_uid;
        return (changeToServiceUser());
    }

    if (errno) {
        rodsLogError(LOG_ERROR, SYS_USER_RETRIEVE_ERR,
                     "setServiceUser: error in getpwnam %s, errno = %d",
                     serviceUser, errno);
        return (SYS_USER_RETRIEVE_ERR - errno);
    }
    else {
        rodsLogError(LOG_ERROR, SYS_USER_RETRIEVE_ERR,
                     "setServiceUser: user %s doesn't exist", serviceUser);
        return (SYS_USER_RETRIEVE_ERR);
    }
#else
    return (0);
#endif
}

/* isServiceUserSet - check if the service user has been configured
 */
int
isServiceUserSet()
{
    if (ServiceUid) {
        return 1;
    }
    else {
        return 0;
    }
}

/* changeToRootUser - take on root privilege by setting the process
 *                    effective uid to zero.
 */
int
changeToRootUser()
{
    int prev_errno, my_errno;

    if (!isServiceUserSet()) {
        /* not configured ... just return */
        return (0);
    }

#ifndef windows_platform
    /* preserve the errno from before. We'll often be   */
    /* called after a "permission denied" type error,   */
    /* so we need to preserve this previous error state */
    prev_errno = errno;
    if (seteuid(0) == -1) {
        my_errno = errno;
        errno = prev_errno;
        rodsLogError(LOG_ERROR, SYS_USER_NO_PERMISSION - my_errno, 
                     "changeToRootUser: can't change to root user id");
        return (SYS_USER_NO_PERMISSION - my_errno);
    }
#endif

    return (0);
}

/* changeToServiceUser - set the process effective uid to that of the
 *                       configured service user. Normally used to give
 *                       up root permission.
 */
int
changeToServiceUser()
{
    int prev_errno, my_errno;

    if (!isServiceUserSet()) {
        /* not configured ... just return */
        return (0);
    }

#ifndef windows_platform
    prev_errno = errno;
    if (seteuid(ServiceUid) == -1) {
        my_errno = errno;
        errno = prev_errno;
        rodsLogError(LOG_ERROR, SYS_USER_NO_PERMISSION - my_errno, 
                     "changeToServiceUser: can't change to service user id");
        return (SYS_USER_NO_PERMISSION - my_errno);
    }
#endif

    return (0);
}

#endif /* RUN_SERVER_AS_ROOT */


