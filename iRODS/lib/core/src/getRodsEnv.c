/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
 This routine sets up the rodsEnv structure using the contents of the
 .irodsEnv file and possibly some environment variables.  For each of
 the .irodsEnv items, if an environment variable with the same name
 exists, it overrides the possible .irodsEnv item.  This is called by
 the various Rods commands and the agent.

 This routine also fills in irodsHome and irodsCwd if they are not
 otherwise defined, and if values needed to create them are available.

 The '#' character indicates a comment line.  Items may be enclosed in
 quotes, but do not need to be.  One or more spaces, or a '=', will
 preceed the item values.

 The items are defined in the rodsEnv struct.  The item names in the
 .irodsEnv file need to match the item names of the struct.

 If an error occurs, a message may logged or displayed but the
 structure is filled with whatever values are available.

 There is also an 'appendRodsEnv' function to add text to
 the env file, either creating it or appending to it.
*/

#include "rods.h"

#include "getRodsEnv.h"
#include "rodsLog.h"

#ifdef windows_platform
#include "irodsntutil.h"
#endif

#define BUF_LEN 100
#define LARGE_BUF_LEN MAX_NAME_LEN+20

#define RODS_ENV_FILE "/.irods/.irodsEnv"  /* under the HOME directory */

extern int ProcessType;
extern char *rstrcpy(char *dst, char *src, int len);

char *findNextTokenAndTerm(char *inPtr);

int getRodsEnvFromFile(char *fileName, rodsEnv *rodsEnvArg, int errorLevel);
int getRodsEnvFromEnv(rodsEnv *rodsEnvArg);
int createRodsEnvDefaults(rodsEnv *rodsEnvArg);

static char configFileName[LONG_NAME_LEN];
static char authFileName[LONG_NAME_LEN]="";
static int irodsEnvFile=0;

char *
getRodsEnvFileName() {
   return(configFileName);
}

/* Return the auth filename, if any */
/* Used by obf routines so that the env struct doesn't have to be passed 
   up and down the calling chain */
char *
getRodsEnvAuthFileName() {
   return(authFileName);
}

/* convert either an integer value or a name matching the defines, to
   a value for the Logging Level */
int
convertLogLevel(char *inputStr) {
   int i;
   i = atoi(inputStr);
   if (i > 0 && i <= LOG_SQL) return(i);
   if (strcmp(inputStr, "LOG_SQL")==0) return(LOG_SQL);
   if (strcmp(inputStr, "LOG_SYS_FATAL")==0) return(LOG_SYS_FATAL);
   if (strcmp(inputStr, "LOG_SYS_WARNING")==0) return(LOG_SYS_WARNING);
   if (strcmp(inputStr, "LOG_ERROR")==0) return(LOG_ERROR);
   if (strcmp(inputStr, "LOG_NOTICE")==0) return(LOG_NOTICE);
   if (strcmp(inputStr, "LOG_DEBUG")==0) return(LOG_DEBUG);
   if (strcmp(inputStr, "LOG_DEBUG3")==0) return(LOG_DEBUG3);
   if (strcmp(inputStr, "LOG_DEBUG2")==0) return(LOG_DEBUG2);
   if (strcmp(inputStr, "LOG_DEBUG1")==0) return(LOG_DEBUG1);
   return(0);
}

int getRodsEnv(rodsEnv *rodsEnvArg) {
   char *getVar = NULL;
   int status;
   int ppid;
   char ppidStr[BUF_LEN];

#ifdef windows_platform
   /* we handle env file differently in Windows */
   if(ProcessType != CLIENT_PT)
   {
	   char rodsEnvFilenameWP[1024];
	   char *tmpstr1;
	   int t;
	   tmpstr1 = iRODSNtGetServerConfigPath();
	   sprintf(rodsEnvFilenameWP, "%s\\irodsEnv.txt", tmpstr1);
	   t = getRodsEnvFromFile(rodsEnvFilenameWP, rodsEnvArg, LOG_DEBUG);
	   if(t < 0)
		   return t;
	   return createRodsEnvDefaults(rodsEnvArg);
   }

   getVar = iRODSNt_gethome();
#else
#ifdef UNI_CODE
#include <locale.h>
   setlocale (LC_ALL, "");
#endif
   getVar = getenv("HOME");
#endif
   if (getVar==NULL) {
      rstrcpy(configFileName,"", LONG_NAME_LEN);
   }
   else {
      rstrcpy(configFileName,getVar, LONG_NAME_LEN);
   }
   rstrcat(configFileName, RODS_ENV_FILE, LONG_NAME_LEN);

   getVar = getenv("irodsEnvFile");
   if (getVar!=NULL && *getVar!='\0') {
#ifdef windows_platform
       getVar = strdup(getenv("irodsEnvFile"));
#endif
      rstrcpy(configFileName, findNextTokenAndTerm(getVar), LONG_NAME_LEN);
      rodsLog(LOG_NOTICE,
	      "environment variable set, irodsEnvFile=%s",
	      configFileName);
      irodsEnvFile=1; /* indicate that this was set */
   }

   memset(rodsEnvArg,0,sizeof(rodsEnv));

   /* RAJA CHANGED Feb 1, 207  from LOG_NOTICE to LOG_DEBUG */
   status = getRodsEnvFromFile(configFileName, rodsEnvArg, LOG_DEBUG);
   status = getRodsEnvFromEnv(rodsEnvArg);
   status = createRodsEnvDefaults(rodsEnvArg);

   /* Only client processes will do this, otherwise a lot of errors */
   if (ProcessType == CLIENT_PT) {
#ifdef windows_platform    
	   /* windows only allow one session per user. This is because there is no ppid.*/
       char tmpCfg[LONG_NAME_LEN];
	   sprintf(tmpCfg, "%s.cwd", configFileName);
       strcpy( configFileName, tmpCfg );
#else
      if (irodsEnvFile==0) {
	 /* For normal case, use the ppid as part of the session file name */
	 ppid = getppid();
	 sprintf(ppidStr, ".%d", ppid);
      }
      else {
	 /* When irodsEnvFile set, use a fixed string so that all the
            children processes (with inherited env) will find the same
            one.  This is useful when running scripts. */
	 sprintf(ppidStr, ".%s", "cwd");
      }
	  rstrcat(configFileName, ppidStr, LONG_NAME_LEN);
#endif
      status = getRodsEnvFromFile(configFileName, rodsEnvArg, LOG_DEBUG);
   }

#ifdef windows_platform
   if(getVar != NULL)
	   free(getVar);
#endif

   return(0);
}

int getRodsEnvFromFile(char *fileName, rodsEnv *rodsEnvArg, int errorLevel) {
   FILE *file;
   char buf[LARGE_BUF_LEN];
   char *fchar;
   int envFileFound;
   char *key;
   int msgLevel;

   msgLevel = LOG_NOTICE;
   if (ProcessType == AGENT_PT) {
      /* For an Agent process, make the LOG_NOTICE messages an even lower
         priority (LOG_DEBUG) so that by default the log file will be
         shorter.  The Server will log the environment values at startup
         but these will almost always be redundant for the Agent.  */
      msgLevel = LOG_DEBUG;  
   }

/*
   Read and process the env file
*/
   envFileFound=0;
#ifdef windows_platform
   file = iRODSNt_fopen(fileName, "r");
#else
   file = fopen(fileName, "r");
#endif
   if (file != NULL) {
      envFileFound=1;
      buf[LARGE_BUF_LEN-1]='\0';
      fchar = fgets(buf, LARGE_BUF_LEN-1, file);
      for(;fchar!='\0';) {
	 if (buf[0]=='#' || buf[0]=='/') {
	    buf[0]='\0'; /* Comment line, ignore */
	 }
	 key=strstr(buf, "irodsUserName");
	 if (key != NULL) {
	    rstrcpy(rodsEnvArg->rodsUserName, findNextTokenAndTerm(key+13),
		    NAME_LEN);
	    rodsLog(msgLevel, "irodsUserName=%s",rodsEnvArg->rodsUserName);
	 }
	 key=strstr(buf, "irodsHost");
	 if (key != NULL) {
	    rstrcpy(rodsEnvArg->rodsHost, findNextTokenAndTerm(key+9),
		    NAME_LEN);
	    rodsLog(msgLevel, "irodsHost=%s",rodsEnvArg->rodsHost);
	 }
         /* add xmsgHost. mw */
	 key=strstr(buf, "xmsgHost");
         if (key != NULL) {
            rstrcpy(rodsEnvArg->xmsgHost, findNextTokenAndTerm(key+9),
                    NAME_LEN);
            rodsLog(msgLevel, "xmsgHost=%s",rodsEnvArg->xmsgHost);
         }
	 key=strstr(buf, "irodsPort");
	 if (key != NULL) {
	    rodsEnvArg->rodsPort=atoi(findNextTokenAndTerm(key+9));
	    rodsLog(msgLevel, "irodsPort=%d",rodsEnvArg->rodsPort);
	 }
         /* add xmsgPort. mw */
         key=strstr(buf, "xmsgPort");
         if (key != NULL) {
            rodsEnvArg->xmsgPort=atoi(findNextTokenAndTerm(key+8));
            rodsLog(msgLevel, "xmsgPort=%d",rodsEnvArg->xmsgPort);
         }
	 key=strstr(buf, "irodsHome");
	 if (key != NULL) {
	    rstrcpy(rodsEnvArg->rodsHome, findNextTokenAndTerm(key+9),
		    MAX_NAME_LEN);
	    rodsLog(msgLevel, "irodsHome=%s",rodsEnvArg->rodsHome);
	 }
	 key=strstr(buf, "irodsCwd");
	 if (key != NULL) {
	    rstrcpy(rodsEnvArg->rodsCwd, findNextTokenAndTerm(key+8),
		    MAX_NAME_LEN);
	    rodsLog(msgLevel, "irodsCwd=%s",rodsEnvArg->rodsCwd);
	 }
	 key=strstr(buf, "irodsAuthScheme");
	 if (key != NULL) {
	    static char tmpStr1[120];
	    char *getVar;

	    rstrcpy(rodsEnvArg->rodsAuthScheme, findNextTokenAndTerm(key+15),
		    LONG_NAME_LEN);
	    rodsLog(msgLevel, "irodsAuthScheme=%s",
		    rodsEnvArg->rodsAuthScheme);
	    /* Also put it into the environment for easy access,
               unless there already is one (which should be used instead) */
	    getVar = getenv("irodsAuthScheme");
	    if (getVar==NULL) {
	       snprintf(tmpStr1,100,"irodsAuthScheme=%s",
			rodsEnvArg->rodsAuthScheme);
	       putenv(tmpStr1);
	    }
	 }
	 key=strstr(buf, "irodsDefResource");
	 if (key != NULL) {
	    rstrcpy(rodsEnvArg->rodsDefResource,findNextTokenAndTerm(key+16),
		    LONG_NAME_LEN);
	    rodsLog(msgLevel, "irodsDefResource=%s",
		    rodsEnvArg->rodsDefResource);
	 }
	 key=strstr(buf, "irodsZone");
	 if (key != NULL) {
	    rstrcpy(rodsEnvArg->rodsZone,findNextTokenAndTerm(key+9),
		    LONG_NAME_LEN);
	    rodsLog(msgLevel, "irodsZone=%s",
		    rodsEnvArg->rodsZone);
	 }
	 key=strstr(buf, "irodsServerDn");
	 if (key != NULL) {
	    char *myStr;
	    char *getVar;
	    myStr = (char *)malloc(strlen(buf));
	    rstrcpy(myStr, findNextTokenAndTerm(key+13),
		    LONG_NAME_LEN);
	    rodsEnvArg->rodsServerDn=myStr;
	    rodsLog(msgLevel, "irodsServerDn=%s",
		    rodsEnvArg->rodsServerDn);
	    /* Also put it into the environment for easy access,
               unless there already is one (which should be used instead) */
	    getVar = getenv("irodsServerDn");
	    if (getVar==NULL) {
	       char *tmpStr2;
	       int tmpLen;
	       tmpLen = strlen(myStr)+40;
	       tmpStr2 = (char *)malloc(tmpLen);
	       snprintf(tmpStr2,tmpLen,"irodsServerDn=%s", 
			rodsEnvArg->rodsServerDn);
	       putenv(tmpStr2);
		   free( tmpStr2 ); // JMC cppcheck - leak
	    }
	 }
	 key=strstr(buf, "irodsLogLevel");
	 if (key != NULL) {
	    char *levelStr;
	    levelStr = findNextTokenAndTerm(key+13);
	    rodsEnvArg->rodsLogLevel=convertLogLevel(levelStr);
	    if (rodsEnvArg->rodsLogLevel) {
	       rodsLogLevel(rodsEnvArg->rodsLogLevel); /* process it */
	    }
	    rodsLog(msgLevel,
	      "environment variable set, irodsLogLevel(input)=%s, value=%d",
	      levelStr, rodsEnvArg->rodsLogLevel);
	 }
	 key=strstr(buf, "irodsAuthFileName");
	 if (key != NULL) {
	    rstrcpy(rodsEnvArg->rodsAuthFileName,
		      findNextTokenAndTerm(key+17), LONG_NAME_LEN);
	    rodsLog(msgLevel, "irodsAuthFileName=%s",
		    rodsEnvArg->rodsAuthFileName);
	    rstrcpy(authFileName, rodsEnvArg->rodsAuthFileName, LONG_NAME_LEN);
	 }
	 key=strstr(buf, "irodsDebug");
	 if (key != NULL) {
	    rstrcpy(rodsEnvArg->rodsDebug, findNextTokenAndTerm(key+10),
		    NAME_LEN);
	    rodsLog(msgLevel, "irodsDebug=%s",rodsEnvArg->rodsDebug);
	 }
	 fchar = fgets(buf, LARGE_BUF_LEN-1, file);
      }
      fclose (file);
   }
   else {
      rodsLog(errorLevel,
	      "getRodsEnv() could not open environment file %s",
	      fileName);
#ifdef windows_platform
	  return -1;
#endif
   }
   return(0);
}

int
getRodsEnvFromEnv(rodsEnv *rodsEnvArg) {

/*
   Check for and process the environment variables
*/
   char *getVar;

   getVar = getenv("irodsUserName");
   if (getVar!=NULL) {
      rstrcpy(rodsEnvArg->rodsUserName, findNextTokenAndTerm(getVar),NAME_LEN);
      rodsLog(LOG_NOTICE,
	      "environment variable set, irodsUserName=%s",
	      rodsEnvArg->rodsUserName);
   }
   getVar = getenv("irodsHost");
   if (getVar!=NULL) {
      rstrcpy(rodsEnvArg->rodsHost, findNextTokenAndTerm(getVar),NAME_LEN);
      rodsLog(LOG_NOTICE,
	      "environment variable set, irodsHost=%s",
	      rodsEnvArg->rodsHost);
   }
   /* add xmsgHost. mw */
   getVar = getenv("xmsgHost");
   if (getVar!=NULL) {
      rstrcpy(rodsEnvArg->xmsgHost, findNextTokenAndTerm(getVar),NAME_LEN);
      rodsLog(LOG_NOTICE,
              "environment variable set, xmsgHost=%s",
              rodsEnvArg->xmsgHost);
   }

   getVar = getenv("irodsPort");
   if (getVar!=NULL) {
      rodsEnvArg->rodsPort=atoi(findNextTokenAndTerm(getVar));
      rodsLog(LOG_NOTICE,
	      "environment variable set, irodsPort=%d",
	      rodsEnvArg->rodsPort);
   }

   /* add xmsgPort. mw */

   getVar = getenv("xmsgPort");
   if (getVar!=NULL) {
      rodsEnvArg->xmsgPort=atoi(findNextTokenAndTerm(getVar));
      rodsLog(LOG_NOTICE,
              "environment variable set, xmsgPort=%d",
              rodsEnvArg->xmsgPort);
   }

   getVar = getenv("irodsHome");
   if (getVar!=NULL) {
      rstrcpy(rodsEnvArg->rodsHome, findNextTokenAndTerm(getVar),MAX_NAME_LEN);
      rodsLog(LOG_NOTICE,
	      "environment variable set, irodsHome=%s",
	      rodsEnvArg->rodsHome);
   }
   getVar = getenv("irodsCwd");
   if (getVar!=NULL) {
      rstrcpy(rodsEnvArg->rodsCwd, findNextTokenAndTerm(getVar),MAX_NAME_LEN);
      rodsLog(LOG_NOTICE,
	      "environment variable set, irodsCwd=%s",
	      rodsEnvArg->rodsCwd);
   }
   getVar = getenv("irodsAuthScheme");
   if (getVar!=NULL) {
      rstrcpy(rodsEnvArg->rodsAuthScheme, findNextTokenAndTerm(getVar),
	      LONG_NAME_LEN);
      rodsLog(LOG_NOTICE,
	      "environment variable set, irodsAuthScheme=%s",
	      rodsEnvArg->rodsAuthScheme);
   }
   getVar = getenv("irodsDefResource");
   if (getVar!=NULL) {
      rstrcpy(rodsEnvArg->rodsDefResource, findNextTokenAndTerm(getVar),
	     LONG_NAME_LEN);
      rodsLog(LOG_NOTICE,
	      "environment variable set, irodsDefResource=%s",
	      rodsEnvArg->rodsDefResource);
   }
   getVar = getenv("irodsZone");
   if (getVar!=NULL) {
      rstrcpy(rodsEnvArg->rodsZone, findNextTokenAndTerm(getVar), 
	      LONG_NAME_LEN);
      rodsLog(LOG_NOTICE,
	      "environment variable set, irodsZone=%s",
	      rodsEnvArg->rodsZone);
   }
   getVar = getenv("irodsServerDn");
   if (getVar!=NULL) {
      char *myStr;
      myStr = (char *)malloc(strlen(getVar)+10);
      strcpy(myStr, findNextTokenAndTerm(getVar));
      rodsEnvArg->rodsServerDn=myStr;
      rodsLog(LOG_NOTICE, "environment variable set, irodsServerDn=%s",
	      rodsEnvArg->rodsServerDn);
   }
   getVar = getenv("irodsLogLevel");
   if (getVar!=NULL) {
      rodsEnvArg->rodsLogLevel=convertLogLevel(getVar);
      if (rodsEnvArg->rodsLogLevel) {
	 rodsLogLevel(rodsEnvArg->rodsLogLevel); /* go ahead and process it */
      }
      rodsLog(LOG_NOTICE,
	      "environment variable set, irodsLogLevel(input)=%s, value=%d",
	      getVar, rodsEnvArg->rodsLogLevel);
   }
   getVar = getenv("irodsAuthFileName");
   if (getVar!=NULL) {
      rstrcpy(rodsEnvArg->rodsAuthFileName, findNextTokenAndTerm(getVar),
	      LONG_NAME_LEN);
      rodsLog(LOG_NOTICE,
	      "environment variable set, irodsAuthFileName=%s",
	      rodsEnvArg->rodsAuthFileName);
      rstrcpy(authFileName, rodsEnvArg->rodsAuthFileName, LONG_NAME_LEN);
   }
   getVar = getenv("irodsDebug");
   if (getVar!=NULL) {
      rstrcpy(rodsEnvArg->rodsDebug, findNextTokenAndTerm(getVar),NAME_LEN);
      rodsLog(LOG_NOTICE,
	      "environment variable set, irodsDebug=%s",
	      rodsEnvArg->rodsDebug);
   }
   return (0);
}

/* build a couple default values from others if appropriate */
int
createRodsEnvDefaults(rodsEnv *rodsEnvArg) {
   if (strlen(rodsEnvArg->rodsHome)==0) {
      if (strlen(rodsEnvArg->rodsUserName)>0 &&
	  strlen(rodsEnvArg->rodsZone)>0) {
	 snprintf(rodsEnvArg->rodsHome,  MAX_NAME_LEN, "/%s/home/%s",
		  rodsEnvArg->rodsZone, rodsEnvArg->rodsUserName);
      }
      rodsLog(LOG_NOTICE, "created irodsHome=%s", rodsEnvArg->rodsHome);
   }
   if (strlen(rodsEnvArg->rodsCwd)==0 &&
       strlen(rodsEnvArg->rodsHome)>0) {
      rstrcpy(rodsEnvArg->rodsCwd, rodsEnvArg->rodsHome, MAX_NAME_LEN);
      rodsLog(LOG_NOTICE, "created irodsCwd=%s", rodsEnvArg->rodsCwd);
   }

   return (0);
}


/*
 find the next delimited token and terminate the string with matching quotes
 */
char *findNextTokenAndTerm(char *inPtr)
{
   char *myPtr;
   char *savePtr;
   char *nextPtr;
   int whiteSpace;
   myPtr = inPtr;
   whiteSpace=1;
   for (;;myPtr++) {
      if (*myPtr==' ' || *myPtr=='=') {
	 continue;
      }
      if (*myPtr=='"' && whiteSpace) {
	 myPtr++;
	 savePtr=myPtr;
	 for (;;) {
	    if (*myPtr=='"') {
	       nextPtr=myPtr+1;
	       if (*nextPtr==' ' || *nextPtr=='\n'  || *nextPtr=='\0') {
		  /* imbedded "s are OK */
		  *myPtr='\0';
		  return(savePtr);
	       }
	    }
	    if (*myPtr=='\n') *myPtr='\0';
	    if (*myPtr=='\0') {
	       /* terminated without a corresponding ", so backup and
                  put the starting one back */
	       savePtr--;
	       *savePtr='"';
	       return(savePtr);
	    }
	    myPtr++;
	 }
      }
      if (*myPtr=='\'' && whiteSpace) {
	 myPtr++;
	 savePtr=myPtr;
	 for (;;) {
	    if (*myPtr=='\'') {
	       nextPtr=myPtr+1;
	       if (*nextPtr==' ' || *nextPtr=='\n'  || *nextPtr=='\0') {
		  /* imbedded 's are OK */
		  *myPtr='\0';
		  return(savePtr);
	       }
	    }
	    if (*myPtr=='\n') *myPtr='\0';
	    if (*myPtr=='\0') {
	       /* terminated without a corresponding ", so backup and
                  put the starting one back */
	       savePtr--;
	       *savePtr='\'';
	       return(savePtr);
	    }
	    myPtr++;
	 }
      }
      if (whiteSpace) savePtr=myPtr;
      whiteSpace=0;
      if (*myPtr=='\n') *myPtr='\0';
	  if (*myPtr=='\r') *myPtr='\0';
      if (*myPtr=='\0') {
	 return(savePtr);
      }
   }
}

int appendRodsEnv(char *appendText) {
   FILE *fptr;
   char *getVar = NULL;

#ifdef windows_platform
   getVar = iRODSNt_gethome();
#else
   getVar = getenv("HOME");
#endif
   if (getVar==NULL) {
      rstrcpy(configFileName,"", LONG_NAME_LEN);
   }
   else {
      rstrcpy(configFileName,getVar, LONG_NAME_LEN);
   }
   rstrcat(configFileName, RODS_ENV_FILE, LONG_NAME_LEN);

   getVar = getenv("irodsEnvFile");
   if (getVar!=NULL && *getVar!='\0') {
#ifdef windows_platform
       getVar = strdup(getenv("irodsEnvFile"));
#endif
      rstrcpy(configFileName, findNextTokenAndTerm(getVar), LONG_NAME_LEN);
   }
   fptr = fopen (configFileName, "a");
   if (fptr == NULL) {
      rodsLog(LOG_ERROR,
	      "appendRodsEnv: cannot create file %s",
	      configFileName);
      return(0);
   }
   fputs(appendText, fptr);
   fclose (fptr);
   return(0);
}
