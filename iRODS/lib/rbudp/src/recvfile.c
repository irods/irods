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

int
main (int argc, char **argv)
{
  struct timeval start, end;
  rbudpReceiver_t *rbudpReceiver;

  if (argc < 5)
    {
      printf
	("Usage: recvfile <sender> <path to the orig file> <path to the dest file name> <MTU>\n");
      printf
	(" \n Example : recvfile <192.168.81.100 /path/to/file/on/sender /path/to/file/on/receiver 9000 \n");
      exit (1);
    }


  rbudpReceiver = malloc (sizeof (rbudpReceiver_t));
  memset (rbudpReceiver, 0, sizeof (rbudpReceiver_t));

  // the constructor
  QUANTAnet_rbudpReceiver_c (rbudpReceiver, RECEIVE_PORT);
  
  // the init
  initReceiver (rbudpReceiver, argv[1]);

  printf ("init finishes\n");
  gettimeofday (&start, NULL);
  QUANTAnet_rbudpBase_c (&rbudpReceiver->rbudpBase); 
  getfile (rbudpReceiver, argv[2], argv[3], atoi (argv[4]));
  gettimeofday (&end, NULL);
  recvClose (rbudpReceiver);

  printf ("time consumed: %d microseconds\n", USEC (&start, &end));
  return 1;
}
