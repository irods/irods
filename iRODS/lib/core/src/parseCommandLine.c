/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* 

This routine parses the command line, converting it into values in the
rodsArguments_t structure.  This is used by all the RODS C client
commands (the R commands), which helps ensure that we use common
options throughout the command set.  Also see the
doc/DesignSpecs/CommandLineOptions document.

If you need additional options, please update this routine and the
CommandLineOptions document so we can keep it all consistent.
*/

#include "rods.h"
#include "parseCommandLine.h"
#include "rodsLog.h"
#include "rcMisc.h"
#ifdef windows_platform
#include "irodsntutil.h"
#endif


/* 
 Input: 
   argc and argv,
   optString: the option string to use, should be a subset of the
     full parseCommandLine list, the ones supported by this caller.
     If null or empty, use the full list.
   includeLong: if non-0 also check on the long form options, else do not.
 Output:
   the rodsArg structure is returned
 */
int 
parseCmdLineOpt (int argc, char **argv, char *optString, int includeLong,
		 rodsArguments_t *rodsArgs) {
   char opt;

   int i;

   char fullOpts[]="aAbc:C:dD:efFghH:ikK:lm:n:N:p:P:qrR:s:S:t:Tu:vVzZxWY:";
   char *opts;
   int VCount=0;

   /* Set all flags and pointers to false/null */
   memset(rodsArgs, 0, sizeof(rodsArguments_t));

   if (argv != NULL && argv[0] != NULL) {
	/* set SP_OPTION to argv[0] so it can be passed to server */
	char child[MAX_NAME_LEN], parent[MAX_NAME_LEN];
	*child = '\0';
	splitPathByKey (argv[0], parent, child, '/');
	if (*child != '\0') {
	    mySetenvStr (SP_OPTION, child);
	}
    }

   /* handle the long options first */
   if (includeLong) {
      for (i=0;i<argc;i++) {
         if (strcmp("--link", argv[i])==0) {
            rodsArgs->link=True;
            argv[i]="-Z";  /* ignore symlink */
         }

	 if (strcmp("--parallel", argv[i])==0) {
	    rodsArgs->parallel=True;
	    argv[i]="-Z";  /* place holder so the getopt will parse */
	 }
	 if (strcmp("--serial", argv[i])==0) {
	    rodsArgs->serial=True;
	    argv[i]="-Z";
	 }
         if (strcmp("--master-icat", argv[i])==0) {
	    rodsArgs->masterIcat=True;
	    argv[i]="-Z";
	 }
         if (strcmp("--silent", argv[i])==0) {  /* also -W */
            rodsArgs->silent=True;
            argv[i]="-Z";
         }
         if (strcmp("--test", argv[i])==0) {
	    rodsArgs->test=True;
	    argv[i]="-Z";
	 }
	 if (strcmp("--verify", argv[i])==0) {  /* also -x */
	    rodsArgs->verify=True;
	    argv[i]="-Z";
	 }
	 if (strcmp("--version", argv[i])==0) {  /* also -W */
	    rodsArgs->version=True;
	    argv[i]="-Z";
	 }
	 if (strcmp("--retries", argv[i])==0) {  /* also -Y */
	    rodsArgs->retries=True;
	    argv[i]="-Z";
            if (i + 2 < argc) {
               if (*argv[i+1] == '-') {
                   rodsLog (LOG_ERROR,
                    "--retries option needs an iput file");
                    return USER_INPUT_OPTION_ERR;
               }
	       rodsArgs->retriesValue=atoi(argv[i+1]);
               argv[i+1]="-Z";
            }
	 }
	 if (strcmp("--no-page", argv[i])==0) {
	    rodsArgs->noPage=True;
	    argv[i]="-Z";
	 }
         if (strcmp("--repl", argv[i])==0) {
            rodsArgs->regRepl=True;
            argv[i]="-Z";
         }
         if (strcmp("--sql", argv[i])==0) {
            rodsArgs->sql=True;
            argv[i]="-Z";
         }
         if (strcmp("--lfrestart", argv[i])==0) {
            rodsArgs->lfrestart=True;
            argv[i]="-Z";
            if (i + 2 < argc) {
	       if (*argv[i+1] == '-') {
                   rodsLog (LOG_ERROR,
                    "--lfrestart option needs an iput file");
		    return USER_INPUT_OPTION_ERR;
               }
               rodsArgs->lfrestartFileString=strdup(argv[i+1]);
               argv[i+1]="-Z";
            }
         }
         if (strcmp("--orphan", argv[i])==0) {
            rodsArgs->orphan=True;
            argv[i]="-Z";
         }
         if (strcmp("--age", argv[i])==0) {  /* also -Y */
            rodsArgs->age=True;
            argv[i]="-Z";
            if (i + 2 < argc) {
               if (*argv[i+1] == '-') {
                   rodsLog (LOG_ERROR,
                    "--age option needs an iput file");
                    return USER_INPUT_OPTION_ERR;
               }
               rodsArgs->agevalue=atoi(argv[i+1]);
               argv[i+1]="-Z";
            }
         }
         if (strcmp("--dryrun", argv[i])==0) {
            rodsArgs->dryrun=True;
            argv[i]="-Z";
         }
      }
   }

   /* handle the short options */
   if (optString != NULL && *optString != '\0') 
      opts=optString;
   else
      opts=fullOpts;

   while ((opt = getopt(argc, argv, opts)) != (char)EOF) {
      switch(opt) {
      case 'a':
         rodsArgs->all=True;
         break;
       case 'A':
         rodsArgs->accessControl=True;
         break;
      case 'b':
         rodsArgs->bulk=True;
         break;
      case 'B':
         rodsArgs->backupMode=True;
         break;
      case 'c':
         rodsArgs->condition=True;
         rodsArgs->conditionString=optarg;
         break;
      case 'C':
         rodsArgs->collection=True;
         rodsArgs->collectionString=optarg;
         break;
      case 'd':
         rodsArgs->dataObjects=True;
         break;
      case 'D':
         rodsArgs->dataType=True;
         rodsArgs->dataTypeString=optarg;
         break;
      case 'e':
         rodsArgs->echo=True;
         break;
      case 'f':
         rodsArgs->force=True;
         break;
      case 'F':
         rodsArgs->file=True;
         rodsArgs->fileString=optarg;
         break;
      case 'g':
         rodsArgs->global=True;
         break;
      case 'G':
         rodsArgs->rescGroup=True;
         rodsArgs->rescGroupString=optarg;
         break;
      case 'H':
         rodsArgs->hostAddr=True;
         rodsArgs->hostAddrString=optarg;
         break;
      case 'h':
         rodsArgs->help=True;
         break;
      case 'i':
         rodsArgs->input=True;
         break;
      case 'I':
         rodsArgs->redirectConn=True;	/* connect directly to resc server */
         break;
      case 'k':
         rodsArgs->checksum=True;
         break;
      case 'K':
         rodsArgs->verifyChecksum=True;
         break;
      case 'l':
         rodsArgs->longOption=True;
         break;
      case 'L':
         rodsArgs->longOption=True;
         rodsArgs->veryLongOption=True;
         break;
      case 'm':
         rodsArgs->mountCollection=True;
         rodsArgs->mountType = optarg;
         break;
      case 'M':
         rodsArgs->admin=True;
         break;
      case 'n':
	 rodsArgs->replNum=True;
	 rodsArgs->replNumValue = optarg;
         break;
      case 'N':
         rodsArgs->number=True;
         rodsArgs->numberValue = atoi(optarg);
         break;
      case 'o':
         rodsArgs->option=True;
         rodsArgs->optionString=optarg;
         break;
      case 'p':
         rodsArgs->physicalPath=True;
         rodsArgs->physicalPathString=optarg;
         break;
      case 'P':
         rodsArgs->logicalPath=True;
         rodsArgs->logicalPathString=optarg;
         rodsArgs->progressFlag = True;
         break;
      case 'q':
         rodsArgs->query=True;
         break;
      case 'Q':
         rodsArgs->rbudp=True;
         break;
      case 'r':
         rodsArgs->recursive=True;
         break;
      case 'R':
         rodsArgs->resource=True;
         rodsArgs->resourceString=optarg;
         break;
      case 's':
         rodsArgs->sizeFlag=True;
	 if (optarg != NULL) {
	    /* irsync uses sizeFlag for sync but has no size value */  
             rodsArgs->size = strtoll (optarg, 0, 0);
	 }
         break;
      case 'S':
         rodsArgs->srcResc=True;
	 rodsArgs->srcRescString=optarg;
         break;
      case 't':
         rodsArgs->ticket=True;
         rodsArgs->ticketString=optarg;
         break;
      case 'T':
         rodsArgs->reconnect=True;
         break;
      case 'u':
         rodsArgs->user=True;
         rodsArgs->userString=optarg;
         break;
      case 'U':
         rodsArgs->unmount=True;
         break;
      case 'v':
         rodsArgs->verbose=True;
         break;
      case 'V':
	 VCount++;
         rodsArgs->verbose=True;  /* also set verbose */
         rodsArgs->veryVerbose=True;
	 if (VCount <=1) {
	    rodsLogLevel(LOG_NOTICE);
	 }
	 else {
	    rodsLogLevel(LOG_DEBUG); /* multiple V's is for DEBUG level */
	 }
         break;
      case 'z':
         rodsArgs->zone=True;
         rodsArgs->zoneName=optarg;
         break;
      case 'Z':
	 /* noop; Z is placeholder for the long options */
         break;

      /* The following are also -- options */
      case 'x':
	 rodsArgs->extract=True;
         break;
      case 'X':
         rodsArgs->restart=True;
         rodsArgs->restartFileString=optarg;
         break;
      case 'W':
	 rodsArgs->version=True;
         break;
      case 'Y':
	 rodsArgs->retries=True;
	 rodsArgs->retriesValue= atoi(optarg);
         break;
      default:
	 rodsLogError (LOG_ERROR, USER_INPUT_OPTION_ERR,
		       "parseCmdLineOpt: Option -%c not supported",
		       optopt);
	 return (USER_INPUT_OPTION_ERR);
      }
   }
   rodsArgs->optind = optind;
   return(0);
}

