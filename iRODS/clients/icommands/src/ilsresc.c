/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* 
  This is a regular-user level utility to list the resources.
*/

#include "rods.h"
#include "rodsClient.h"

#define MAX_SQL 300
#define BIG_STR 200

rcComm_t *Conn;

char zoneArgument[MAX_NAME_LEN+2]="";

void usage();

/* 
 print the results of a general query.
 */
int
printGenQueryResults(rcComm_t *Conn, int status, genQueryOut_t *genQueryOut, 
		     char *descriptions[], int doDashes)
{
   int printCount;
   int i, j;
   char localTime[20];
   printCount=0;
   if (status!=0) {
      printError(Conn, status, "rcGenQuery");
   }
   else {
      if (status !=CAT_NO_ROWS_FOUND) {
	 for (i=0;i<genQueryOut->rowCnt;i++) {
	    if (i>0 && doDashes) printf("----\n");
	    for (j=0;j<genQueryOut->attriCnt;j++) {
	       char *tResult;
	       tResult = genQueryOut->sqlResult[j].value;
	       tResult += i*genQueryOut->sqlResult[j].len;
	       if (*descriptions[j]!='\0') {
		  if (strstr(descriptions[j],"time")!=0) {
		     getLocalTimeFromRodsTime(tResult, localTime);
		     printf("%s: %s: %s\n", descriptions[j], tResult, 
			    localTime);
		  } 
		  else {
		     printf("%s: %s\n", descriptions[j], tResult);
		  }
	       }
	       else {
		  printf("%s\n", tResult);
	       }
	       printCount++;
	    }
	 }
      }
   }
   return(printCount);
}

/* 
 print the results of a general query, formatted for the Resc ACLs
 */
int
printGenQueryResultsRescACLs(rcComm_t *Conn, int status, 
			     genQueryOut_t *genQueryOut)
{
   int printCount;
   int i, j;
   char interChars[]="#:   ";
   printCount=0;
   if (status!=0) {
      printError(Conn, status, "rcGenQuery");
   }
   else {
      if (status !=CAT_NO_ROWS_FOUND) {
	 for (i=0;i<genQueryOut->rowCnt;i++) {
	    printf("      ");
	    for (j=0;j<genQueryOut->attriCnt;j++) {
	       char *tResult;
	       tResult = genQueryOut->sqlResult[j].value;
	       tResult += i*genQueryOut->sqlResult[j].len;
	       printf("%s%c",tResult,interChars[j]);
	       printCount++;
	    }
	    printf("\n");
	 }
      }
   }
   return(printCount);
}

/*
Via a general query, show a resource
*/
int
showResc(char *name, int longOption) 
{
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut;
   int i1a[20];
   int i1b[20]={0,0,0,0,0,0,0,0,0,0,0,0};
   int i2a[20];
   char *condVal[10];
   char v1[BIG_STR];
   int i, status;
   int printCount;
   char *columnNames[]={"resource name", "resc id", "zone", "type", "class",
			"location",  "vault", "free space", "status",
			"info", "comment",
			"create time", "modify time"};

   memset (&genQueryInp, 0, sizeof (genQueryInp_t));
   printCount=0;

   i=0;
   i1a[i++]=COL_R_RESC_NAME;
   if (longOption) {
      i1a[i++]=COL_R_RESC_ID;
      i1a[i++]=COL_R_ZONE_NAME;
      i1a[i++]=COL_R_TYPE_NAME;
      i1a[i++]=COL_R_CLASS_NAME;
      i1a[i++]=COL_R_LOC;
      i1a[i++]=COL_R_VAULT_PATH;
      i1a[i++]=COL_R_FREE_SPACE;
      i1a[i++]=COL_R_RESC_STATUS;
      i1a[i++]=COL_R_RESC_INFO;
      i1a[i++]=COL_R_RESC_COMMENT;
      i1a[i++]=COL_R_CREATE_TIME;
      i1a[i++]=COL_R_MODIFY_TIME;
   }
   else {
      columnNames[0]="";
   }
   
   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = i;

   genQueryInp.sqlCondInp.inx = i2a;
   genQueryInp.sqlCondInp.value = condVal;
   if (name !=NULL && *name != '\0') {
      i2a[0]=COL_R_RESC_NAME;
      sprintf(v1,"='%s'",name);
      condVal[0]=v1;
      genQueryInp.sqlCondInp.len=1;
   }
   else {
      genQueryInp.sqlCondInp.len=0;
   }

   if (zoneArgument[0]!='\0') {
      addKeyVal (&genQueryInp.condInput, ZONE_KW, zoneArgument);
   }

   genQueryInp.maxRows=50;
   genQueryInp.continueInx=0;
   status = rcGenQuery(Conn, &genQueryInp, &genQueryOut);
   if (status == CAT_NO_ROWS_FOUND) {
      i1a[0]=COL_R_RESC_INFO;
      genQueryInp.selectInp.len = 1;
      status = rcGenQuery(Conn, &genQueryInp, &genQueryOut);
      if (status==0) {
	 printf("None\n");
	 return(0);
      }
      if (status == CAT_NO_ROWS_FOUND) {
	 if (name!=NULL && name[0]!='\0') {
	    printf("Resource %s does not exist.\n", name);
	 }
	 else {
	    printf("Resource does not exist.\n");
	 }
	 return(0);
      }
   }

   printCount+= printGenQueryResults(Conn, status, genQueryOut, columnNames, 
				     longOption);
   while (status==0 && genQueryOut->continueInx > 0) {
      genQueryInp.continueInx=genQueryOut->continueInx;
      status = rcGenQuery(Conn, &genQueryInp, &genQueryOut);
      if (genQueryOut->rowCnt > 0 && longOption) {
	 printf("----\n");
      }
      printCount+= printGenQueryResults(Conn, status, genQueryOut, 
					columnNames, longOption);
   }

   return (1);
}

/*
 Show the resource group
 */
int
showOneRescGroup(char *rescGroupName, int longOption)
{
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut;
   int i1a[20];
   int i1b[20]={0,0,0,0,0,0,0,0,0,0,0,0};
   int i2a[20];
   char *condVal[10];
   char v1[BIG_STR];
   int i, status;
   char *tResult;

   memset (&genQueryInp, 0, sizeof (genQueryInp_t));
   i=0;
   i1a[i++]=COL_R_RESC_NAME;
   
   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = i;

   genQueryInp.sqlCondInp.inx = i2a;
   genQueryInp.sqlCondInp.value = condVal;

   i2a[0]=COL_RESC_GROUP_NAME;
   sprintf(v1,"='%s'",rescGroupName);
   condVal[0]=v1;
   genQueryInp.sqlCondInp.len=1;

   if (zoneArgument[0]!='\0') {
      addKeyVal (&genQueryInp.condInput, ZONE_KW, zoneArgument);
   }

   genQueryInp.maxRows=50;
   genQueryInp.continueInx=0;
   status = rcGenQuery(Conn, &genQueryInp, &genQueryOut);
   if (status == CAT_NO_ROWS_FOUND) {
      return(0);
   }

   if (longOption) {
      printf("resource group: %s \n",rescGroupName);
   }
   else {
      printf("%s (resource group), resources: ",rescGroupName);
   }

   for (i=0;i<genQueryOut->rowCnt;i++) {
      tResult = genQueryOut->sqlResult[0].value;
      tResult += i*genQueryOut->sqlResult[0].len;
      if (longOption) {
	 printf("Includes resource: %s\n", tResult);
      }
      else {
	 printf("%s ", tResult);
      }
   }

   while (status==0 && genQueryOut->continueInx > 0) {
      genQueryInp.continueInx=genQueryOut->continueInx;
      status = rcGenQuery(Conn, &genQueryInp, &genQueryOut);
      for (i=0;i<genQueryOut->rowCnt;i++) {
	 tResult = genQueryOut->sqlResult[0].value;
	 tResult += i*genQueryOut->sqlResult[0].len;
	 printf("%s ", tResult);
      }
   }
   if (!longOption) {
      printf("\n");
   }

   return (1);
}

/*
Via a general query, show a resource Access Control List
*/
int
showRescAcl(char *name) 
{
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut;
   int i1a[20];
   int i1b[20]={0,0,0,0,0,0,0,0,0,0,0,0};
   int i2a[20];
   char *condVal[10];
   char v1[BIG_STR];
   int i, status;
   int printCount;

   memset (&genQueryInp, 0, sizeof (genQueryInp_t));
   printCount=0;


   i=0;
   i1a[i++]=COL_RESC_USER_NAME;
   i1a[i++]=COL_RESC_USER_ZONE;
   i1a[i++]=COL_RESC_ACCESS_NAME;
   
   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = i;

   genQueryInp.sqlCondInp.inx = i2a;
   genQueryInp.sqlCondInp.value = condVal;
   if (name !=NULL && *name != '\0') {
      i2a[0]=COL_R_RESC_NAME;
      sprintf(v1,"='%s'",name);
      condVal[0]=v1;
      genQueryInp.sqlCondInp.len=1;
   }
   else {
      genQueryInp.sqlCondInp.len=0;
   }

   if (zoneArgument[0]!='\0') {
      addKeyVal (&genQueryInp.condInput, ZONE_KW, zoneArgument);
   }

   genQueryInp.maxRows=50;
   genQueryInp.continueInx=0;
   status = rcGenQuery(Conn, &genQueryInp, &genQueryOut);
   if (status == CAT_NO_ROWS_FOUND) {
      i1a[0]=COL_R_RESC_INFO;
      genQueryInp.selectInp.len = 1;
      status = rcGenQuery(Conn, &genQueryInp, &genQueryOut);
      if (status==0) {
	 printf("None\n");
	 return(0);
      }
      if (status == CAT_NO_ROWS_FOUND) {
	 if (name!=NULL && name[0]!='\0') {
	    printf("Resource %s does not exist.\n", name);
	 }
	 else {
	    printf("Resource does not exist.\n");
	 }
	 return(0);
      }
   }
   printf("Resource access permissions apply only to Database Resources\n");
   printf("(external databases).  See the Database Resources page on the\n");
   printf("irods web site for more information.\n");

   printf("   %s\n",name);

   printCount+= printGenQueryResultsRescACLs(Conn, status, genQueryOut);
   while (status==0 && genQueryOut->continueInx > 0) {
      genQueryInp.continueInx=genQueryOut->continueInx;
      status = rcGenQuery(Conn, &genQueryInp, &genQueryOut);
      printCount+= printGenQueryResultsRescACLs(Conn, status, genQueryOut);
   }

   return (1);
}


/*
  Show the resource groups, if any
*/
int
showRescGroups(int longOption)
{
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut;
   int i1a[20];
   int i1b[20]={0,0,0,0,0,0,0,0,0,0,0,0};
   int i2a[20];
   char *condVal[10];
   int i, status;

   memset(&genQueryInp, 0, sizeof(genQueryInp));

   i=0;
   i1a[i++]=COL_RESC_GROUP_NAME;
   
   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = i;

   genQueryInp.sqlCondInp.inx = i2a;
   genQueryInp.sqlCondInp.value = condVal;

   genQueryInp.sqlCondInp.len=0;

   if (zoneArgument[0]!='\0') {
      addKeyVal (&genQueryInp.condInput, ZONE_KW, zoneArgument);
   }

   genQueryInp.maxRows=50;
   genQueryInp.continueInx=0;
   status = rcGenQuery(Conn, &genQueryInp, &genQueryOut);
   if (status == CAT_NO_ROWS_FOUND) {
      return(0);
   }

   if (status != 0) return(0);

   for (i=0;i<genQueryOut->rowCnt;i++) {
      char *tResult;
      tResult = genQueryOut->sqlResult[0].value;
      tResult += i*genQueryOut->sqlResult[0].len;
      if (longOption==0) {
	 printf("%s (resource group)\n", tResult);
      }
      else {
	 printf("-----\n");
	 showOneRescGroup(tResult, longOption);
      }
   }

   while (status==0 && genQueryOut->continueInx > 0) {
      genQueryInp.continueInx=genQueryOut->continueInx;
      status = rcGenQuery(Conn, &genQueryInp, &genQueryOut);
      if (status) {
	 return(0);
      }
      for (i=0;i<genQueryOut->rowCnt;i++) {
	 char *tResult;
	 tResult = genQueryOut->sqlResult[0].value;
	 tResult += i*genQueryOut->sqlResult[0].len;
	 if (longOption==0) {
	    printf("%s (resource group)\n", tResult);
	 }
	 else {
	    printf("-----\n");
	    showOneRescGroup(tResult, longOption);
	 }
      }
   }
   return (0);
}

int
main(int argc, char **argv) {
   int status, status2;
   rErrMsg_t errMsg;

   rodsArguments_t myRodsArgs;

   char *mySubName;
   char *myName;

   rodsEnv myEnv;


   rodsLogLevel(LOG_ERROR);

   status = parseCmdLineOpt (argc, argv, "AhvVlz:", 0, &myRodsArgs);
   if (status) {
      printf("Use -h for help.\n");
      exit(1);
   }

   if (myRodsArgs.help==True) {
      usage();
      exit(0);
   }

   if (myRodsArgs.zone==True) {
      strncpy(zoneArgument, myRodsArgs.zoneName, MAX_NAME_LEN);
   }

   status = getRodsEnv (&myEnv);
   if (status < 0) {
      rodsLog (LOG_ERROR, "main: getRodsEnv error. status = %d",
	       status);
      exit (1);
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
      exit (3);
   }

   status2=0;
   if (myRodsArgs.optind == argc) {  /* no resource name specified */
      status = showResc(argv[myRodsArgs.optind], myRodsArgs.longOption);
      status2 = showRescGroups(myRodsArgs.longOption);
   }
   else {
      if (myRodsArgs.accessControl == True) {
	 showRescAcl(argv[myRodsArgs.optind]);
      }
      else {
	 status = showOneRescGroup(argv[myRodsArgs.optind], 
				   myRodsArgs.longOption);
	 if (status==0) {
	    status2 = showResc(argv[myRodsArgs.optind], myRodsArgs.longOption);
	    if (status2==0) {
	       /* Only print this if both fail */
	       printf("Resource-group %s does not exist.\n",
		      argv[myRodsArgs.optind]); 
	    }
	 }
      }
   }

   rcDisconnect(Conn);

   /* Exit 0 if one or more items were displayed */
   if (status > 0) exit(0);
   if (status2 > 0) exit(0);
   exit(4);
}

/*
Print the main usage/help information.
 */
void usage()
{
   char *msgs[]={
"ilsresc lists iRODS resources and resource-groups",
"Usage: ilsresc [-lvVhA] [Name]", 
"If Name is present, list only that resource or resource-group,",
"otherwise list them all ",
"Options are:", 
" -l Long format - list details",
" -v verbose",
" -V Very verbose",
" -z Zonename  list resources of specified Zone",
" -A Rescname  list the access permisssions (applies to Database Resources only)",
" -h This help",
""};
   int i;
   for (i=0;;i++) {
      if (strlen(msgs[i])==0) break;
      printf("%s\n",msgs[i]);
   }
   printReleaseInfo("ilsresc");
}

