/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* 
  This is an interface to the database objects.
*/

#include "rods.h"
#include "rodsClient.h"

#define MAX_SQL 4000
#define BIG_STR 200

char g_cwd[BIG_STR];
char g_dbor[BIG_STR]="";
int g_force;

int debug=0;

char zoneArgument[MAX_NAME_LEN+2]="";

rcComm_t *Conn;
rodsEnv myEnv;

int lastCommandStatus=0;
int printCount=0;

rodsArguments_t myRodsArgs;


void usage(char *subOpt);

/* 
 Prompt for input and parse into tokens
*/
void
getInput(char *cmdToken[], int maxTokens) {
   int lenstr, i;
   static char ttybuf[BIG_STR];
   int nTokens;
   int tokenFlag; /* 1: start reg, 2: start ", 3: start ' */
   char *cpTokenStart;
   char *stat;

   memset(ttybuf, 0, BIG_STR);
   fputs("idbo>",stdout);
   stat = fgets(ttybuf, BIG_STR, stdin);
   if (stat==0) {
      printf("\n");
      rcDisconnect(Conn);
      if (lastCommandStatus != 0) exit(4);
      exit(0);
   }
   lenstr=strlen(ttybuf);
   for (i=0;i<maxTokens;i++) {
      cmdToken[i]="";
   }
   cpTokenStart = ttybuf;
   nTokens=0;
   tokenFlag=0;
   for (i=0;i<lenstr;i++) {
      if (ttybuf[i]=='\n') {
	 ttybuf[i]='\0';
	 cmdToken[nTokens++]=cpTokenStart;
	 return;
      }
      if (tokenFlag==0) {
	 if (ttybuf[i]=='\'') {
	    tokenFlag=3;
	    cpTokenStart++;
	 }
	 else if (ttybuf[i]=='"') {
	    tokenFlag=2;
	    cpTokenStart++;
	 }
	 else if (ttybuf[i]==' ') {
	    cpTokenStart++;
	 }
	 else {
	    tokenFlag=1;
	 }
      }
      else if (tokenFlag == 1) {
	 if (ttybuf[i]==' ') {
	    ttybuf[i]='\0';
	    cmdToken[nTokens++]=cpTokenStart;
	    cpTokenStart = &ttybuf[i+1];
	    tokenFlag=0;
	 }
      }
      else if (tokenFlag == 2) {
	 if (ttybuf[i]=='"') {
	    ttybuf[i]='\0';
	    cmdToken[nTokens++]=cpTokenStart;
	    cpTokenStart = &ttybuf[i+1];
	    tokenFlag=0;
	 }
      }
      else if (tokenFlag == 3) {
	 if (ttybuf[i]=='\'') {
	    ttybuf[i]='\0';
	    cmdToken[nTokens++]=cpTokenStart;
	    cpTokenStart = &ttybuf[i+1];
	    tokenFlag=0;
	 }
      }
   }
}

/*
 This is a copy of the iquest function, used here in idbo to
 show database objects.
 */
int
queryAndShowStrCond(rcComm_t *conn, char *hint, char *format, 
		    char *selectConditionString, int noDistinctFlag,
                    char *zoneArgument, int noPageFlag)
{
/*
  NoDistinctFlag is 1 if the user is requesting 'distinct' to be skipped.
*/

   genQueryInp_t genQueryInp;
   int i;
   genQueryOut_t *genQueryOut = NULL;

   memset (&genQueryInp, 0, sizeof (genQueryInp_t));
   i = fillGenQueryInpFromStrCond(selectConditionString, &genQueryInp);
   if (i < 0)
      return(i);

   if (noDistinctFlag) {
      genQueryInp.options = NO_DISTINCT;
   }

   if (zoneArgument!=0 && zoneArgument[0]!='\0') {
      addKeyVal (&genQueryInp.condInput, ZONE_KW, zoneArgument);
      printf("Zone is %s\n",zoneArgument);
   }

   genQueryInp.maxRows= MAX_SQL_ROWS;
   genQueryInp.continueInx=0;
   i = rcGenQuery (conn, &genQueryInp, &genQueryOut);
   if (i < 0)
      return(i);

   i = printGenQueryOut(stdout, format,hint,  genQueryOut);
   if (i < 0)
      return(i);


   while (i==0 && genQueryOut->continueInx > 0) {
      if (noPageFlag==0) {
	 char inbuf[100];
	 printf("Continue? [Y/n]");
	 fgets(inbuf, 90, stdin);
	 if (strncmp(inbuf, "n", 1)==0) break;
      }
      genQueryInp.continueInx=genQueryOut->continueInx;
      i = rcGenQuery (conn, &genQueryInp, &genQueryOut);
      if (i < 0)
	 return(i);
      i = printGenQueryOut(stdout, format,hint,  genQueryOut);
      if (i < 0)
	 return(i);
   }

   return(0);

}

int
execDbo(char *dbRescName, char *dboName, char *args[10]) {
   databaseObjControlInp_t databaseObjControlInp;
   databaseObjControlOut_t *databaseObjControlOut;
   int status;
   char *myName;
   char *mySubName;
   char fullName[BIG_STR];
   int i;

   memset((void *)&databaseObjControlInp, 0, sizeof(databaseObjControlInp));

   if (dbRescName==NULL || strlen(dbRescName)==0) {
      printf("You need to include the resource name; see 'help control'.\n");
      return(0);
   }

   databaseObjControlInp.dbrName = dbRescName;

   databaseObjControlInp.option = DBO_EXECUTE;

   if(dboName[0]=='/') {
      strncpy(fullName, dboName, BIG_STR);
   }
   else {
      strncpy(fullName, g_cwd, BIG_STR);
      strncat(fullName, "/", BIG_STR);
      strncat(fullName, dboName, BIG_STR);
   }

   databaseObjControlInp.dboName = fullName;
   databaseObjControlInp.dborName = g_dbor;
   databaseObjControlInp.subOption = g_force;

   for (i=0;i<10;i++) databaseObjControlInp.args[i] = args[i];

   status = rcDatabaseObjControl(Conn, &databaseObjControlInp, 
				 &databaseObjControlOut);
   if (status) {
      myName = rodsErrorName(status, &mySubName);
      rodsLog (LOG_ERROR, "rcDatabaseObjControl failed with error %d %s %s",
	       status, myName, mySubName);
      if (databaseObjControlOut != NULL &&
	  databaseObjControlOut->outBuf != NULL &&
	  strlen(databaseObjControlOut->outBuf)>0 ) {
	 int ix;
	 if(status == DBR_NOT_COMPILED_IN) {
	    printf("Message returned by the iRODS server:\n");
	 }
	 else {
	    printf("Message returned by the DBMS or DBO interface:\n");
	 }
	 printf("%s",databaseObjControlOut->outBuf);
	 ix = strlen(databaseObjControlOut->outBuf);
	 if (databaseObjControlOut->outBuf[ix-1]!='\n') {
	    printf("\n");
	 }
      }
      if (Conn->rError) {
	 rError_t *Err;
         rErrMsg_t *ErrMsg;
	 int i, len;
	 Err = Conn->rError;
	 len = Err->len;
	 for (i=0;i<len;i++) {
	    ErrMsg = Err->errMsg[i];
	    rodsLog(LOG_ERROR, "Detail: %s", ErrMsg->msg);
	 }
      }
      return(status);
   }
   if (*databaseObjControlOut->outBuf != '\0') {
      printf("%s",databaseObjControlOut->outBuf);
   }
   return(status);
}

/* commit or rollback */
int
dbrControl(char *dbRescName, int option) {
   databaseObjControlInp_t databaseObjControlInp;
   databaseObjControlOut_t *databaseObjControlOut;
   int status;
   char *myName;
   char *mySubName;

   memset((void *)&databaseObjControlInp, 0, sizeof(databaseObjControlInp));

   if (dbRescName==NULL || strlen(dbRescName)==0) {
      printf("You need to include the resource name; see 'help control'.\n");
      return(0);
   }

   databaseObjControlInp.dbrName = dbRescName;

   databaseObjControlInp.option = option;

   status = rcDatabaseObjControl(Conn, &databaseObjControlInp, 
				 &databaseObjControlOut);
   if (status) {
      myName = rodsErrorName(status, &mySubName);
      rodsLog (LOG_ERROR, "rcDatabaseObjControl failed with error %d %s %s",
	       status, myName, mySubName);
      if (databaseObjControlOut != NULL &&
	  databaseObjControlOut->outBuf != NULL &&
	  strlen(databaseObjControlOut->outBuf)>0 ) {
	 if(status == DBR_NOT_COMPILED_IN) {
	    printf("Error message return by the iRODS server:\n");
	 }
	 else {
	    printf("Error message return by the DBMS or DBO interface:\n");
	 }
	 printf("%s",databaseObjControlOut->outBuf);
      }
      return(status);
   }
   if (*databaseObjControlOut->outBuf != '\0') {
      printf("%s",databaseObjControlOut->outBuf);
   }
   return(status);
}

int
openDatabaseResc(char *dbRescName) {
   databaseRescOpenInp_t databaseRescOpenInp;
   int status;
   char *myName;
   char *mySubName;
   databaseRescOpenInp.dbrName = dbRescName;
   status = rcDatabaseRescOpen(Conn, &databaseRescOpenInp);
   if (status<0) {
      myName = rodsErrorName(status, &mySubName);
      rodsLog (LOG_ERROR, "rcDatabaseRescOpen failed with error %d %s %s",
	       status, myName, mySubName);
      return(status);
   }
/*   printf("open object-descriptor (index to open database)=%d\n",
     status);  Not generally needed anymore */
   return(status);
}

int
closeDatabaseResc(char *dbRescName) {
   databaseRescCloseInp_t databaseRescCloseInp;
   int status;
   char *myName;
   char *mySubName;
   databaseRescCloseInp.dbrName = dbRescName;
   status = rcDatabaseRescClose(Conn, &databaseRescCloseInp);
   if (status<0) {
      myName = rodsErrorName(status, &mySubName);
      rodsLog (LOG_ERROR, "rcDatabaseRescClose failed with error %d %s %s",
	       status, myName, mySubName);
      return(status);
   }
   return(status);
}

void
dborOutput(char *arg1, char *arg2) {
   char *dborName;

   g_force=0;
   if (arg2==NULL || *arg2=='\0') {
      dborName = arg1;
   }
   else {
      dborName = arg2;
      if (strncmp(arg1, "-f", 2)==0) g_force=1;
   }

   if (dborName==NULL || dborName[0]=='\0') {
      printf("DBOR option disabled; 'exec' results will be displayed\n");
      g_dbor[0]='\0';
      return;
   }
   if(dborName[0]=='/') {
      strncpy(g_dbor, dborName, BIG_STR);
   }
   else {
      strncpy(g_dbor, g_cwd, BIG_STR);
      strncat(g_dbor, "/", BIG_STR);
      strncat(g_dbor, dborName, BIG_STR);
   }
   printf("'exec' results will be stored in %s\n", g_dbor);
   if (g_force) {
      printf("If %s exists, the contents will be overwritten\n", g_dbor);
   }
   return;
}

void
getInputLine(char *prompt, char *result, int max_result_len) {
   char *stat;
   int i;
   fputs(prompt,stdout);
   stat = fgets(result, max_result_len, stdin);
   for (i=0;i<max_result_len;i++) {
      if (result[i]=='\n') {
	 result[i]='\0';
	 break;
      }
   }
}


/* handle a command,
   return code is 0 if the command was (at least partially) valid,
   -1 for quitting,
   -2 for if invalid
   -3 if empty.
 */
int
doCommand(char *cmdToken[]) {
   if (strcmp(cmdToken[0],"help")==0 ||
	      strcmp(cmdToken[0],"h") == 0) {
      usage(cmdToken[1]);
      return(0);
   }
   if (strcmp(cmdToken[0],"quit")==0 ||
	      strcmp(cmdToken[0],"q") == 0) {
      return(-1);
   }

   if (strcmp(cmdToken[0],"open") == 0) {
      openDatabaseResc(cmdToken[1]);
      return(0);
   }

   if (strcmp(cmdToken[0],"close") == 0) {
      closeDatabaseResc(cmdToken[1]);
      return(0);
   }

   if (strcmp(cmdToken[0],"ls") == 0) {
      queryAndShowStrCond(Conn, NULL, "%s/%s",
			  "select COLL_NAME, DATA_NAME where DATA_TYPE_NAME = 'database object'",
			  0, myRodsArgs.zoneName,
			  myRodsArgs.noPage);
      return(0);
   }

   if (strcmp(cmdToken[0],"exec") == 0) {
      execDbo(cmdToken[1], cmdToken[2], &cmdToken[3]);
      return(0);
   }
   if (strcmp(cmdToken[0],"commit") == 0) {
      dbrControl(cmdToken[1], DBR_COMMIT);
      return(0);
   }
   if (strcmp(cmdToken[0],"output") == 0) {
      dborOutput(cmdToken[1], cmdToken[2]);
      return(0);
   }
   if (strcmp(cmdToken[0],"rollback") == 0) {
      dbrControl(cmdToken[1], DBR_ROLLBACK);
      return(0);
   }
   if (strlen(cmdToken[0])>0) {
      printf("Unrecognized command\n");
   }
   return(-3);
}

int
main(int argc, char **argv) {
   int status, i, j;
   rErrMsg_t errMsg;

   char *mySubName;
   char *myName;

   int argOffset;

   int maxCmdTokens=20;
   char *cmdToken[20];
   int keepGoing;
   int firstTime;

   rodsLogLevel(LOG_ERROR);

   status = parseCmdLineOpt (argc, argv, "vVhrcRCduz:", 0, &myRodsArgs);
   if (status) {
      printf("Use -h for help.\n");
      exit(1);
   }
   if (myRodsArgs.help==True) {
      usage("");
      exit(0);
   }

   if (myRodsArgs.zone==True) {
      strncpy(zoneArgument, myRodsArgs.zoneName, MAX_NAME_LEN);
   }

   argOffset = myRodsArgs.optind;
   if (argOffset > 1) {
      if (argOffset > 2) {
	 if (*argv[1]=='-' && *(argv[1]+1)=='z') {
	    if (*(argv[1]+2)=='\0') {
	       argOffset=3;  /* skip -z zone */
	    }
	    else {
	       argOffset=2;  /* skip -zzone */
	    }
	 }
	 else {
	    argOffset=1; /* Ignore the parseCmdLineOpt parsing 
			    as -d etc handled  below*/
	 }
      }
      else {
	 argOffset=1; /* Ignore the parseCmdLineOpt parsing 
			 as -d etc handled  below*/
      }
   }

   status = getRodsEnv (&myEnv);
   if (status < 0) {
      rodsLog (LOG_ERROR, "main: getRodsEnv error. status = %d",
	       status);
      exit (1);
   }
   strncpy(g_cwd,myEnv.rodsCwd,BIG_STR);
   if (strlen(g_cwd)==0) {
      strcpy(g_cwd,"/");
   }

   for (i=0;i<maxCmdTokens;i++) {
      cmdToken[i]="";
   }
   j=0;
   for (i=argOffset;i<argc;i++) {
      cmdToken[j++]=argv[i];
   }

   if (strcmp(cmdToken[0],"help")==0 ||
	      strcmp(cmdToken[0],"h") == 0) {
      usage(cmdToken[1]);
      exit(0);
   }

   Conn = rcConnect (myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                     myEnv.rodsZone, 0, &errMsg);

   if (Conn == NULL) {
      myName = rodsErrorName(errMsg.status, &mySubName);
      rodsLog(LOG_ERROR, "rcConnect failure %s (%s) (%d) %s",
	      myName,
	      mySubName,
	      errMsg.status,
	      errMsg.msg);

      exit (2);
   }

   status = clientLogin(Conn);
   if (status != 0) {
      if (!debug) exit (3);
   }

   keepGoing=1;
   firstTime=1;
   while (keepGoing) {
      int status;
      status=doCommand(cmdToken);
      if (status==-1) keepGoing=0;
      if (firstTime) {
	 if (status==0) keepGoing=0;
	 if (status==-2) {
	    keepGoing=0;
	    lastCommandStatus=-1;
	 }
	 firstTime=0;
      }
      if (keepGoing) {
	 getInput(cmdToken, maxCmdTokens);
      }
   }

   rcDisconnect(Conn);

   if (lastCommandStatus != 0) exit(4);
   exit(0);
}

void
printMsgs(char *msgs[]) {
   int i;
   for (i=0;;i++) {
      if (strlen(msgs[i])==0) return;
      printf("%s\n",msgs[i]);
   }
}

/*
Print the main usage/help information.
 */
void usageMain()
{
   char *msgs[]={
"Execute a Database Object (DBO) on a Database Resource (DBR) or",
"perform other DBO operations.",
"Typical usage:",
" idbo exec DBR DBO",
" ",
"A single command can be entered on the command line or, if blank, it",
"will prompt go into interactive mode and prompt for commands.",
"Commands are:",
"  open DBR    (open a database resource)",
"  close DBR   (close a database resource)",
"  exec DBR DBO [arguments] (execute a DBO on a DBR)",
"  output [-f] DBOR  (store 'exec' results in another data-object)",
"  commit DBR   (commit updates to a DBR (done via a DBO))",
"  rollback DBR (rollback updates instead)",
"  ls           (list defined Database-Objects in the Zone)",
"  help (or h) [command] (this help, or help on a command)",
"  quit  (or 'q', exit idbo)",
"Where DBR and DBO are the names of a Database Resource and Database Object.",
" ",
"You can exectute a DBO without first opening the DBR (in which case the",
"server will open and close it), so you can run a DBO from the command",
"line: 'idbo exec DBR DBO'",
" ",
"Like other unix utilities, a series of commands can be piped into it:",
"'cat file1 | idbo' (maintaining one connection for all commands).",
" ",
"See 'Database Resources' on the irods web site for more information.",
" ",
"Try 'help command' for more help on a specific command.",
"'help exec' will explain options on the exec command.",

""};
   printMsgs(msgs);
   printReleaseInfo("idbo");
}

/*
Print either main usage/help information, or some more specific
information on particular commands.
 */
void
usage(char *subOpt)
{
   char *openMsgs[]={
"open  DatabaseResourceName", 
"Open the specified database resource. ",
"If you are just running DBO that only queries, you do not need to",
"open or close the DBR, it will be done automatically.",
""};
   char *closeMsgs[]={
"  close DBR",
"Close a database resource.",
"If you have opened a DBR, it will be closed.",
"If you are just running DBO that only queries, you do not need to",
"open or close the DBR, it will be done automatically.",
"If you have updated a DBR via a DBO, you should 'commit' before closing.",
""};
   char *execMsgs[]={
"  exec DBR DBO [arguments]",
"Execute a DBO on a DBR.",
"By default, the results (of a query) will be returned and displayed.",
"If you are just running a DBO that only queries, you do not need to",
"open or close the DBR, it will be done automatically.",
"Error messages, if any, will be displayed.",
"For DBOs that take one or more arguments, these are supplied after the DBO",
"name; if an argument contains a blank, enclose it with singles quotes (').",
"DBOs are specified via either the full or relative (to the current) path."
""};
   char *outputMsgs[]={
"  output [-f] DBOR  (store 'exec' results in another data-object)",
"When 'exec' is run, instead of returning the results to the terminal,",
"write the results to the DBOR data-object.  If -f is included,",
"the data-object will be overwritten if it exists.",
"DBORs (DBO Result data-objects) are specified via either the full or",
"relative (to the current) path.",
""};
   char *lsMsgs[]={
"ls ", 
"List information about all defined database objects (in the zone).",
"This does a query to find all the DBOs. This can also be done via iquest, with:",
"select COLL_NAME, DATA_NAME where DATA_TYPE_NAME = 'database object'",
"Once found, other information is available via 'ils'.",
"You can also 'iget' the data-object to see the contents (the SQL).",
""};
   char *commitMsgs[]={
"  commit DBR",
"commit any updates to a DBR.",
"To run a DBO that updates the DBR, you need to open, exec, and commit.",
"If you are just running DBO that only queries, you do not need to",
"open or close the DBR, or commit.",
""};
   char *rollbackMsgs[]={
"  rollback DBR",
"rollback any updates to a DBR.",
"To run a DBO that updates the DBR, you do a rollback to cancel",
"the updates.",
""};
   char *helpMsgs[]={
" help (or h) [command] (general help, or more details on a command)",
"If you specify a command, a brief description of that command",
"will be displayed.",
""};
   char *quitMsgs[]={
      " quit (or q) ",
"Exit idbo",
""};


   char *subCmds[]={"open", "close", "exec", "output", "ls", 
		    "commit", "rollback", "help", "h", "quit", "q",
		    ""};

   char **pMsgs[]={ openMsgs, closeMsgs, execMsgs, outputMsgs, lsMsgs,
		    commitMsgs, rollbackMsgs, helpMsgs, helpMsgs,
		    quitMsgs, quitMsgs};

   if (*subOpt=='\0') {
      usageMain();
   }
   else {
      int i;
      for (i=0;;i++) {
	 if (strlen(subCmds[i])==0) break;
	 if (strcmp(subOpt,subCmds[i])==0) {
	    printMsgs(pMsgs[i]);
	 }
      }
   }
}
