/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rodsServer.h"
#include "reGlobalsExtern.h"
#include "reDefines.h"
#include <sys/time.h>
time_t time_offset(const time_t *base, const struct tm *off);
void
retest_usage (char *prog)
{
    printf ("Usage: %s [-D objPath] [-R rescName] [-T dataType] [-H clientHost] [-S dataSize] [-U dataOwnerName] [-Z dataOwnerZone][-n otherUserName] [-z otherUserZone] [-t otherUserType] [-p proxyUserName] [-q proxyUserZone] <ruleSet> <action> [arg ...]\n", prog);
}

int main(int argc, char **argv)
{
    int status;
    int c,i,j;
    int uFlag = 0;;
    char tmpStr[100];
    char retestflagEV[200];
    char reloopbackflagEV[200];
    ruleExecInfo_t *rei;
    char *args[MAX_NUM_OF_ARGS_IN_ACTION];
    char ruleSet[RULE_SET_DEF_LENGTH];
    hrtime_t ht1, ht2, ht3;
    time_t now = time(0);
    struct tm off = {0};
    time_t new;
    struct tm *mytm;

    rei = mallocAndZero(sizeof(ruleExecInfo_t));
    if (argc < 2) {
      retest_usage(argv[0]);
      exit(1);
    }
    if (getenv(RETESTFLAG) == NULL) {
      sprintf(retestflagEV,"RETESTFLAG=%i",COMMAND_TEST_1);
      putenv(retestflagEV);
    }
    if (getenv(RELOOPBACKFLAG) == NULL) {
      sprintf(reloopbackflagEV,"RELOOPBACKFLAG=%i",LOOP_BACK_1);
      putenv(reloopbackflagEV);
    }

    rei->doi = mallocAndZero(sizeof(dataObjInfo_t));
    rei->rgi = mallocAndZero(sizeof(rescGrpInfo_t));
    rei->rgi->rescInfo = mallocAndZero(sizeof(rescInfo_t));
    rei->uoip = mallocAndZero(sizeof(userInfo_t));
    rei->uoic = mallocAndZero(sizeof(userInfo_t));
    rei->uoio = mallocAndZero(sizeof(userInfo_t));
    rei->coi = mallocAndZero(sizeof(collInfo_t));

    rei->next = NULL;

    strcpy(rei->doi->objPath,"/home/sekar.sdsc");
    strcpy(rei->doi->rescName,"unix-sdsc");
    strcpy(rei->doi->dataType,"text");
    strcpy(rei->uoic->authInfo.host,"");
    rei->doi->dataSize = 100;

    rei->doinp = NULL;
    rei->rsComm = NULL;


    while ((c=getopt(argc, argv,"D:T:R:H:S:U:Z:n:z:t:p:q:h")) != EOF) {
      switch (c) {
      case 'D':
	strcpy(rei->doi->objPath,optarg);
	break;
      case 'R':
	strcpy(rei->doi->rescName,optarg);
	break;
      case 'S':
	rei->doi->dataSize = atol(optarg);
	break;
      case 'T':
	strcpy(rei->doi->dataType,optarg);
	break;
      case 'U':
	strcpy(rei->doi->dataOwnerName,optarg);
	break;
      case 'Z':
	strcpy(rei->doi->dataOwnerZone,optarg);
	break;
      case 'p':
	strcpy(rei->uoip->userName,optarg);
	break;
      case 'q':
	strcpy(rei->uoip->rodsZone,optarg);
	break;
      case 'n':
	strcpy(rei->uoio->userName,optarg);
	break;
      case 'z':
	strcpy(rei->uoio->rodsZone,optarg);
	break;
      case 't':
	strcpy(rei->uoio->userType,optarg);
	break;
       case 'H':
	strcpy(rei->uoic->authInfo.host,optarg);
	break;
      case 'h':
	retest_usage (argv[0]);
	exit (0);
      default:
	retest_usage (argv[0]);
	exit (1);
      }
    }
    print_uoi(rei->uoio);
    for (i = optind+2; i < argc; i++) {
      args[i-optind-2] = strdup(argv[i]);
    }
    if (optind+1 == argc) {
      retest_usage(argv[0]);
      exit(1);
    }

    strcpy(ruleSet,argv[optind]);
    ht1 = gethrtime();
    initRuleStruct(ruleSet,"core", "core");
    ht2 = gethrtime();

    /************
    pushStack(&delayStack,"1"); printf("pushStack %s\n","1");
    pushStack(&delayStack,"2"); printf("pushStack %s\n","2");
    pushStack(&delayStack,"3"); printf("pushStack %s\n","3");
    pushStack(&delayStack,"4"); printf("pushStack %s\n","4");
    popStack(&delayStack,tmpStr);  printf("popStack %s\n",tmpStr);
    popStack(&delayStack,tmpStr);  printf("popStack %s\n",tmpStr);
    pushStack(&delayStack,"a"); printf("pushStack %s\n","a");
    pushStack(&delayStack,"b"); printf("pushStack %s\n","b");
    pushStack(&delayStack,"c"); printf("pushStack %s\n","c");
    popStack(&delayStack,tmpStr);  printf("popStack %s\n",tmpStr);
    popStack(&delayStack,tmpStr);  printf("popStack %s\n",tmpStr);
    popStack(&delayStack,tmpStr);  printf("popStack %s\n",tmpStr);
    popStack(&delayStack,tmpStr);  printf("popStack %s\n",tmpStr);
    pushStack(&delayStack,"100"); printf("pushStack %s\n","100");
    popStack(&delayStack,tmpStr);  printf("popStack %s\n",tmpStr);
    popStack(&delayStack,tmpStr);  printf("popStack %s\n",tmpStr);
    popStack(&delayStack,tmpStr);  printf("popStack %s\n",tmpStr);
    popStack(&delayStack,tmpStr);  printf("popStack %s\n",tmpStr);
    pushStack(&delayStack,"11"); printf("pushStack %s\n","11");
    pushStack(&delayStack,"12"); printf("pushStack %s\n","12");
    popStack(&delayStack,tmpStr);  printf("popStack %s\n",tmpStr);
    popStack(&delayStack,tmpStr);  printf("popStack %s\n",tmpStr);
    exit(0);
    ************/
    /******
    printf("local time now: %s\n", ctime(&now));
    mytm = localtime (&now);
    printf ("NOW=%4d-%2d-%2d-%2d.%2d.%2d\n", 
      mytm->tm_year + 1900, mytm->tm_mon + 1, mytm->tm_mday, 
      mytm->tm_hour, mytm->tm_min, mytm->tm_sec);
    
    off.tm_hour = -23;
    new = time_offset(&now, &off);
    printf("local time 23 hours ago: %s\n", ctime(&new));
    mytm = localtime (&new);
    printf ("NOW-23H=%4d-%2d-%2d-%2d.%2d.%2d\n", 
      mytm->tm_year + 1900, mytm->tm_mon + 1, mytm->tm_mday, 
      mytm->tm_hour, mytm->tm_min, mytm->tm_sec);
    
    off.tm_hour = 0;
    off.tm_mday = 50;
    new = time_offset(&now, &off);
    printf("local time 20 days ago: %s\n", ctime(&new));
    mytm = localtime (&new);
    printf ("NOW-20D=%4d-%2d-%2d-%2d.%2d.%2d\n", 
      mytm->tm_year + 1900, mytm->tm_mon + 1, mytm->tm_mday, 
      mytm->tm_hour, mytm->tm_min, mytm->tm_sec);
    exit(0);
    ************/
    /*******************
    argc=0;
    i = parseAction(argv[optind+1], tmpStr, args, &argc);
    printf(" inAction = %s\n Action= %s\n NumARgs=%i\n Status = %i\n",argv[optind+1], tmpStr, argc,i);
    for (i = 0; i < argc; i++) {
      printf("   Arg[%i] = |%s|\n",i,args[i]);
    }
    exit(0);
    ************/

    /*******
    i = execMyRule("myTestRule||msiDataObjOpen(*A,*X)##msiDataObjCreate(*B,*Y)##msiDataObjClose(*X,*Z1)##msiDataObjClose(*Y,*Z2)",NULL,rei);
    exit(0);
    *******/
    /****
    i = applyRule(argv[optind+1], args, argc - optind-2, rei, SAVE_REI);
    ****/
    i = applyRuleArgPA(argv[optind+1], args, argc - optind-2, NULL, rei, SAVE_REI);
    /****/
    ht3 = gethrtime();
    if (reTestFlag == COMMAND_TEST_1||  reTestFlag == COMMAND_TEST_MSI) { 
        fprintf(stdout,"Rule Initialization Time = %.2f millisecs\n", (float) (ht2 - ht1)/1000000);
        fprintf(stdout,"Rule Execution     Time = %.2f millisecs\n", (float) (ht3-ht1)/1000000);
    }
    if (reTestFlag == HTML_TEST_1 || reTestFlag ==  HTML_TEST_MSI) {
        fprintf(stdout,"Rule Initialization Time = %.2f millisecs<BR>n", (float) (ht2 - ht1)/1000000);
        fprintf(stdout,"Rule Execution     Time = %.2f millisecs<BR>\n", (float) (ht3-ht1)/1000000);
    }
    if (i != 0) {
      rodsLogError(LOG_ERROR,i,"reTest Failed:");
      exit(1);
    }
    exit(0);
}

time_t time_offset(const time_t *base, const struct tm *off) {
struct tm *t;

t = localtime(base);
if (!t) return (time_t)(-1);

t->tm_sec += off->tm_sec;
t->tm_min += off->tm_min;
t->tm_hour += off->tm_hour;
t->tm_mday += off->tm_mday;
t->tm_mon += off->tm_mon;
t->tm_year += off->tm_year;

return mktime(t);
}

