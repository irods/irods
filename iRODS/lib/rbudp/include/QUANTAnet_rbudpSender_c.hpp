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

#ifndef _QUANTAPLUS_RBUDPSENDER_C

#define _QUANTAPLUS_RBUDPSENDER_C

#include "QUANTAnet_rbudpBase_c.h"

#define SEND_PORT	38000
#define SEND_BUF_SIZE	 64*1024*1024
#define MAX_SEND_ERR_CNT	1000
#define MAX_NO_PROGRESS_CNT	10

/**
RBUDP Sender class.   This class implements the sender part of RBUDP protocol. First, instantiate the QUANTAnet_rbudpSender_c class. Then
call this object's init() method with the host name or IP address of the
receiver as parameter. This is a blocking call that waits for the receiver
to make to init() call to connect to the sender.  After that, you can call
this object's send(buffer, sizeofBuffer, sendRate, sizeofPacket) to send
a buffer.  After you finish sending all buffers, you can call this object's
close() to close this session.
 
*/

typedef struct  QUANTAnet_rbudpSender_c
{
	rbudpBase_t rbudpBase;
	struct msghdr msgSend;
	struct iovec iovSend[2];
	struct _rbudpHeader sendHeader;
} rbudpSender_t;

	void udpSendWritev();
	int udpSend(rbudpSender_t *rbudpSender);
	int initSendRudp(rbudpSender_t *rbudpSender, void* buffer, 
          int bufSize, int sRate, int pSize);

        /** Constructor by telling which TCP and UDP ports we are going to use.
                @ param port the TCP server port and UDP local and remote ports will be calculated based on it.
        */
	void 
	QUANTAnet_rbudpSender_c(rbudpSender_t *rbudpSender, int port);

        /** Constructor when we want to reuse exising TCP socket.
                @ param tcpsock the TCP socket we are going to use in RBUDP.
                @ param port UDP local and remote ports will be calculated based on it.
        */
	void QUANTAnet_rbudpSender_reuse (rbudpSender_t *rbudpSender,
	  int tcpsock, int port);
	// ~QUANTAnet_rbudpSender_c() {};
	/** Send a memory block using RBUDP protocol
		@param buffer the pointer of the buffer you want to send.
		@param bufSize the size of the buffer you want to send.
		@param sendRate the first-pass UDP blasting rate in Kbps, should be decided based on the actual available bandwidth.
		@param packetSize payload size of each UDP packet, suggest 1460 considering the total plusing the header not exceeding the Ethernet MTU 1500.
	*/
	int sendBuf (rbudpSender_t *rbudpSender, void * buffer, int bufSize, 
	  int sendRate, int packetSize);


        /** Send a file to the receiver using RBUDP protocol.
	        @param sendRate the first-pass UDP blasting rate in Kbps.
		@param packetSize payload size of each UDP packet.
	*/
        int rbSendfile(rbudpSender_t *rbudpSender, int sendRate, int packetSize,
	  char *fname);
        
	int sendfileByFd (rbudpSender_t *rbudpSender, int sendRate, 
	  int packetSize, int fd);

        /** Send a set of files to the receiver using RBUDP protocol.
	        @param sendRate the first-pass UDP blasting rate in Kbps.
		@param packetSize payload size of each UDP packet.
	*/
        int sendfilelist(rbudpSender_t *rbudpSender, int sendRate, 
	  int packetSize);

        /** Send a data stream from a UNIX file descriptor
	    to the receiver using RBUDP protocol.
		@param fromfd   the file descriptor to fetch data from
	        @param sendRate the first-pass UDP blasting rate in Kbps.
		@param packetSize payload size of each UDP packet.
		@param bufSize  buffer size -- data is sent in this-size chunks
	*/
        int sendstream(rbudpSender_t *rbudpSender, int fromfd, int sendRate, 
	  int packetSize, int bufSize);

	/** Initialize a RBUDP session
		@param remoteHost the name of the receiving host
	*/
	void initSender (rbudpSender_t *rbudpSender, char *remoteHost);

  /** As part of initialization, open TCP and UDP sockets as needed
      @param remoteHost the name of receiving host
  */
  void openSession(rbudpSender_t *rbudpSender, char *remoteHost);

  /// As part of initialization, TCP socket listens and initialize members
  void listenAndInit(rbudpSender_t *rbudpSender);

	/// Close the RBUDP session 
	void sendClose(rbudpSender_t *rbudpSender);
#endif
