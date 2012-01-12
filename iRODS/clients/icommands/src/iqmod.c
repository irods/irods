/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* 
  Initial version of an administrator interface for changing values
  in delayed execution rules.
*/

#include "rodsClient.h"
#include "parseCommandLine.h"

#define MAX_SQL 300
#define BIG_STR 200

void usage();

int debug=0;

rcComm_t *Conn;
rodsEnv myEnv;

int
modDelayedRule(char *ruleId, char *fieldName, char *fieldValue) {
   int status;

   ruleExecModInp_t ruleExecModInp;
   memset(&ruleExecModInp, 0, sizeof(ruleExecModInp));

   /* for the time fields, convert from YYYY-MM-DD.hh:mm:ss in sec of unix time 
	  if necessary */
   if ( strcmp(fieldName, "exeTime") == 0 || strcmp(fieldName, "estimateExeTime") == 0 ||
        strcmp(fieldName, "lastExeTime") == 0 ) {
	status = checkDateFormat(fieldValue);
	if ( status == DATE_FORMAT_ERR ) {
		printf("Time format error: it should be sec in unix time or YYYY-MM-DD.hh:mm:ss.\n");
		return (status);
	}
   }
   
   strncpy(ruleExecModInp.ruleId, ruleId, NAME_LEN);
   addKeyVal(&ruleExecModInp.condInput, fieldName, fieldValue);

   status = rcRuleExecMod(Conn, &ruleExecModInp);

   if (status == CAT_SUCCESS_BUT_WITH_NO_INFO) {
      printf("No rule found with id %s\n",ruleId);
      return(status);
   }
   if (status < 0) {
      printError(Conn, status, "rcRuleExecMod");
   }
   return(status);
}

int
main(int argc, char **argv) {
   int status;
   rErrMsg_t errMsg;

   rodsArguments_t myRodsArgs;
   int argOffset;

   rodsLogLevel(LOG_ERROR);

   status = parseCmdLineOpt (argc, argv, "hvV", 0, &myRodsArgs);
   if (status) {
      printf("Use -h for help\n");
      exit(1);
   }
   if (myRodsArgs.help==True) {
      usage();
      exit(0);
   }
   argOffset = myRodsArgs.optind;

   status = getRodsEnv (&myEnv);
   if (status < 0) {
      rodsLog (LOG_ERROR, "main: getRodsEnv error. status = %d",
	       status);
      exit (1);
   }

   if (argOffset != argc-3) {
      usage();
      exit(-1);
   }
   Conn = rcConnect (myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                     myEnv.rodsZone, 0, &errMsg);
   if (Conn == NULL) {
      exit (2);
   }

   status = clientLogin(Conn);
   if (status != 0) {
      printError(Conn, status, "clientLogin");
      if (!debug) exit (3);
   }

   status = modDelayedRule(argv[argOffset], argv[argOffset+1],
			   argv[argOffset+2]);

   rcDisconnect(Conn);

   if (status!=0) exit(4);
   exit(0);
}

void usage()
{
   printf("Usage: iqmod [-vVh] ruleId FIELD_NAME fieldValue\n");
   printf("\n");
   printf("iqmod modifies values in existing delayed rules.\n");
   printf("\n");
   printf("FIELD_NAME can be: %s %s %s %s\n",
	  RULE_NAME_KW, RULE_REI_FILE_PATH_KW, RULE_USER_NAME_KW, 
	  RULE_EXE_ADDRESS_KW);
   printf("%s %s %s %s\n",
	  RULE_EXE_TIME_KW, RULE_EXE_FREQUENCY_KW, RULE_PRIORITY_KW,
	  RULE_ESTIMATE_EXE_TIME_KW);
   printf("%s %s or %s \n",
	  RULE_NOTIFICATION_ADDR_KW, RULE_LAST_EXE_TIME_KW,RULE_EXE_STATUS_KW);
   printf("\n");
   printf("Also see iqstat and iqdel.\n");
   printReleaseInfo("iqmod");
}
