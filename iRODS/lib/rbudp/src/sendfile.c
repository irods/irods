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

#include "QUANTAnet_rbudpSender_c.h"

int
main (int argc, char **argv)
{
  int sendRate;
  rbudpSender_t *rbudpSender;

  if (argc < 4)
    {
      printf ("Usage: sendfile <receiver> <sending rate (Kbps)> <MTU>\n");
      exit (1);
    }

  sendRate = atoi (argv[2]);
  // QUANTAnet_rbudpSender_c *mysender = new QUANTAnet_rbudpSender_c (38000);
  rbudpSender = malloc (sizeof (rbudpSender_t));
  memset (rbudpSender, 0, sizeof (rbudpSender_t));

  // the constructor
  QUANTAnet_rbudpSender_c (rbudpSender, SEND_PORT);

  // mysender->init (argv[1]);
  initSender (rbudpSender, argv[1]);

  QUANTAnet_rbudpBase_c (&rbudpSender->rbudpBase);
  // mysender->sendfile (sendRate, atoi (argv[3]));
  rbSendfile (rbudpSender, sendRate, atoi (argv[3]), (char *) 0);

  // mysender->close ();
  sendClose (rbudpSender);
  return 1;
}
