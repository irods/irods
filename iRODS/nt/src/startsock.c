#include "startsock.h"

int
startWinsock()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err; 

    wVersionRequested = MAKEWORD( 2, 2 ); 

    err = WSAStartup( wVersionRequested, &wsaData );

    if ( err != 0 ) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */  
        (void) fprintf(stderr,
	     "FATAL: startWinsock: WSAStartup() failed: errno=%s\n", errno);
/*      return(STATUS_ERROR); */
        return -2001;   /* STATUS_ERROR = -2001 */
    }
 
    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */ 
    if ( LOBYTE( wsaData.wVersion ) != 2 ||
        HIBYTE( wsaData.wVersion ) != 2 ) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        (void) fprintf(stderr,
	     "FATAL: startWinsock: worng WinSock version (requested 2.2 \
              or later, obtained %d.%d)\n", HIBYTE( wsaData.wVersion ), 
              LOBYTE( wsaData.wVersion ) );
        WSACleanup( );
/*      return(STATUS_ERROR);  */
        return -2001;
    } 

    /* The WinSock DLL is acceptable. Proceed. */
    return 0;
}


int
cleanWinsock() 
{

    int err;

    err = WSACleanup();

    if ( err != 0 ) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */  
        (void) fprintf(stderr,
	     "FATAL: cleanWinsock: WSACleanup() failed: errno=%s\n", errno);
/*      return(STATUS_ERROR); */
        return -2001;
    }

    return 0;

}



SOCKET
duplicateWinsock(SOCKET oldSocket)
{
     WSAPROTOCOL_INFO ProtocolInfo;
     SOCKET duplicateSocket;

     // create duplicate socket descriptor, so when the master closes
     // the original descriptor the the socket won't disappear
     if (WSADuplicateSocket (oldSocket,
                             getpid(),
                             &ProtocolInfo) == SOCKET_ERROR) {
         fprintf(stderr, "duplicateWinsock: socket duplication failed, error=%d\n", WSAGetLastError());
         return(0);
     }

     if ( (duplicateSocket = WSASocket(FROM_PROTOCOL_INFO,
                                       FROM_PROTOCOL_INFO,
                                       FROM_PROTOCOL_INFO,                
                                       &ProtocolInfo,
                                       0,
                                       0)) == INVALID_SOCKET ){ 
         fprintf(stderr, "duplicateWinsock: duplicate socket creation failed, error=%d\n", WSAGetLastError());
         return(0);
     } 

     return (duplicateSocket);

}
