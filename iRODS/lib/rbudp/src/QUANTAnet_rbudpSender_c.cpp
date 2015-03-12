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

#include "QUANTAnet_rbudpSender_c.hpp"
#include "rodsErrorTable.hpp"
#include "rodsLog.hpp"

#include <stdarg.h>
#include <string>
#include <limits>

// If you want to output debug info on terminals when running, put
//  fprintf(stderr, __VA_ARGS__);
//  fprintf(stderr, "\n");
// into TRACE_DEBUG definition.
// If you want to log the output into a log file, uncomment DEBUG, and put
//  fprintf(log, __VA_ARGS__);
//  fprintf(log, "\n");
// into TRACE_DEBUG definition.

//#define DEBUG

/*#define TRACE_DEBUG(...) do { \
  fprintf(stderr, __VA_ARGS__);\
  fprintf(stderr, "\n");\
  }  while(0)
*/

void
QUANTAnet_rbudpSender_c( rbudpSender_t *rbudpSender, int port ) {
    memset( rbudpSender, 0, sizeof( rbudpBase_t ) );
    rbudpSender->rbudpBase.tcpPort = port;
    rbudpSender->rbudpBase.udpLocalPort = port;
    rbudpSender->rbudpBase.udpRemotePort = port + 1;
    rbudpSender->rbudpBase.hasTcpSock = 0;
}

void QUANTAnet_rbudpSender_reuse( rbudpSender_t *rbudpSender,
                                  int tcpsock, int port ) {
    rbudpSender->rbudpBase.hasTcpSock = 1;
    rbudpSender->rbudpBase.tcpSockfd = tcpsock;
    rbudpSender->rbudpBase.udpLocalPort = port;
    rbudpSender->rbudpBase.udpRemotePort = port;
}


int  sendBuf( rbudpSender_t *rbudpSender, void * buffer, int bufSize,
              int sendRate, int packetSize ) {
    int done = 0;
    int status = 0;
    struct timeval curTime, startTime;
    double srate;
    gettimeofday( &curTime, NULL );
    startTime = curTime;
    int lastRemainNumberOfPackets = 0;
    int noProgressCnt = 0;
    initSendRudp( rbudpSender, buffer, bufSize, sendRate, packetSize );
    while ( !done ) {
        // blast UDP packets
        if ( rbudpSender->rbudpBase.verbose > 1 ) {
            TRACE_DEBUG( "sending UDP packets" );
        }
        reportTime( &curTime );
        status = udpSend( rbudpSender );
        if ( status < 0 ) {
            return status;
        }

        srate = ( double ) rbudpSender->rbudpBase.remainNumberOfPackets *
                rbudpSender->rbudpBase.payloadSize * 8 /
                ( double ) reportTime( &curTime );
        if ( rbudpSender->rbudpBase.verbose > 1 ) {
            TRACE_DEBUG( "real sending rate in this send is %f", srate );
        }

        if ( lastRemainNumberOfPackets == 0 ) {
            lastRemainNumberOfPackets =
                rbudpSender->rbudpBase.remainNumberOfPackets;
        }
        // send end of UDP signal
        if ( rbudpSender->rbudpBase.verbose > 1 )
            TRACE_DEBUG( "send to socket %d an end signal.",
                         rbudpSender->rbudpBase.tcpSockfd );
        if ( rbudpSender->rbudpBase.verbose > 1 ) {
            fprintf( stderr, "write %d bytes.\n", ( int ) sizeof( rbudpSender->rbudpBase.endOfUdp ) );
        }
        int error_code = writen( rbudpSender->rbudpBase.tcpSockfd, ( char * )&rbudpSender->rbudpBase.endOfUdp,
                                 sizeof( rbudpSender->rbudpBase.endOfUdp ) );
        if ( error_code < 0 ) {
            rodsLog( LOG_ERROR, "writen failed in sendBuf with error code %d", error_code );
        }
        rbudpSender->rbudpBase.endOfUdp.round ++;

        reportTime( &curTime );
        gettimeofday( &curTime, NULL );
        if ( rbudpSender->rbudpBase.verbose > 1 ) {
            TRACE_DEBUG( "Current time: %d %ld", curTime.tv_sec, curTime.tv_usec );
        }

        // receive error list
        if ( rbudpSender->rbudpBase.verbose > 1 ) {
            TRACE_DEBUG( "waiting for error bitmap" );
        }

        int n = readn( rbudpSender->rbudpBase.tcpSockfd,
                       rbudpSender->rbudpBase.errorBitmap,
                       rbudpSender->rbudpBase.sizeofErrorBitmap );
        if ( n < 0 ) {
            perror( "read" );
            return errno ? ( -1 * errno ) : -1;
        }

        if ( ( unsigned char )rbudpSender->rbudpBase.errorBitmap[0] == 1 ) {
            done = 1;
            rbudpSender->rbudpBase.remainNumberOfPackets = 0;
            if ( rbudpSender->rbudpBase.verbose > 1 ) {
                TRACE_DEBUG( "done." );
            }
        }
        else {
            rbudpSender->rbudpBase.remainNumberOfPackets =
                updateHashTable( &rbudpSender->rbudpBase );
            if ( rbudpSender->rbudpBase.remainNumberOfPackets >=
                    lastRemainNumberOfPackets ) {
                noProgressCnt++;
                if ( noProgressCnt >= MAX_NO_PROGRESS_CNT ) {
                    return SYS_UDP_TRANSFER_ERR - errno;
                }
            }
            else {
                lastRemainNumberOfPackets =
                    rbudpSender->rbudpBase.remainNumberOfPackets;
                noProgressCnt = 0;
            }
        }

        if ( rbudpSender->rbudpBase.isFirstBlast ) {
            rbudpSender->rbudpBase.isFirstBlast = 0;
            double lossRate =
                ( double )rbudpSender->rbudpBase.remainNumberOfPackets /
                ( double )rbudpSender->rbudpBase.totalNumberOfPackets;
            //	if (rbudpSender->rbudpBase.remainNumberOfPackets > 0)
            //	    usecsPerPacket = (int) ((double)usecsPerPacket / (1.0 - lossRate - 0.05));
            if ( rbudpSender->rbudpBase.verbose > 0 ) {
                float dt = ( curTime.tv_sec - startTime.tv_sec )
                           + 1e-6 * ( curTime.tv_usec - startTime.tv_usec );
                float mbps = 1e-6 * 8 * bufSize / ( dt == 0 ? .01 : dt );
                TRACE_DEBUG( "loss rate: %f  on %dK in %.3f seconds (%.2f Mbits/s)",
                             lossRate, ( int )bufSize >> 10, dt, mbps );
                if ( rbudpSender->rbudpBase.verbose > 1 )
                    TRACE_DEBUG( "usecsPerPacket updated to %d",
                                 rbudpSender->rbudpBase.usecsPerPacket );
            }
        }
    }
    free( rbudpSender->rbudpBase.errorBitmap );
    free( rbudpSender->rbudpBase.hashTable );
    return 0;
}

/* XXXXX need to handle status */
int
udpSend( rbudpSender_t *rbudpSender ) {
    int i, done, actualPayloadSize;
    struct timeval start, now;
    char *msg = ( char * ) malloc( rbudpSender->rbudpBase.packetSize );
    int sendErrCnt = 0;

    done = 0;
    i = 0;
    gettimeofday( &start, NULL );
    while ( !done ) {
        gettimeofday( &now, NULL );
        if ( USEC( &start, &now ) < rbudpSender->rbudpBase.usecsPerPacket * i ) {
            // busy wait or sleep
            //		usleep(1);
        }
        else {
            // last packet is probably smaller than regular packets
            // we have to pad the last packet, make it the same length
            // as regular packets
            if ( rbudpSender->rbudpBase.hashTable[i] <
                    rbudpSender->rbudpBase.totalNumberOfPackets - 1 ) {
                actualPayloadSize =
                    rbudpSender->rbudpBase.payloadSize;
            }
            else {
                actualPayloadSize =
                    rbudpSender->rbudpBase.lastPayloadSize;
            }
            rbudpSender->sendHeader.seq =
                rbudpSender->rbudpBase.hashTable[i];
            bcopy( &rbudpSender->sendHeader, msg,
                   rbudpSender->rbudpBase.headerSize );
            bcopy( ( char* )( ( char* )rbudpSender->rbudpBase.mainBuffer +
                              ( rbudpSender->sendHeader.seq *
                                rbudpSender->rbudpBase.payloadSize ) ),
                   msg + rbudpSender->rbudpBase.headerSize,
                   actualPayloadSize );

            //TRACE_DEBUG("sent %d, packet size: %d",
            // sendHeader.seq, actualPayloadSize+headerSize);
            if ( rbudpSender->rbudpBase.udpServerAddr.sin_addr.s_addr			  == htonl( INADDR_ANY ) ) {
                if ( send( rbudpSender->rbudpBase.udpSockfd, msg,
                           actualPayloadSize +
                           rbudpSender->rbudpBase.headerSize, 0 ) < 0 ) {
                    perror( "send" );
                    sendErrCnt++;
                    if ( sendErrCnt > MAX_SEND_ERR_CNT ) {
                        return SYS_UDP_TRANSFER_ERR - errno;
                    }
                }
            }
            else {
                if ( sendto( rbudpSender->rbudpBase.udpSockfd, msg,
                             actualPayloadSize +
                             rbudpSender->rbudpBase.headerSize, 0,
                             ( const struct sockaddr * )
                             &rbudpSender->rbudpBase.udpServerAddr,
                             sizeof( rbudpSender->rbudpBase.udpServerAddr ) )
                        < 0 ) {
                    perror( "sendto" );
                    sendErrCnt++;
                    if ( sendErrCnt > MAX_SEND_ERR_CNT ) {
                        return SYS_UDP_TRANSFER_ERR - errno;
                    }
                }
            }
            i++;
            if ( i >= rbudpSender->rbudpBase.remainNumberOfPackets ) {
                done = 1;
            }
        }
    }
    free( msg );
    return 0;
}

int  sendstream( rbudpSender_t *rbudpSender, int fromfd, int sendRate,
                 int packetSize, int bufSize ) {
    int tcpSockfd = rbudpSender->rbudpBase.tcpSockfd;
    int verbose = rbudpSender->rbudpBase.verbose;

    // Receive startup message
    char ack[1];
    int n = readn( tcpSockfd, ack, 1 );
    if ( n < 0 ) {
        fprintf( stderr, "stream ack read error.\n" );
        return FAILED;
    }


    // allocate buffer
    char *buf = ( char * )malloc( bufSize );
    if ( buf == 0 ) {
        fprintf( stderr, " sendstream: Couldn't malloc %d bytes for buffer\n", bufSize );
        return FAILED;
    }

    long long bytesread;

    while ( ( bytesread = readn( fromfd, buf, bufSize ) ) > 0 ) {
        long long nread = rb_htonll( bytesread );
        if ( writen( tcpSockfd, ( char * )&nread, sizeof( nread ) ) != sizeof( nread ) ) {
            {
                fprintf( stderr, "tcp send failed.\n" );
                free( buf );
                return FAILED;
            }
        }
        if ( verbose > 1 ) {
            fprintf( stderr, "sending %lld bytes\n", bytesread );
        }
        sendBuf( rbudpSender, buf, bytesread, sendRate, packetSize );
    }
    close( fromfd );
    free( buf );
    return ( bytesread == 0 ) ? RB_SUCCESS /*clean EOF*/ : FAILED /*error*/;
}

int  rbSendfile( rbudpSender_t *rbudpSender, int sendRate, int packetSize,
                 char *fname ) {
    int tcpSockfd = rbudpSender->rbudpBase.tcpSockfd;
    int verbose = rbudpSender->rbudpBase.verbose;

    // Receive the getfile message
    std::string filename_string;
    if ( fname ) {
        filename_string = std::string( fname );
    }
    else {
        char fnameRead[SIZEOFFILENAME + 1];
        int n = readn( tcpSockfd, fnameRead, SIZEOFFILENAME );
        if ( n < 0 ) {
            fprintf( stderr, "read error.\n" );
            return FAILED;
        }
        fnameRead[n] = '\0';
        filename_string = std::string( fnameRead );
    }

    if ( verbose > 0 ) {
        fprintf( stderr, "Send file %s\n", filename_string.c_str() );
    }

    int fd = open( filename_string.c_str(), O_RDONLY );
    if ( fd < 0 ) {
        fprintf( stderr, "open file failed.\n" );
        return FAILED;
    }
    int status = sendfileByFd( rbudpSender, sendRate, packetSize, fd );
    close( fd );

    return status;
}

int  sendfileByFd( rbudpSender_t *rbudpSender, int sendRate, int packetSize,
                   int fd ) {
    int tcpSockfd = rbudpSender->rbudpBase.tcpSockfd;
    int verbose = rbudpSender->rbudpBase.verbose;
    int status = 0;
    long long remaining;
    long long offset = 0;

    struct stat filestat;
    if ( fstat( fd, &filestat ) < 0 ) {
        fprintf( stderr, "stat error.\n" );
        return errno ? ( -1 * errno ) : -1;
    }

    long long filesize = filestat.st_size;
    if ( verbose > 0 ) {
        fprintf( stderr, "The size of the file is %lld\n", filesize );
    }

    long long nfilesize = rb_htonll( filesize );
    if ( verbose > 0 ) {
        fprintf( stderr, "write %d bytess.\n", ( int ) sizeof( nfilesize ) );
    }

    // Send the file size to the receiver.
    if ( writen( tcpSockfd, ( char * )&nfilesize, sizeof( nfilesize ) ) != sizeof( nfilesize ) ) {
        {
            fprintf( stderr, "tcp send failed.\n" );
            return errno ? ( -1 * errno ) : -1;
        }
    }

    remaining = filesize;
    while ( remaining > 0 ) {
        uint toSend;

        if ( remaining > ( uint ) ONE_GIGA ) {
            toSend = ( uint ) ONE_GIGA;
        }
        else {
            toSend = remaining;
        }

        if ( verbose > 0 )
            TRACE_DEBUG( "Sending %d bytes chunk. %lld bytes remaining",
                         toSend, remaining - toSend );

        char *buf = ( char * )mmap( NULL, toSend, PROT_READ, MAP_SHARED, fd, offset );
        if ( buf == MAP_FAILED ) {
            fprintf( stderr, "mmap failed. toSend = %d, offset = %lld, errno = %d\n",
                     toSend, offset, errno );
            return errno ? ( -1 * errno ) : -1;
        }

        status = sendBuf( rbudpSender, buf, toSend, sendRate, packetSize );

        munmap( buf, toSend );
        if ( status < 0 ) {
            fprintf( stderr, "sendBuf error, status = %d\n", status );
            break;
        }
        remaining -= toSend;
        offset += toSend;
    }

    return status;
}

int  sendfilelist( rbudpSender_t *rbudpSender, int sendRate, int packetSize ) {
    int tcpSockfd = rbudpSender->rbudpBase.tcpSockfd;
    int verbose = rbudpSender->rbudpBase.verbose;

    // Receive the getfile message

    while ( true ) {
        char fname[SIZEOFFILENAME + 1];
        int n = readn( tcpSockfd, fname, SIZEOFFILENAME );
        if ( n <= 0 ) {
            fprintf( stderr, "read error.\n" );
            return FAILED;
        }
        fname[n] = '\0';

        // If "finish" signal (All zero), return success.
        char test[SIZEOFFILENAME];
        memset( ( void* )test, 0, SIZEOFFILENAME );
        if ( memcmp( test, fname, SIZEOFFILENAME ) == 0 ) {
            return RB_SUCCESS;
        }
        //otherwise, get the filename

        if ( verbose > 0 ) {
            fprintf( stderr, "Send file %s\n", fname );
        }

        int fd = open( fname, O_RDONLY );
        if ( fd < 0 ) {
            fprintf( stderr, "open file failed.\n" );
            return FAILED;
        }

        off_t lseek_return = lseek( fd, 0, SEEK_END );
        int errsv = errno;
        if ( ( off_t ) - 1 == lseek_return ) {
            fprintf( stderr, "SEEK_END lseek failed with error %d.\n", errsv );
            close( fd );
            return FAILED;
        }
        if ( lseek_return > std::numeric_limits<long long>::max() ) {
            fprintf( stderr, "file of size %ju is too large for a long long.\n", ( uintmax_t )lseek_return );
            close( fd );
            return FAILED;
        }
        long long filesize = ( long long )lseek_return;
        if ( verbose > 0 ) {
            fprintf( stderr, "The size of the file is %lld\n", filesize );
        }
        lseek_return = lseek( fd, 0, SEEK_SET );
        errsv = errno;
        if ( ( off_t ) - 1 == lseek_return ) {
            fprintf( stderr, "SEEK_SET lseek failed with error %d.\n", errsv );
            close( fd );
            return FAILED;
        }

        long long nfilesize = rb_htonll( filesize );

        // Send the file size to the receiver.
        if ( writen( tcpSockfd, ( char * )&nfilesize, sizeof( nfilesize ) ) != sizeof( nfilesize ) ) {
            {
                fprintf( stderr, "tcp send failed.\n" );
                close( fd );
                return FAILED;
            }
        }

        char *buf = ( char * )mmap( NULL, filesize, PROT_READ, MAP_SHARED, fd, 0 );

        sendBuf( rbudpSender, buf, filesize, sendRate, packetSize );

        munmap( buf, filesize );
        close( fd );
    }
}

/* sendRate: Kbps */
int  initSendRudp( rbudpSender_t *rbudpSender, void* buffer, int bufSize,
                   int sRate, int pSize ) {
    int i;

    rbudpSender->rbudpBase.mainBuffer = ( char * )buffer;
    rbudpSender->rbudpBase.dataSize = bufSize;
    rbudpSender->rbudpBase.sendRate = sRate;
    rbudpSender->rbudpBase.payloadSize = pSize;
    rbudpSender->rbudpBase.headerSize = sizeof( struct _rbudpHeader );
    rbudpSender->rbudpBase.packetSize = rbudpSender->rbudpBase.payloadSize
                                        + rbudpSender->rbudpBase.headerSize;
    rbudpSender->rbudpBase.usecsPerPacket =
        8 * rbudpSender->rbudpBase.payloadSize * 1000 /
        rbudpSender->rbudpBase.sendRate;
    rbudpSender->rbudpBase.isFirstBlast = 1;

    if ( rbudpSender->rbudpBase.dataSize %
            rbudpSender->rbudpBase.payloadSize == 0 ) {
        rbudpSender->rbudpBase.totalNumberOfPackets =
            rbudpSender->rbudpBase.dataSize /
            rbudpSender->rbudpBase.payloadSize;
        rbudpSender->rbudpBase.lastPayloadSize =
            rbudpSender->rbudpBase.payloadSize;
    }
    else {
        rbudpSender->rbudpBase.totalNumberOfPackets =
            rbudpSender->rbudpBase.dataSize /
            rbudpSender->rbudpBase.payloadSize + 1; /* the last packet is not full */
        rbudpSender->rbudpBase.lastPayloadSize =
            rbudpSender->rbudpBase.dataSize -
            rbudpSender->rbudpBase.payloadSize *
            ( rbudpSender->rbudpBase.totalNumberOfPackets - 1 );
    }

    rbudpSender->rbudpBase.remainNumberOfPackets =
        rbudpSender->rbudpBase.totalNumberOfPackets;
    rbudpSender->rbudpBase.sizeofErrorBitmap =
        rbudpSender->rbudpBase.totalNumberOfPackets / 8 + 2;
    rbudpSender->rbudpBase.errorBitmap =
        ( char * )malloc( rbudpSender->rbudpBase.sizeofErrorBitmap );
    rbudpSender->rbudpBase.hashTable =
        ( long long * )malloc( rbudpSender->rbudpBase.totalNumberOfPackets *
                               sizeof( long long ) );

    rbudpSender->rbudpBase.endOfUdp.round = 0;
    memset( rbudpSender->rbudpBase.endOfUdp.end, 'E', sizeof( rbudpSender->rbudpBase.endOfUdp.end ) );

    if ( rbudpSender->rbudpBase.verbose > 1 )
        TRACE_DEBUG( "totalNumberOfPackets: %d",
                     rbudpSender->rbudpBase.totalNumberOfPackets );
    if ( rbudpSender->rbudpBase.verbose > 1 )
        TRACE_DEBUG( "usecsPerPacket: %d",
                     rbudpSender->rbudpBase.usecsPerPacket );

    if ( rbudpSender->rbudpBase.errorBitmap == NULL ) {
        fprintf( stderr, "malloc errorBitmap failed\n" );
        return -1;
    }
    if ( rbudpSender->rbudpBase.hashTable == NULL ) {
        fprintf( stderr, "malloc hashTable failed\n" );
        return -1;
    }

    /* Initialize the hash table */
    for ( i = 0; i < rbudpSender->rbudpBase.totalNumberOfPackets; i++ ) {
        rbudpSender->rbudpBase.hashTable[i] = i;
    }
    return 0;
}


void  initSender( rbudpSender_t *rbudpSender, char *remoteHost ) {
    openSession( rbudpSender, remoteHost );
    listenAndInit( rbudpSender );
}

void  openSession( rbudpSender_t *rbudpSender, char *remoteHost ) {
#ifdef DEBUG
    log = fopen( "rbudpsend.log", "w" );
#endif
    connectUDP( &rbudpSender->rbudpBase, remoteHost );
    if ( !rbudpSender->rbudpBase.hasTcpSock ) {
        initTCPServer( &rbudpSender->rbudpBase );
    }
}

void  listenAndInit( rbudpSender_t *rbudpSender ) {
    if ( !rbudpSender->rbudpBase.hasTcpSock ) {
        listenTCPServer( &rbudpSender->rbudpBase );
    }

#if defined (__sgi)
//	msgSend.msg_name = (caddr_t)&udpServerAddr.sin_addr;
    rbudpSender->msgSend.msg_name = ( caddr_t )&rbudpSender->rbudpBase.udpServerAddr;
#else
//        msgSend.msg_name = (void *)&udpServerAddr.sin_addr;
//        msgSend.msg_name = reinterpret_cast<char*>(&udpServerAddr);
    rbudpSender->msgSend.msg_name = ( char* )( &rbudpSender->rbudpBase.udpServerAddr );
#endif
    rbudpSender->msgSend.msg_namelen = sizeof( rbudpSender->rbudpBase.udpServerAddr );
    rbudpSender->msgSend.msg_iov = rbudpSender->iovSend;
    rbudpSender->msgSend.msg_iovlen = 2;

//	rbudpSender->iovSend[0].iov_base = reinterpret_cast<char*>(&sendHeader);
    rbudpSender->iovSend[0].iov_base = ( char* )( &rbudpSender->sendHeader );
    rbudpSender->iovSend[0].iov_len = sizeof( struct _rbudpHeader );
}

void  sendClose( rbudpSender_t *rbudpSender ) {
    if ( !rbudpSender->rbudpBase.hasTcpSock ) {
        close( rbudpSender->rbudpBase.tcpSockfd );
        /* listenfd is not open for client */ // JMC - backport 4615
        if ( rbudpSender->rbudpBase.listenfd > 0 ) {
            close( rbudpSender->rbudpBase.listenfd );
        }
    }
    close( rbudpSender->rbudpBase.udpSockfd );
#ifdef DEBUG
    fclose( log );
#endif
}

