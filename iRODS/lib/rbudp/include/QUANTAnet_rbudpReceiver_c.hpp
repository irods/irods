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

#ifndef _QUANTAPLUS_RBUDPRECEIVER_C
#define _QUANTAPLUS_RBUDPRECEIVER_C

#include "QUANTAnet_rbudpBase_c.h"

/**
RBUDP Receiver class.  This class implements the receiver part of RBUDP protocol. First, instantiate the QUANTAnet_rbudpReceiver_c class. Then
call this object's init() method with the host name or IP address of the
sender as parameter.  After the connection is established, you should
immediately call this object's receive(buffer, sizeofBuffer,
sizeofPacket) to receive a buffer.  After you finish receiving all buffers,
you can call this object's close() to close this session.

*/

#define RECEIVE_PORT       38000

typedef struct  QUANTAnet_rbudpReceiver_c
{
        rbudpBase_t rbudpBase;
	struct msghdr msgRecv;
	struct iovec iovRecv[2];
	struct _rbudpHeader recvHeader;
} rbudpReceiver_t;

	void udpReceiveReadv();
	int udpReceive(rbudpReceiver_t *rbudpReceiver);
	int initReceiveRudp(rbudpReceiver_t *rbudpReceiver, void* buffer, 
	  int bufSize, int pSize);
	
	/** Constructor by telling which TCP and UDP ports we are going to use.
		@ param port the TCP server port and UDP local and remote ports will be calculated based on it.
	*/
	void
	QUANTAnet_rbudpReceiver_c(rbudpReceiver_t *rbudpReceiver, int port);

	/** Constructor when we want to reuse exising TCP socket.
		@ param tcpsock the TCP socket we are going to use in RBUDP.
		@ param port UDP local and remote ports will be calculated based on it.
	*/
	void
	QUANTAnet_rbudpReceiver_c_reuse (rbudpReceiver_t *rbudpReceiver, 
	  int tcpsock, int port);
	// ~QUANTAnet_rbudpReceiver_c() {};
	/** Receive a block of memory from the sender using RBUDP protocol.
		@param buffer the pointer of the buffer you put the received data into.
		@param bufSize the size of the received data.  It has to be consistent with the sender.
		@param packetSize payload size of each UDP packet, suggest 1460 considering the Ethernet MTU restriction and the header size.  It has to be consistent with the sender.
	*/
	int receiveBuf (rbudpReceiver_t *rbudpReceiver, void * buffer, 
	  int bufSize,  int packetSize);

        /** Receive a file from the sender using RBUDP protocol.
	        @param origFName the name of the file you want to get in sender side.
                @param destFName the name of the file you want to save as in receiver side.
		@param packetSize payload size of each UDP packet.
	*/
        int getfile(rbudpReceiver_t *rbudpReceiver, char * origFName, 
	  char * destFName, int packetSize);
	int getfileByFd (rbudpReceiver_t *rbudpReceiver, int fd, 
          int packetSize);
        
	/** Receive a set of files from the sender using RBUDP protocol.
	        @param fileList the name of the file you have all source and dest filenames.
		@param packetSize payload size of each UDP packet.
	*/
        int getfilelist(rbudpReceiver_t *rbudpReceiver, char * fileList, 
	  int packetSize);

	/** Receive a stream, write it to a UNIX file descriptor
		@param tofd   file descriptor to write data to
		@param packetSize  payload size of each UDP packet
	    (buffer size is determined by sender)
	 */
	int getstream(rbudpReceiver_t *rbudpReceiver, int tofd, int packetSize);

	/** Initialize a RBUDP session in the receiving side
		@param remoteHost the name of the sending host.
	*/
	void initReceiver (rbudpReceiver_t *rbudpReceiver, char *remoteHost);
	/// Close the RBUDP session.
	void recvClose (rbudpReceiver_t *rbudpReceiver);
#endif
