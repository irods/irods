/******************************************************************************
 * QUANTA - A toolkit for High Performance Data Sharing
 * Copyright (C) 2003 Electronic Visualization Laboratory,
 * University of Illinois at Chicago
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either Version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Direct questions, comments etc about Quanta to cavern@evl.uic.edu
 *****************************************************************************/

#include "QUANTAnet_rbudpBase_c.hpp"
#include "rodsLog.hpp"
#include <stdarg.h>

// inline void TRACE_DEBUG( char *format, ...)
void TRACE_DEBUG( char *format, ... ) {
    va_list arglist;
    va_start( arglist, format );
    vfprintf( stderr, format, arglist );
    fprintf( stderr, "\n" );
    va_end( arglist );
}

void QUANTAnet_rbudpBase_c( rbudpBase_t *rbudpBase ) {
    rbudpBase->udpSockBufSize = UDPSOCKBUF;
    rbudpBase->verbose = 2;     // chatty by default
}

void setUDPBufSize( rbudpBase_t *rbudpBase, int nbytes ) {
    rbudpBase->udpSockBufSize =
        nbytes > 0 ? rbudpBase->udpSockBufSize : UDPSOCKBUF;
}

void setverbose( rbudpBase_t *rbudpBase, int v ) {
    rbudpBase->verbose = v;
}

void checkbuf( int udpSockfd, int sockbufsize, int verbose ) {
    int oldsend = -1, oldrecv = -1;
    int newsend = -1, newrecv = -1;
    socklen_t olen = sizeof( int );

    if ( getsockopt( udpSockfd, SOL_SOCKET, SO_SNDBUF, &oldsend, &olen ) < 0 ) {
        perror( "getsockopt: SO_SNDBUF" );
    }
    if ( getsockopt( udpSockfd, SOL_SOCKET, SO_RCVBUF, &oldrecv, &olen ) < 0 ) {
        perror( "getsockopt: SO_RCVBUF" );
    }
    if ( sockbufsize > 0 ) {
        if ( setsockopt( udpSockfd, SOL_SOCKET, SO_SNDBUF, &sockbufsize, sizeof( sockbufsize ) ) < 0 ) {
            perror( "setsockopt: SO_SNDBUF" );
        }
        if ( setsockopt( udpSockfd, SOL_SOCKET, SO_RCVBUF, &sockbufsize, sizeof( sockbufsize ) ) < 0 ) {
            perror( "setsockopt: SO_RCVBUF" );
        }
        if ( getsockopt( udpSockfd, SOL_SOCKET, SO_SNDBUF, &newsend, &olen ) < 0 ) {
            perror( "getsockopt: SO_SNDBUF" );
        }
        if ( getsockopt( udpSockfd, SOL_SOCKET, SO_RCVBUF, &newrecv, &olen ) < 0 ) {
            perror( "getsockopt: SO_RCVBUF" );
        }
    }
    if ( verbose ) fprintf( stderr, "UDP sockbufsize was %d/%d now %d/%d (send/recv)\n",
                                oldsend, oldrecv, newsend, newrecv );
}

int passiveUDP( rbudpBase_t *rbudpBase, char *host ) {
    struct sockaddr_in cliaddr;
    struct hostent *phe;

    // Create a UDP sink
    if ( ( rbudpBase->udpSockfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
        perror( "socket error" );
        return errno ? ( -1 * errno ) : -1;
    }

    bzero( ( char * )&rbudpBase->udpServerAddr,
           sizeof( rbudpBase->udpServerAddr ) );
    rbudpBase->udpServerAddr.sin_family = AF_INET;
    rbudpBase->udpServerAddr.sin_addr.s_addr = htonl( INADDR_ANY );
    rbudpBase->udpServerAddr.sin_port = htons( rbudpBase->udpLocalPort );

    if ( ( bind( rbudpBase->udpSockfd,
                 ( struct sockaddr * )&rbudpBase->udpServerAddr,
                 sizeof( rbudpBase->udpServerAddr ) ) ) < 0 ) {
        perror( "UDP bind error" );
        return errno ? ( -1 * errno ) : -1;
    }

    // Use connected UDP to receive only from a specific host and port.
    bzero( &cliaddr, sizeof( cliaddr ) );
    if ( ( phe = gethostbyname( host ) ) ) {
        memcpy( &cliaddr.sin_addr, phe->h_addr, phe->h_length );
    }
    else if ( ( int )( cliaddr.sin_addr.s_addr = inet_addr( host ) ) == -1 ) {
        perror( "can't get host entry" );
        return errno ? ( -1 * errno ) : -1;
    }

    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons( rbudpBase->udpRemotePort );
    if ( connect( rbudpBase->udpSockfd, ( struct sockaddr * ) &cliaddr,
                  sizeof( cliaddr ) ) < 0 ) {
        perror( "connect() error" );
        return errno ? ( -1 * errno ) : -1;
    }

    checkbuf( rbudpBase->udpSockfd, rbudpBase->udpSockBufSize,
              rbudpBase->verbose );
    return 0;
}

int connectTCP( rbudpBase_t *rbudpBase, char * host ) {
    static struct sockaddr_in tcpServerAddr;
    int retval = 0;
    struct hostent *phe;
    struct timeval start, now;

    /*Create a TCP connection */
    bzero( ( char * )&tcpServerAddr, sizeof( tcpServerAddr ) );
    tcpServerAddr.sin_family = AF_INET;
    if ( ( phe = gethostbyname( host ) ) ) {
        memcpy( &tcpServerAddr.sin_addr, phe->h_addr, phe->h_length );
    }
    else if ( ( int )( tcpServerAddr.sin_addr.s_addr = inet_addr( host ) ) == -1 ) {
        perror( "can't get host entry" );
        return errno ? ( -1 * errno ) : -1;
    }
    tcpServerAddr.sin_port = htons( rbudpBase->tcpPort );

    if ( ( rbudpBase->tcpSockfd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
        perror( "socket error" );
        return errno ? ( -1 * errno ) : -1;
    }

    printf( "try to conn.\n" );
    gettimeofday( &start, NULL );
    do {
        retval = connect( rbudpBase->tcpSockfd, ( struct sockaddr * )&tcpServerAddr, sizeof( tcpServerAddr ) );
        gettimeofday( &now, NULL );
    }
    while ( ( retval < 0 ) && ( USEC( &start, &now ) < 5000000 ) );
    return retval;
}

int connectUDP( rbudpBase_t *rbudpBase, char *host ) {
    static struct sockaddr_in udpClientAddr;
    struct hostent *phe;

    // Fill in the structure whith the address of the server that we want to send to
    // udpServerAddr is class global variable, will be used to send data
    bzero( &rbudpBase->udpServerAddr, sizeof( rbudpBase->udpServerAddr ) );
    rbudpBase->udpServerAddr.sin_family = AF_INET;
    if ( ( phe = gethostbyname( host ) ) ) {
        memcpy( &rbudpBase->udpServerAddr.sin_addr, phe->h_addr, phe->h_length );
    }
    else if ( ( int )( rbudpBase->udpServerAddr.sin_addr.s_addr = inet_addr( host ) ) == -1 ) {
        perror( "can't get host entry" );
        return errno ? ( -1 * errno ) : -1;
    }
    rbudpBase->udpServerAddr.sin_port = htons( rbudpBase->udpRemotePort );

    /* Open a UDP socket */
    if ( ( rbudpBase->udpSockfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
        perror( "socket error" );
        return errno ? ( -1 * errno ) : -1;
    }

    // Allow the port to be reused.
    int yes = 1;
    if ( setsockopt( rbudpBase->udpSockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( int ) ) == -1 ) {
        perror( "setsockopt" );
        return errno ? ( -1 * errno ) : -1;
    }

    /* Bind any local address for us */
    bzero( &udpClientAddr, sizeof( udpClientAddr ) );
    udpClientAddr.sin_family = AF_INET;
    udpClientAddr.sin_addr.s_addr = htonl( INADDR_ANY );
    udpClientAddr.sin_port = htons( rbudpBase->udpLocalPort );
    if ( ( bind( rbudpBase->udpSockfd, ( struct sockaddr * )&udpClientAddr,
                 sizeof( udpClientAddr ) ) ) < 0 ) {
        perror( "UDP client bind error" );
        return errno ? ( -1 * errno ) : -1;
    }

    checkbuf( rbudpBase->udpSockfd, rbudpBase->udpSockBufSize,
              rbudpBase->verbose );
    return 0;
}

void initTCPServer( rbudpBase_t *rbudpBase ) {
    struct sockaddr_in tcpServerAddr;

    // Create TCP connection as a server in order to transmit the error list
    // Open a TCP socket
    if ( ( rbudpBase->listenfd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
        perror( "socket error" );
        exit( 1 );
    }

    int yes = 1;
    if ( setsockopt( rbudpBase->listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( int ) ) == -1 ) {
        perror( "setsockopt" );
        return;
    }

    /* Bind our local address so that the client can send to us */
    bzero( &tcpServerAddr, sizeof( tcpServerAddr ) );
    tcpServerAddr.sin_family = AF_INET;
    tcpServerAddr.sin_addr.s_addr = htonl( INADDR_ANY );
    tcpServerAddr.sin_port = htons( rbudpBase->tcpPort );

    if ( ( bind( rbudpBase->listenfd, ( struct sockaddr * )&tcpServerAddr, sizeof( tcpServerAddr ) ) ) < 0 ) {
        perror( "bind error" );
        exit( 1 );
    }

    if ( ( listen( rbudpBase->listenfd, 5 ) ) < 0 ) {
        perror( "listen error" );
        return;
    }
}

void listenTCPServer( rbudpBase_t *rbudpBase ) {
    struct sockaddr_in tcpClientAddr;
    socklen_t clilen;

    clilen = sizeof( tcpClientAddr );
    rbudpBase->tcpSockfd = accept( rbudpBase->listenfd, ( struct sockaddr * ) &tcpClientAddr, &clilen );

    if ( rbudpBase->tcpSockfd < 0 ) {
        perror( "accept error" );
        return;
    }
}

int readn( register int fd, register char *ptr, register int nbytes ) {
    int nleft, nread;
    nleft = nbytes;
    while ( nleft > 0 ) {
        nread = read( fd, ptr, nleft );
        if ( nread < 0 ) {
            return ( nread );   /*error */
        }
        else if ( nread == 0 ) {
            break;    /* EOF */
        }

        nleft -= nread;
        ptr += nread;
    }
    return nbytes - nleft;
}

int writen( register int fd, register char *ptr, register int nbytes ) {
    int nleft, nwritten;

    nleft = nbytes;
    while ( nleft > 0 ) {
        nwritten = write( fd, ptr, nleft );
        if ( nwritten <= 0 ) {
            return ( nwritten );    /* error */
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return nbytes - nleft;
}

int reportTime( struct timeval *start ) {
    struct timeval end;
    int usecs;
    gettimeofday( &end, NULL );
    usecs = 1000000 * ( end.tv_sec - start->tv_sec ) + ( end.tv_usec - start->tv_usec );
    start->tv_sec = end.tv_sec;
    start->tv_usec = end.tv_usec;
    return usecs;
}



void initErrorBitmap( rbudpBase_t *rbudpBase ) {
    int i;
    // the first byte is 0 if there is error.  1 if all done.
    int startOfLastByte = rbudpBase->totalNumberOfPackets - ( rbudpBase->sizeofErrorBitmap - 2 ) * 8;
    unsigned char bits[8] = {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080};

    /* The first byte is for judging all_done */
    for ( i = 0; i < rbudpBase->sizeofErrorBitmap; i++ ) {
        rbudpBase->errorBitmap[i] = 0;
    }
    /* Preset those bits unused */
    for ( i = startOfLastByte; i < 8; i++ ) {
        rbudpBase->errorBitmap[rbudpBase->sizeofErrorBitmap - 1] |= bits[i];
    }

    /* Hack: we're not sure whether the peer is the same
     * endian-ness we are, and the RBUDP protocol doesn't specify.
     * Let's assume that it's like us, but keep a flag
     * to set if we see unreasonable-looking sequence numbers.
     */
    rbudpBase->peerswap = RB_FALSE;
}

int ptohseq( rbudpBase_t *rbudpBase, int origseq ) {
    int seq = origseq;

    if ( rbudpBase->peerswap ) {
        seq = swab32( origseq );
    }

    if ( seq < 0 || ( seq >> 3 ) >= rbudpBase->sizeofErrorBitmap - 1 ) {
        if ( !rbudpBase->peerswap ) {
            rbudpBase->peerswap = RB_TRUE;
            if ( rbudpBase->verbose ) {
                fprintf( stderr, "peer has different endian-ness from ours\n" );
            }
            return ptohseq( rbudpBase, seq );
        }
        else {
            fprintf( stderr, "Unreasonable RBUDP sequence number %d = %x\n",
                     origseq, origseq );
            return 0;
        }
    }
    return seq;
}


void updateErrorBitmap( rbudpBase_t *rbudpBase, long long seq ) {
    long long index_in_list, offset_in_index;
    unsigned char bits[8] = {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080};
    if ( rbudpBase->peerswap ) {
        seq = swab32( seq );
    }
    if ( seq < 0 || ( seq >> 3 ) >= rbudpBase->sizeofErrorBitmap - 1 ) {
        if ( !rbudpBase->peerswap ) {
            rbudpBase->peerswap = RB_TRUE;
            if ( rbudpBase->verbose ) {
                fprintf( stderr, "peer has opposite endian-ness to ours, swapping seqno bytes\n" );
            }
        }
        seq = swab32( seq );
    }
    if ( seq < 0 || ( seq >> 3 ) >= rbudpBase->sizeofErrorBitmap - 1 ) {
        fprintf( stderr, "sequence number 0x%llx out of range 0..%d\n",
                 seq, rbudpBase->sizeofErrorBitmap * 8 - 1 );
        return;
    }
    // seq starts with 0
    index_in_list = seq >> 3;
    index_in_list ++;
    offset_in_index = seq % 8;
    rbudpBase->errorBitmap[index_in_list] |= bits[offset_in_index];
}


// return the count of errors
// The first byte is reserved to indicate if any packet's missing
int updateHashTable( rbudpBase_t *rbudpBase ) {
    int count = 0;
    int i, j;
    unsigned char bits[8] = {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080};

    for ( i = 1; i < rbudpBase->sizeofErrorBitmap; i++ ) {
        for ( j = 0; j < 8; j++ ) {
            if ( ( rbudpBase->errorBitmap[i] & bits[j] ) == 0 ) {
                rbudpBase->hashTable[count] = ( i - 1 ) * 8 + j;
                count ++;
            }
        }
    }
// set the first byte to let the sender know "all done"
    if ( count == 0 ) {
        rbudpBase->errorBitmap[0] = 1;
    }
    return count;
}

// Utility functions

/* Unconditionally swap bytes in a 32-bit value */
int swab32( int val ) {
    return ( ( ( val >> 24 ) & 0xFF ) | ( ( val >> 8 ) & 0xFF00 )
             | ( ( val & 0xFF00 ) << 8 ) | ( ( val & 0xFF ) << 24 ) );
}

long long rb_htonll( long long lll ) {
    long long nll = 0;
    unsigned char *cp = ( unsigned char * )&nll;

    cp[0] = ( lll >> 56 ) & 0xFF;
    cp[1] = ( lll >> 48 ) & 0xFF;
    cp[2] = ( lll >> 40 ) & 0xFF;
    cp[3] = ( lll >> 32 ) & 0xFF;
    cp[4] = ( lll >> 24 ) & 0xFF;
    cp[5] = ( lll >> 16 ) & 0xFF;
    cp[6] = ( lll >>  8 ) & 0xFF;
    cp[7] = ( lll >>  0 ) & 0xFF;

    return nll;
}

long long rb_ntohll( long long nll ) {
    unsigned char *cp = ( unsigned char * )&nll;

    return ( ( long long )cp[0] << 56 ) |
           ( ( long long )cp[1] << 48 ) |
           ( ( long long )cp[2] << 40 ) |
           ( ( long long )cp[3] << 32 ) |
           ( ( long long )cp[4] << 24 ) |
           ( ( long long )cp[5] << 16 ) |
           ( ( long long )cp[6] <<  8 ) |
           ( ( long long )cp[7] <<  0 );
}

int
setUdpSockOpt( int udpSockfd ) {
    int sockbufsize = UDPSOCKBUF;
    int error_code;
    error_code = setsockopt( udpSockfd, SOL_SOCKET, SO_SNDBUF, &sockbufsize,
                             sizeof( sockbufsize ) );
    if ( error_code != 0 ) {
        rodsLog( LOG_ERROR, "setsockopt failed on the send buffer in setUdpSockOpt with %d", error_code );
    }
    error_code = setsockopt( udpSockfd, SOL_SOCKET, SO_RCVBUF, &sockbufsize,
                             sizeof( sockbufsize ) );
    if ( error_code != 0 ) {
        rodsLog( LOG_ERROR, "setsockopt failed on the receive buffer in setUdpSockOpt with %d", error_code );
    }
    return 0;
}

