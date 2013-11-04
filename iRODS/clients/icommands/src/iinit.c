/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
// =-=-=-=-=-=-=-
// irods includes
#include "rods.h"
#include "parseCommandLine.h"
#include "rcMisc.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_native_auth_object.h"
#include "eirods_pam_auth_object.h"
#include "eirods_kvp_string_parser.h"
#include "eirods_auth_constants.h"

#include<iostream>
void usage (char *prog);
void usageTTL ();

#define TTYBUF_LEN 100
#define UPDATE_TEXT_LEN NAME_LEN*10

/* 
 Attempt to make the ~/.irods directory in case it doesn't exist (may
 be needed to write the .irodsA file and perhaps the .irodsEnv file).
 */
int
mkrodsdir() {
   char dirName[NAME_LEN];
   int mode;
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
   iRODSNt_mkdir (dirName, mode);
#else
   mkdir (dirName, mode);
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
    int ttl=0;

    status = parseCmdLineOpt(argc, argv, "ehvVlZ", 1, &myRodsArgs);
    if (status != 0) {
       printf("Use -h for help.\n");
       exit(1);
    }

    if (myRodsArgs.echo==True) {
       echoFlag=1;
    }
    
    if (myRodsArgs.help==True && myRodsArgs.ttl==True) {
        usageTTL();
        exit(0);
    }

    if (myRodsArgs.help==True) {
       usage(argv[0]);
       exit(0);
    }

    status = getRodsEnv (&myEnv);
    if (status < 0) {
       rodsLog (LOG_ERROR, "main: getRodsEnv error. status = %d",
		status);
       exit (1);
    }

    if (myRodsArgs.longOption==True) {
	    rodsLogLevel(LOG_NOTICE);
    }

    if (myRodsArgs.ttl==True) {
        ttl = myRodsArgs.ttlValue;
        if (ttl < 1) {
            printf("Time To Live value needs to be a positive integer\n");
            exit(1);
        }
        if (strncmp("PAM",myEnv.rodsAuthScheme,3)==0 ||
            strncmp("pam",myEnv.rodsAuthScheme,3)==0) {
        }
        else {
            printf("Time-To-Live only applies when using PAM authentication\n");
            exit(1);
        }
    }

    ix = myRodsArgs.optind;

    password="";
    if (ix < argc) {
       password = argv[ix];
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


    // =-=-=-=-=-=-=-
    // ensure scheme is lower case for comparison
    std::string lower_scheme = myEnv.rodsAuthScheme;;
    std::transform( 
       lower_scheme.begin(), 
       lower_scheme.end(), 
       lower_scheme.begin(), 
       ::tolower );

    if( eirods::AUTH_PAM_SCHEME == lower_scheme ) {
       doPassword=0;
    }

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

    // =-=-=-=-=-=-=-
    // PAM auth gets special consideration, and also includes an
    // auth by the usual convention
    bool pam_flg = false;
    if( eirods::AUTH_PAM_SCHEME == lower_scheme ) {
        // =-=-=-=-=-=-=-
        // set a flag stating that we have done pam and the auth
        // scheme needs overriden
        pam_flg = true;

        // =-=-=-=-=-=-=-
        // build a context string which includes the ttl and password
        std::stringstream ttl_str;  ttl_str << ttl;
        std::string context = eirods::AUTH_TTL_KEY      +
                              eirods::kvp_association() + 
                              ttl_str.str()             +
                              eirods::kvp_delimiter()   +
                              eirods::AUTH_PASSWORD_KEY +
                              eirods::kvp_association() + 
                              password;
        // =-=-=-=-=-=-=-
        // pass the context with the ttl as well as an override which 
        // demands the pam authentication plugin
        status = clientLogin( Conn, context.c_str(), eirods::AUTH_PAM_SCHEME.c_str() );
        if( status != 0 ) {
            exit(8);
        }

        // =-=-=-=-=-=-=-
        // if this succeeded, do the regular login below to check 
        // that the generated password works properly.  
    } // if pam
        
    // =-=-=-=-=-=-=-
    // since we might be using PAM
    // and check that the user/password is OK 
    const char* auth_scheme = ( pam_flg )                        ? 
                              eirods::AUTH_NATIVE_SCHEME.c_str() : 
                              myEnv.rodsAuthScheme;
    status = clientLogin( Conn, 0, auth_scheme );
    if (status != 0) {
        rcDisconnect( Conn );
        exit (7);
    }

    printErrorStack(Conn->rError);
    rcDisconnect(Conn);

    /* Save updates to .irodsEnv. */
    if (doingEnvFileUpdate==1) {
        appendRodsEnv(updateText);
    }

    exit (0);

} // main


void usage( char *prog ) {
    printf("Creates a file containing your iRODS password in a scrambled form,\n");
    printf("to be used automatically by the icommands.\n");
    printf("Usage: %s [-ehvVl]\n", prog);
    printf(" -e  echo the password as you enter it (normally there is no echo)\n");
    printf(" -l  list the iRODS environment variables (only)\n");
    printf(" -v  verbose\n");
    printf(" -V  Very verbose\n");
    printf("--ttl ttl  set the password Time To Live (specified in hours)\n");
    printf("           Run 'iinit -h --ttl' for more\n");

    printf(" -h  this help\n");
    printReleaseInfo("iinit");
}

void usageTTL() {
    printf("When using regular iRODS passwords you can use --ttl (Time To Live)\n");
    printf("to request a credential (a temporary password) that will be valid\n");
    printf("for only the number of hours you specify (up to a limit set by the\n");
    printf("administrator).  This is more secure, as this temporary password\n");
    printf("(not your permanent one) will be stored in the obfuscated\n");
    printf("credential file (.irodsA) for use by the other i-commands.\n");
    printf("\n");
    printf("When using PAM, iinit always generates a temporary iRODS password\n");
    printf("for use by the other i-commands, using a time-limit set by the\n");
    printf("administrator (usually a few days).  With the --ttl option, you can\n");
    printf("specify how long this derived password will be valid, within the\n");
    printf("limits set by the administrator.\n");
}

