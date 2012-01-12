/* This is a very simple test of the irods standard IO emulation
  library.  When using standard i/o, include stdio.h; to change to
  using isio, change stdio.h to isio.h. */

/* include <stdio.h> */
#include "isio.h"   /* the irods standard IO emulation library */
#include <stdlib.h>
#include <string.h>
main(argc, argv)
int argc;
char **argv;
{
    FILE *FI, *FO;
    int i1,i2,i3;
    int count, rval, wval;
    char buf[1024*100];
    int bufsize;
 
    if (argc < 3) {
      printf("test1 file-in file-out [seek-position] [buffersize] \n");
      exit(-1);
    }

    FI = fopen(argv[1],"r");
    if (FI==0)
    {
        fprintf(stderr,"can't open input file %s\n",argv[1]);
        exit(-2);
    }

    FO = fopen(argv[2],"w");
    if (FO==0) {
        fprintf(stderr,"can't open output file %s\n",argv[2]);
        exit(-3);
    }

    if (argc >= 4) {
       long seekPos;
       seekPos=atol(argv[3]);
       fseek(FO, seekPos, SEEK_SET);
    }

    bufsize = sizeof(buf);

    if (argc >= 5) {
       int i;
       i = atoi(argv[4]);
       if (i > bufsize) {
	  perror("buffer size is too big");
	  exit(-4);
       }
       if (i <= 0) {
	  perror("invalid buffer size");
	  exit(-5);
       }
       bufsize=i;
    }

    do {
      memset(buf, 0, sizeof(buf));
      printf("buffer size=%d\n",bufsize);
      rval = fread(&buf[0], 1, bufsize, FI);
      printf("rval=%d\n",rval);
      if (rval < 0) {
	perror("read stream message");
      }
      if (rval > 0) {
	wval = fwrite(&buf[0], 1, rval, FO);
	if (wval < 0) {
	  perror("fwriting data:");
	}
      }
    } while (rval > 0);
    fclose(FI);
    fclose(FO);
    exit(0);
}
