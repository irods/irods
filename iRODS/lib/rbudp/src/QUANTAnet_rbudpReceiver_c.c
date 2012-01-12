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

#include "QUANTAnet_rbudpReceiver_c.h"

#include <stdarg.h>

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

#if 0
inline void TRACE_DEBUG( char *format, ...)
{
	va_list arglist;
	va_start( arglist, format);
	vfprintf( stderr, format, arglist);
	fprintf( stderr, "\n");
	va_end(arglist);
}
#endif

void
QUANTAnet_rbudpReceiver_c(rbudpReceiver_t *rbudpReceiver, int port) {
        rbudpReceiver->rbudpBase.tcpPort = port;
        rbudpReceiver->rbudpBase.udpLocalPort = port + 1;
        rbudpReceiver->rbudpBase.udpRemotePort = port;
        rbudpReceiver->rbudpBase.hasTcpSock = 0;
}

void
QUANTAnet_rbudpReceiver_c_reuse (rbudpReceiver_t *rbudpReceiver,
int tcpsock, int port) {
        rbudpReceiver->rbudpBase.hasTcpSock = 1;
        rbudpReceiver->rbudpBase.tcpSockfd = tcpsock;
        rbudpReceiver->rbudpBase.udpLocalPort = port;
        rbudpReceiver->rbudpBase.udpRemotePort = port;
}

int  receiveBuf (rbudpReceiver_t *rbudpReceiver, void * buffer, int bufSize, 
int packetSize)
{
	int done = 0;
        int status = 0;
	struct timeval curTime, startTime;
        int verbose = rbudpReceiver->rbudpBase.verbose;
	
	gettimeofday(&curTime, NULL);
	startTime = curTime;
	initReceiveRudp(rbudpReceiver, buffer, bufSize, packetSize);	
	initErrorBitmap(&rbudpReceiver->rbudpBase);
	while (!done)
	{
		// receive UDP packetsq		TRACE_DEBUG("receiving UDP packets");
		reportTime(&curTime);
			
		status = udpReceive(rbudpReceiver);  
		if (status < 0) return status;
	
		reportTime(&curTime);

		gettimeofday(&curTime, NULL);
		if(verbose>1) TRACE_DEBUG("Current time: %d %ld", curTime.tv_sec, curTime.tv_usec);
		
		if (updateHashTable(&rbudpReceiver->rbudpBase) == 0)
			done = 1;
		
		if(verbose) {
		    float dt = (curTime.tv_sec - startTime.tv_sec) +
			     1e-6*(curTime.tv_usec - startTime.tv_usec);
		    int nerrors = updateHashTable(&rbudpReceiver->rbudpBase);
		    int got = packetSize * 
		      (rbudpReceiver->rbudpBase.totalNumberOfPackets - nerrors);
		    float mbps = 1e-6 * 8 * got / (dt==0 ? .01 : dt);
		    TRACE_DEBUG("Error: %d, Loss rate: %.5f Got %d/%dK in %.2fs (%g Mbit/s)",
			nerrors,
			(double)nerrors/
			(double)rbudpReceiver->rbudpBase.totalNumberOfPackets,
			(int)(got>>10), (int)(bufSize>>10), dt, mbps);
		}
		
		// Send back Error bitmap
		if (writen(rbudpReceiver->rbudpBase.tcpSockfd, 
		  rbudpReceiver->rbudpBase.errorBitmap, 
		  rbudpReceiver->rbudpBase.sizeofErrorBitmap) != 
		  rbudpReceiver->rbudpBase.sizeofErrorBitmap)
                {
                        perror("tcp send");
			return (errno ? (-1 * errno) : -1);
                }			
	}
	free(rbudpReceiver->rbudpBase.errorBitmap);
	free(rbudpReceiver->rbudpBase.hashTable);
	return (0);
}	

#if 0	/* not used */
void  udpReceiveReadv()
{
	int done;
	long long seqno, packetno;
	struct timeval timeout;
	fd_set rset;
	int maxfdpl;
	done = 0; packetno=0;
	
	timeout.tv_sec = 100;
	timeout.tv_usec = 0;
	#define QMAX(x, y) ((x)>(y)?(x):(y))
	maxfdpl = QMAX(udpSockfd, tcpSockfd) + 1;
	FD_ZERO(&rset);
	while (!done)
	{
		// These two FD_SET cannot be put outside the while, don't why though
		FD_SET(udpSockfd, &rset);
		FD_SET(tcpSockfd, &rset);
		select(maxfdpl, &rset, NULL, NULL, &timeout);

		// receiving a packet
		if (FD_ISSET(udpSockfd, &rset))
		{
			// set seqno to expected sequence number
			seqno = hashTable[packetno];
			//fprintf(log, "expect %d\t", seqno);
			iovRecv[1].iov_base = (char *)mainBuffer+(seqno*payloadSize);
			iovRecv[1].iov_len = payloadSize;
			if (recvmsg(udpSockfd, &msgRecv, 0) < 0)
			{
				perror("recvmsg error\n");
				exit(1);
			}

			int rcvseq = ptohseq( recvHeader.seq );
			// make sure it is the expected packet
			if (rcvseq == seqno)
			{
				updateErrorBitmap(rcvseq);
				// next expected packet
				packetno ++;
			}
			// if some packets lost before this packet, move this packet backward
			else if (rcvseq > seqno)
			{
				bcopy(iovRecv[1].iov_base, (char *)mainBuffer+(rcvseq*payloadSize) , iovRecv[1].iov_len);
				updateErrorBitmap(rcvseq);
				// missed some packets, next expected packet
				do packetno ++;
				while((rcvseq != hashTable[packetno]) && (packetno <= remainNumberOfPackets));
				if (packetno > remainNumberOfPackets)
				{	
					fprintf(stderr,"recv error\n");
				}
				packetno ++;
			}
			//TRACE_DEBUG("received %d -> %d", recvHeader.seq, rcvseq);
		}
		//receive end of UDP signal
		else if (FD_ISSET(tcpSockfd, &rset))
		{
			done = 1;
			if(verbose) TRACE_DEBUG("received TCP signal");
                	readn(tcpSockfd, (char *)&endOfUdp, sizeof(struct _endOfUdp));
		}
		else // time out
		{
			done = 1;
		}
	}		
}
#endif

int  udpReceive (rbudpReceiver_t *rbudpReceiver)
{
	int done, actualPayloadSize, retval;
	long long seqno;
	struct timeval start;
	char *msg = (char *) malloc(rbudpReceiver->rbudpBase.packetSize);	
	struct timeval timeout;
	fd_set rset;
	int maxfdpl;
	float prog;
	int oldprog=0;
	done = 0; seqno = 0;
	
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	#define QMAX(x, y) ((x)>(y)?(x):(y))
	maxfdpl = QMAX(rbudpReceiver->rbudpBase.udpSockfd, 
	  rbudpReceiver->rbudpBase.tcpSockfd) + 1;
	FD_ZERO(&rset);
	gettimeofday(&start, NULL);
	while (!done)
	{
		// These two FD_SET cannot be put outside the while, don't why though
		FD_SET(rbudpReceiver->rbudpBase.udpSockfd, &rset);
		FD_SET(rbudpReceiver->rbudpBase.tcpSockfd, &rset);
		retval = select(maxfdpl, &rset, NULL, NULL, &timeout);

		// receiving a packet
		if (FD_ISSET(rbudpReceiver->rbudpBase.udpSockfd, &rset))
		{
		        if (rbudpReceiver->rbudpBase.udpServerAddr.sin_addr.s_addr == htonl(INADDR_ANY)) {
			    // made connect already
		            if (recv (rbudpReceiver->rbudpBase.udpSockfd, msg, 
			      rbudpReceiver->rbudpBase.packetSize, 0) < 0) {
                                perror("recv");
                                return (errno ? (-1 * errno) : -1);
                            }
		        } else {
			    socklen_t fromlen = 
			      sizeof(rbudpReceiver->rbudpBase.udpServerAddr);
		            if (recvfrom (rbudpReceiver->rbudpBase.udpSockfd,
                              msg, rbudpReceiver->rbudpBase.packetSize, 0,
			      (struct sockaddr *)
			      &rbudpReceiver->rbudpBase.udpServerAddr,
          		      &fromlen) < 0) {
                                perror("recvfrom");
                                return (errno ? (-1 * errno) : -1);
                            }
		        }
			
			bcopy(msg, &rbudpReceiver->recvHeader, 
			  sizeof(struct _rbudpHeader));
			seqno = ptohseq(&rbudpReceiver->rbudpBase,
			   rbudpReceiver->recvHeader.seq );

			// If the packet is the last one, 
			if (seqno < 
			  rbudpReceiver->rbudpBase.totalNumberOfPackets - 1)
			{
				actualPayloadSize = 
				  rbudpReceiver->rbudpBase.payloadSize;
			}
			else
			{
				actualPayloadSize = 
				  rbudpReceiver->rbudpBase.lastPayloadSize; 
			}

			bcopy(msg+rbudpReceiver->rbudpBase.headerSize, 
			  (char *)rbudpReceiver->rbudpBase.mainBuffer+
			  (seqno*rbudpReceiver->rbudpBase.payloadSize) , 
			  actualPayloadSize);

			updateErrorBitmap(&rbudpReceiver->rbudpBase, seqno);

			rbudpReceiver->rbudpBase.receivedNumberOfPackets ++;
			prog = (float) 
			  rbudpReceiver->rbudpBase.receivedNumberOfPackets / 
			  (float) rbudpReceiver->rbudpBase.totalNumberOfPackets
			  * 100;
			if ((int)prog > oldprog) {
				oldprog = (int)prog;
				if (oldprog > 100) oldprog = 100;
				if(rbudpReceiver->rbudpBase.progress != 0) {
				    fseek(rbudpReceiver->rbudpBase.progress, 
				      0, SEEK_SET);
				    fprintf(rbudpReceiver->rbudpBase.progress, 
				      "%d\n", oldprog);
				}
			}

		}
		//receive end of UDP signal
		else if (FD_ISSET(rbudpReceiver->rbudpBase.tcpSockfd, &rset))
		{
			done = 1;
                	readn(rbudpReceiver->rbudpBase.tcpSockfd, 
			  (char *)&rbudpReceiver->rbudpBase.endOfUdp, 
			  sizeof(struct _endOfUdp));
		}
		else // time out
		{
			//done = 1;
		}
	}		
	free(msg);

	return 0;
}

int  getstream(rbudpReceiver_t *rbudpReceiver, int tofd, int packetSize )
{
   int verbose = rbudpReceiver->rbudpBase.verbose;

   // Let sender know we're ready.
   if (writen(rbudpReceiver->rbudpBase.tcpSockfd, "\0", 1) != 1)
   {
	perror("tcp send");
	return -1;
   }

   long long curSize = -1;
   char *buf = 0;
   int ok = RB_SUCCESS;

   for(;;) {

       long long nbufSize, bufSize;
       int n = readn(rbudpReceiver->rbudpBase.tcpSockfd, 
	  (char *)&nbufSize, sizeof(nbufSize));
       if (n < 0) {
	 fprintf(stderr,"read error.\n");
	 return(FAILED);
       }

       bufSize = rb_ntohll( nbufSize );

       if(bufSize <= 0)
	 break;

       if(verbose>1) 
	   fprintf(stderr, "accepting %lld byte chunk\n", bufSize);

       if(buf == 0 && bufSize != curSize) {
	 if(buf) free(buf);
         buf = (char *)malloc( bufSize );
         if(buf == 0) {
	     fprintf(stderr, " getstream: Couldn't malloc %lld bytes for buffer\n", bufSize);
	     ok = FAILED;
	     break;
	 }
	 curSize = bufSize;
	}

	receiveBuf (rbudpReceiver, buf, bufSize, packetSize );

	if(write( tofd, buf, bufSize ) < bufSize) {
	    fprintf(stderr, " getstream: couldn't write %lld bytes\n", bufSize);
	    ok = FAILED;
	    break;
	}
    }
    if(buf != 0)
	free(buf);
    close(tofd);
    return(ok);
}

int  getfile(rbudpReceiver_t *rbudpReceiver, char * origFName, 
char * destFName, int packetSize)
{
  int status;

  // Send getfile message.
  if (origFName != NULL) {
     if (writen(rbudpReceiver->rbudpBase.tcpSockfd, origFName, 
       SIZEOFFILENAME) != SIZEOFFILENAME) {
         perror("tcp send");
         return FAILED;	
     }
  }

   int fd = open(destFName, O_RDWR|O_CREAT|O_TRUNC, 0666);

   if (fd < 0) return (errno ? (-1 * errno) : -1); 
   status = getfileByFd (rbudpReceiver, fd, packetSize);
   close(fd);

   return (status);
}

int
getfileByFd (rbudpReceiver_t *rbudpReceiver, int fd, int packetSize)
{  
   long long filesize;
   int status = 0;
   int verbose = rbudpReceiver->rbudpBase.verbose;
  long long remaining;
  long long offset = 0;

   int n = readn(rbudpReceiver->rbudpBase.tcpSockfd, 
     (char *)&filesize, sizeof(filesize));
   if (n < 0) {
     fprintf(stderr,"read error.\n");
      return(errno ? (-1 * errno) : -1);
   }

   /* Can't use ntohl() on long longs! */
   filesize = rb_ntohll(filesize);
   if (verbose > 0) fprintf(stderr,"The size of the file is %lld.\n", filesize);
   
   ftruncate(fd, filesize);
   
   remaining = filesize;
   while (remaining > 0) {
      uint toRead;
      char *buf;

      if (remaining > (uint) ONE_GIGA) {
         toRead = (uint) ONE_GIGA;
      } else {
         toRead = remaining;
      }

      if (verbose > 0)
          TRACE_DEBUG("Receiving %d bytes chunk. %lld bytes remaining", 
            toRead, remaining - toRead);

      buf = (char *)mmap(NULL, toRead, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 
        offset);
      if (buf == MAP_FAILED) {
        fprintf(stderr,"mmap failed. toRead = %d, offset = %lld, errno = %d\n",
         toRead, offset, errno);
        return (errno ? (-1 * errno) : -1);
      }

      status = receiveBuf (rbudpReceiver, buf, toRead, packetSize);

      munmap(buf, toRead);
      if (status < 0) {
         fprintf (stderr, "receiveBuf error, status = %d\n", status);
         break;
      }
      remaining -= toRead;
      offset += toRead;
   }

   
   return status; 
}

int  getfilelist(rbudpReceiver_t *rbudpReceiver, char * fileList, 
int packetSize)
{
  FILE *fp = fopen(fileList, "r");
  if (fp == NULL) {
	fprintf(stderr,"Error open file!\n");
        return -1;
  }

  char str[SIZEOFFILENAME];
  char *origFName, *destFName;
  while(fgets(str,SIZEOFFILENAME,fp)) {
    puts(str);
    origFName = strtok(str, " ");
    destFName = strtok(NULL, " ");
    if ((origFName != NULL) && (destFName != NULL)) {

  // Send getfile message.
   if (writen(rbudpReceiver->rbudpBase.tcpSockfd, origFName, SIZEOFFILENAME) !=
     SIZEOFFILENAME)
   {
       perror("tcp send");
       return FAILED;
   }
   
   long long filesize;
   int n = readn(rbudpReceiver->rbudpBase.tcpSockfd, (char *)&filesize, 
     sizeof(filesize));
   if (n < 0) {
     fprintf(stderr,"read error.\n");
     return(FAILED);
   }

   filesize = rb_ntohll(filesize);
   fprintf(stderr,"The size of the file is %lld.\n", filesize);

   int fd = open(destFName, O_RDWR|O_CREAT|O_TRUNC, 0666);
   ftruncate(fd, filesize);

   char *buf;
   buf = (char *)mmap(NULL, filesize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
   if (buf == MAP_FAILED) {
     fprintf(stderr,"mmap failed.\n");
     return FAILED;
   }

   receiveBuf(rbudpReceiver, buf, filesize, packetSize);

   munmap(buf, filesize);
   close(fd);
  }
  } 

  // send all zero block to end the sender.
  bzero(str, SIZEOFFILENAME);
  if (writen(rbudpReceiver->rbudpBase.tcpSockfd, str, SIZEOFFILENAME) != 
    SIZEOFFILENAME)
  {
    perror("Error Tcp Send");
    return FAILED;
  }

  fclose(fp);
   return RB_SUCCESS;
}

/* sendRate: Mbps */
int  initReceiveRudp(rbudpReceiver_t *rbudpReceiver, void* buffer, 
int bufSize, int pSize)
{
	int i;
	rbudpReceiver->rbudpBase.mainBuffer = (char *)buffer;
	rbudpReceiver->rbudpBase.dataSize = bufSize;
	rbudpReceiver->rbudpBase.payloadSize = pSize;
	rbudpReceiver->rbudpBase.headerSize = sizeof(struct _rbudpHeader);
	rbudpReceiver->rbudpBase.packetSize = 
	  rbudpReceiver->rbudpBase.payloadSize + 
	  rbudpReceiver->rbudpBase.headerSize;
	rbudpReceiver->rbudpBase.isFirstBlast = 1;

	if (rbudpReceiver->rbudpBase.dataSize % 
	  rbudpReceiver->rbudpBase.payloadSize == 0)
	{
		rbudpReceiver->rbudpBase.totalNumberOfPackets = 
		  rbudpReceiver->rbudpBase.dataSize /
		    rbudpReceiver->rbudpBase.payloadSize;
		rbudpReceiver->rbudpBase.lastPayloadSize = 
		  rbudpReceiver->rbudpBase.payloadSize;
	}
	else
	{
		rbudpReceiver->rbudpBase.totalNumberOfPackets = 
		  rbudpReceiver->rbudpBase.dataSize /
		  rbudpReceiver->rbudpBase.payloadSize + 1; /* the last packet is not full */
		rbudpReceiver->rbudpBase.lastPayloadSize = 
		  rbudpReceiver->rbudpBase.dataSize - 
		  rbudpReceiver->rbudpBase.payloadSize * 
		  (rbudpReceiver->rbudpBase.totalNumberOfPackets - 1);
	}
	rbudpReceiver->rbudpBase.remainNumberOfPackets = 
	  rbudpReceiver->rbudpBase.totalNumberOfPackets;
	rbudpReceiver->rbudpBase.receivedNumberOfPackets = 0;
	rbudpReceiver->rbudpBase.sizeofErrorBitmap = 
	  rbudpReceiver->rbudpBase.totalNumberOfPackets/8 + 2;
	rbudpReceiver->rbudpBase.errorBitmap = 
	  (char *)malloc(rbudpReceiver->rbudpBase.sizeofErrorBitmap);
	rbudpReceiver->rbudpBase.hashTable = 
	  (long long *)malloc(rbudpReceiver->rbudpBase.totalNumberOfPackets * 
	  sizeof(long long));
	
	if(rbudpReceiver->rbudpBase.verbose>1) 
	  TRACE_DEBUG("totalNumberOfPackets: %d", 
	    rbudpReceiver->rbudpBase.totalNumberOfPackets);

	if (rbudpReceiver->rbudpBase.errorBitmap == NULL)
	{
		fprintf(stderr,"malloc errorBitmap failed\n");
		return FAILED;
	}
	if (rbudpReceiver->rbudpBase.hashTable == NULL)
	{
		fprintf(stderr,"malloc hashTable failed\n");
		return FAILED;
	}

	/* Initialize the hash table */
	for (i=0; i<rbudpReceiver->rbudpBase.totalNumberOfPackets; i++)
	{
		rbudpReceiver->rbudpBase.hashTable[i] = i;
	}
	return 0;
}


void  initReceiver (rbudpReceiver_t *rbudpReceiver, char *remoteHost)
{
	int n;
	int verbose =  rbudpReceiver->rbudpBase.verbose;

#ifdef DEBUG
	log = fopen("rbudprecv.log","w");
#endif
	if(verbose>2)
	    rbudpReceiver->rbudpBase.progress = fopen("progress.log", "w");	
	else
	    rbudpReceiver->rbudpBase.progress = 0;

	passiveUDP(&rbudpReceiver->rbudpBase, remoteHost);
	// connectUDP (&rbudpReceiver->rbudpBase, remoteHost);

	if (!rbudpReceiver->rbudpBase.hasTcpSock) 
	{
		if(verbose) TRACE_DEBUG("try to connect the sender via TCP ...");
		n = connectTCP(&rbudpReceiver->rbudpBase, remoteHost);
		if (n < 0)
		{
			fprintf(stderr,"connecting TCP failed, make sure the sender has been started\n");
			exit(1);
		}
		if(verbose) TRACE_DEBUG("tcp connected.");
	}
       
	rbudpReceiver->msgRecv.msg_name = NULL;
	rbudpReceiver->msgRecv.msg_namelen = 0;
	rbudpReceiver->msgRecv.msg_iov = rbudpReceiver->iovRecv;
	rbudpReceiver->msgRecv.msg_iovlen = 2;
	
	// iov_base = reinterpret_cast<char*>(&recvHeader);
	rbudpReceiver->iovRecv[0].iov_base = 
	  (char*)(&rbudpReceiver->recvHeader);
	rbudpReceiver->iovRecv[0].iov_len = sizeof(struct _rbudpHeader);
}
	

void recvClose (rbudpReceiver_t *rbudpReceiver)
{
	if (! rbudpReceiver->rbudpBase.hasTcpSock)
		close(rbudpReceiver->rbudpBase.tcpSockfd);
	close(rbudpReceiver->rbudpBase.udpSockfd);
#ifdef DEBUG
	fclose(rbudpReceiver->rbudpBase.log);
#endif
	if(rbudpReceiver->rbudpBase.progress != 0)
	    fclose(rbudpReceiver->rbudpBase.progress);
}
