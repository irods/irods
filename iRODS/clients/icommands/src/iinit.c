/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rods.h"
#include "parseCommandLine.h"
#include "rcMisc.h"

void usage (char *prog);

#define TTYBUF_LEN 100
#define UPDATE_TEXT_LEN NAME_LEN*10

/* 
 Attempt to make the ~/.irods directory in case it doesn't exist (may
 be needed to write the .irodsA file and perhaps the .irodsEnv file).
 */
int
mkrodsdir() {
   char dirName[NAME_LEN];
   int status, mode;
   char *getVar;
#ifdef windows_platform
   getVar = iRODSNt_gethome();
#else
   getVar = getenv("HOME");
#endif
   rstrcpy(dirName, getVar, NAME_LEN);
   rstrcat(dirName, "/.irods", NAME_LEN);
   mode = 0700;
#ifdef _WIN32
   status = iRODSNt_mkdir (dirName, mode);
#else
   status = mkdir (dirName, mode);
#endif
   return(0); /* no error messages as it normally fails */
}

void
printUpdateMsg()
{
#if 0
   printf("One or more fields in your iRODS environment file (.irodsEnv) are\n");
   printf("missing.  This program will ask you for them and if they work (the\n");
   printf("connection/login succeeds), it will update (or create if needed) your\n");
   printf("irods environment file with those values for use by the i-commands.\n");
   printf("You can also edit the environment file with a text editor.\n");
#endif
   printf("One or more fields in your iRODS environment file (.irodsEnv) are\n");
   printf("missing; please enter them.\n");
}
int
main(int argc, char **argv)
{
    int i, ix, status;
    int echoFlag=0;
    char *password;
    rodsEnv myEnv;
    rcComm_t *Conn;
    rErrMsg_t errMsg;
    rodsArguments_t myRodsArgs;
    int useGsi=0;
    int doPassword;
    char ttybuf[TTYBUF_LEN];
    int doingEnvFileUpdate=0;
    char updateText[UPDATE_TEXT_LEN]="";

    status = parseCmdLineOpt(argc, argv, "ehvVl", 0, &myRodsArgs);
    if (status != 0) {
       printf("Use -h for help.\n");
       exit(1);
    }

    if (myRodsArgs.echo==True) {
       echoFlag=1;
    }
    if (myRodsArgs.help==True) {
       usage(argv[0]);
       exit(0);
    }

    if (myRodsArgs.longOption==True) {
	rodsLogLevel(LOG_NOTICE);
    }
 
    ix = myRodsArgs.optind;

    password="";
    if (ix < argc) {
       password = argv[ix];
    }

    status = getRodsEnv (&myEnv);
    if (status < 0) {
       rodsLog (LOG_ERROR, "main: getRodsEnv error. status = %d",
		status);
       exit (1);
    }

    if (myRodsArgs.longOption==True) {
	/* just list the env */
	exit (0);
    }

/*
 Create ~/.irods/ if it does not exist
 */
    mkrodsdir(); 

 /*
    Check on the key Environment values, prompt and save
    them if not already available.
  */
    if (myEnv.rodsHost == NULL || strlen(myEnv.rodsHost)==0) {
       int i;
       if (doingEnvFileUpdate==0) {
	  doingEnvFileUpdate=1;
	  printUpdateMsg();
       }
       printf("Enter the host name (DNS) of the server to connect to:");
       memset(ttybuf, 0, TTYBUF_LEN);
       fgets(ttybuf, TTYBUF_LEN, stdin);
       rstrcat(updateText,"irodsHost ", UPDATE_TEXT_LEN);
       rstrcat(updateText,ttybuf, UPDATE_TEXT_LEN);
       i = strlen(ttybuf);
       if (i>0) ttybuf[i-1]='\0'; /* chop off trailing \n */
       strncpy(myEnv.rodsHost, ttybuf, NAME_LEN);
    }
    if (myEnv.rodsPort == 0) {
       int i;
       if (doingEnvFileUpdate==0) {
	  doingEnvFileUpdate=1;
	  printUpdateMsg();
       }
       printf("Enter the port number:");
       memset(ttybuf, 0, TTYBUF_LEN);
       fgets(ttybuf, TTYBUF_LEN, stdin);
       rstrcat(updateText,"irodsPort ", UPDATE_TEXT_LEN);
       rstrcat(updateText,ttybuf, UPDATE_TEXT_LEN);
       i = strlen(ttybuf);
       if (i>0) ttybuf[i-1]='\0';
       myEnv.rodsPort = atoi(ttybuf);
    }
    if (myEnv.rodsUserName == NULL || strlen(myEnv.rodsUserName)==0) {
       int i;
       if (doingEnvFileUpdate==0) {
	  doingEnvFileUpdate=1;
	  printUpdateMsg();
       }
       printf("Enter your irods user name:");
       memset(ttybuf, 0, TTYBUF_LEN);
       fgets(ttybuf, TTYBUF_LEN, stdin);
       rstrcat(updateText,"irodsUserName ", UPDATE_TEXT_LEN);
       rstrcat(updateText,ttybuf, UPDATE_TEXT_LEN);
       i = strlen(ttybuf);
       if (i>0) ttybuf[i-1]='\0';
       strncpy(myEnv.rodsUserName, ttybuf, NAME_LEN);
    }
    if (myEnv.rodsZone == NULL || strlen(myEnv.rodsZone)==0) {
       int i;
       if (doingEnvFileUpdate==0) {
	  doingEnvFileUpdate=1;
	  printUpdateMsg();
       }
       printf("Enter your irods zone:");
       memset(ttybuf, 0, TTYBUF_LEN);
       fgets(ttybuf, TTYBUF_LEN, stdin);
       rstrcat(updateText,"irodsZone ", UPDATE_TEXT_LEN);
       rstrcat(updateText,ttybuf, UPDATE_TEXT_LEN);
       i = strlen(ttybuf);
       if (i>0) ttybuf[i-1]='\0';
       strncpy(myEnv.rodsZone, ttybuf, NAME_LEN);
    }
    if (doingEnvFileUpdate) {
       printf("Those values will be added to your environment file (for use by\n");
       printf("other i-commands) if the login succeeds.\n\n");
    }

    /*
      Now, get the password
     */
    doPassword=1;
#if defined(GSI_AUTH)
    if (strncmp("GSI",myEnv.rodsAuthScheme,3)==0) {
       useGsi=1;
       doPassword=0;
    }
#endif

    if (strcmp(myEnv.rodsUserName, ANONYMOUS_USER)==0) {
       doPassword=0;
    }
    if (useGsi==1) {
       printf("Using GSI, attempting connection/authentication\n");
    }
    if (doPassword==1) {
       if (myRodsArgs.verbose==True) {
	  i = obfSavePw(echoFlag, 1, 1, password);
       }
       else {
	  i = obfSavePw(echoFlag, 0, 0, password);
       }

       if (i != 0) {
	  rodsLogError(LOG_ERROR, i, "Save Password failure");
	  exit(1);
       }
    }

    /* Connect... */ 
    Conn = rcConnect (myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
      myEnv.rodsZone, 0, &errMsg);
    if (Conn == NULL) {
       rodsLog(LOG_ERROR, 
		    "Saved password, but failed to connect to server %s",
	       myEnv.rodsHost);
       exit(2);
    }

    /* and check that the user/password is OK */
    status = clientLogin(Conn);
    if (status != 0) {
       rcDisconnect(Conn);
       exit (7);
    }

    rcDisconnect(Conn);

    /* Save updates to .irodsEnv. */
    if (doingEnvFileUpdate==1) {
       appendRodsEnv(updateText);
    }

    exit (0);
}


void usage (char *prog)
{
  printf("Creates a file containing your iRODS password in a scrambled form,\n");
  printf("to be used automatically by the icommands.\n");
  printf("Usage: %s [-ehvVl]\n", prog);
  printf(" -e  echo the password as you enter it (normally there is no echo)\n");
  printf(" -l  list the iRODS environment variables (only)\n");
  printf(" -v  verbose\n");
  printf(" -V  Very verbose\n");
  printf(" -h  this help\n");
  printReleaseInfo("iinit");
}

