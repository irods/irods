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
    int ichar;
 
    if (argc < 3) {
      printf("test2 file-in file-out [seek-position]\n");
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


    do {
       ichar = fgetc(FI);
       if (ichar < 0) {
	  perror("fgetc");
       }
       if (ichar != EOF) {
	  wval = fputc(ichar, FO);
	  if (wval < 0) {
	     perror("fputc");
	  }
       }
    } while (ichar != EOF);
    fclose(FI);
    fclose(FO);
    exit(0);
}
