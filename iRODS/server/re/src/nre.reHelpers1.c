/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reHelpers1.h"

#include "index.h"
#include "parser.h"
#include "filesystem.h"
#include "configuration.h"

struct Breakpoint {
	char *actionName;
	char *base;
	int pending;
	int row;
	int line;
	rodsLong_t start;
	rodsLong_t finish; /* exclusive */
} breakPoints[100];

int breakPointsInx = 0;

void disableReDebugger(int grdf[2]) {
	  grdf[0] = GlobalREAuditFlag;
	  grdf[1] = GlobalREDebugFlag;
	  GlobalREAuditFlag = 0;
	  GlobalREDebugFlag = 0;

}

void enableReDebugger(int grdf[2]) {
	  GlobalREAuditFlag = grdf[0];
	  GlobalREDebugFlag = grdf[1];

}
char myHostName[MAX_NAME_LEN];
char waitHdr[HEADER_TYPE_LEN];
char waitMsg[MAX_NAME_LEN];
int myPID;

int initializeReDebug(rsComm_t *svrComm, int flag) 
{
  char condRead[NAME_LEN];
  int i, s,m, status;
  char *readhdr = NULL;
  char *readmsg = NULL;
  char *user = NULL;
  char *addr = NULL;

  if (svrComm == NULL)
    return(0);
  if ( GlobalREDebugFlag != 4 ) 
    return(0);

  s=0;
  m=0;
  myPID = (int) getpid();
  myHostName[0] = '\0';
  gethostname (myHostName, MAX_NAME_LEN);
  sprintf(condRead, "(*XUSER  == \"%s@%s\") && (*XHDR == \"STARTDEBUG\")",
	  svrComm->clientUser.userName, svrComm->clientUser.rodsZone);
  status = _readXMsg(GlobalREDebugFlag, condRead, &m, &s, &readhdr, &readmsg, &user, &addr);
  if (status >= 0) {
    if ( (readmsg != NULL)  && strlen(readmsg) > 0) {
      GlobalREDebugFlag = atoi(readmsg);
    }
    if (readhdr != NULL)
      free(readhdr);
    if (readmsg != NULL)
      free(readmsg);
    if (user != NULL)
      free(user);
    if (addr != NULL)
      free(addr);
    /* initialize reDebug stack space*/
    for (i = 0; i < REDEBUG_STACK_SIZE_FULL; i++) 
      reDebugStackFull[i] = NULL;
    for (i = 0; i < REDEBUG_STACK_SIZE_CURR; i++) {
      reDebugStackCurr[i][0] = NULL;
      reDebugStackCurr[i][1] = NULL;
      reDebugStackCurr[i][2] = NULL;
    }
    memset(breakPoints, 0, sizeof(struct Breakpoint) * 100);

    reDebugStackFullPtr = 0;
    reDebugStackCurrPtr = 0;
    snprintf(waitHdr,HEADER_TYPE_LEN - 1,   "idbug:");

    rodsLog (LOG_NOTICE,"reDebugInitialization: Got Debug StreamId:%i\n",GlobalREDebugFlag);
    snprintf(waitMsg, MAX_NAME_LEN, "PROCESS BEGIN at %s:%i. Client connected from %s at port %i\n", 
	     myHostName, myPID, svrComm->clientAddr,ntohs(svrComm->localAddr.sin_port));
    _writeXMsg(GlobalREDebugFlag, "idbug", waitMsg);
    snprintf(waitMsg, MAX_NAME_LEN, "%s:%i is waiting\n", myHostName, myPID);  }
  return(0);
}


int processXMsg(int streamId, int *msgNum, int *seqNum, 
		char * readhdr, char *readmsg, 
		char *callLabel, Node *node,
		Env *env, int curStat, ruleExecInfo_t *rei)
{

  char myhdr[HEADER_TYPE_LEN];
  char mymsg[MAX_NAME_LEN];
  char *outStr = NULL;
  char *ptr;
  int i,n;
  int iLevel, wCnt;
  int  ruleInx = 0;
  Region *r;
    Res *res;
    rError_t errmsg;
	errmsg.len = 0;
	errmsg.errMsg = NULL;
	r = make_region(0, NULL);
	ParserContext *context = newParserContext(&errmsg, r);
	Pointer *e = newPointer2(readmsg);
	int rulegen = 1;
    int found;
    int grdf[2];
    int cmd;
    snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug:%s",callLabel);
    PARSER_BEGIN(DbgCmd)
		TRY(DbgCmd)
			TTEXT2("n", "next");
			cmd = REDEBUG_STEP_OVER;
		OR(DbgCmd)
			TTEXT2("s", "step");
			cmd = REDEBUG_NEXT;
		OR(DbgCmd)
			TTEXT2("f", "finish");
			cmd = REDEBUG_STEP_OUT;
		OR(DbgCmd)
			TTEXT2("b", "break");
			TRY(Param)
				TTYPE(TK_TEXT);
				char *fn = strdup(token->text);
				int breakPointsInx2;
				for(breakPointsInx2=0;breakPointsInx2<100;breakPointsInx2++) {
					if(breakPoints[breakPointsInx2].actionName == NULL) {
						break;
					}
				}
				if(breakPointsInx2 == 100) {
					_writeXMsg(streamId, myhdr, "Maximum breakpoint count reached. Breakpoint not set.\n");
					cmd = REDEBUG_WAIT;
				} else {
				breakPoints[breakPointsInx2].actionName = fn;
				ptr = NULL;
				TRY(loc)
					TTYPE(TK_TEXT);
					ptr = (char *) malloc(sizeof(token->text) + 2);
					ptr[0] = 'f';
					strcpy(ptr+1, token->text);
					TTEXT(":");
					TTYPE(TK_INT);
					breakPoints[breakPointsInx2].base = ptr;
					breakPoints[breakPointsInx2].line = atoi(token->text);
					rodsLong_t range[2];
					char rulesFileName[MAX_NAME_LEN];
					getRuleBasePath(ptr, rulesFileName);

					FILE *file;
					/* char errbuf[ERR_MSG_LEN]; */
					file = fopen(rulesFileName, "r");
					if (file == NULL) {
							return(RULES_FILE_READ_ERROR);
					}
					Pointer *p = newPointer(file, ptr);


					if(getLineRange(p, breakPoints[breakPointsInx2].line, range) == 0) {
						breakPoints[breakPointsInx2].start = range[0];
						breakPoints[breakPointsInx2].finish = range[1];
					} else {
						breakPoints[breakPointsInx2].actionName = NULL;
					}
					deletePointer(p);
				OR(loc)
					TTYPE(TK_INT);
					if(node!=NULL) {
						breakPoints[breakPointsInx2].base = strdup(node->base);
						breakPoints[breakPointsInx2].line = atoi(token->text);
						rodsLong_t range[2];
						Pointer *p = newPointer2(breakPoints[breakPointsInx2].base);
						if(getLineRange(p, breakPoints[breakPointsInx2].line, range) == 0) {
							breakPoints[breakPointsInx2].start = range[0];
							breakPoints[breakPointsInx2].finish = range[1];
						} else {
							breakPoints[breakPointsInx2].actionName = NULL;
						}
						deletePointer(p);
					} else {
						breakPoints[breakPointsInx2].actionName = NULL;
					}
				OR(loc)
					/* breakPoints[breakPointsInx].base = NULL; */
				END_TRY(loc)

				if(ptr!=NULL && breakPoints[breakPointsInx2].base == NULL) {
					free(ptr);
				}
				if(breakPoints[breakPointsInx2].actionName != NULL)
					snprintf(mymsg, MAX_NAME_LEN, "Breakpoint %i  set at %s\n", breakPointsInx2,
							breakPoints[breakPointsInx2].actionName);
				else
					snprintf(mymsg, MAX_NAME_LEN, "Cannot set breakpoint, source not available\n");
				_writeXMsg(streamId, myhdr, mymsg);
				if(breakPointsInx <= breakPointsInx2) {
					breakPointsInx = breakPointsInx2 + 1;
				}
				cmd = REDEBUG_WAIT;
				}
			OR(Param)
				NEXT_TOKEN_BASIC;
				_writeXMsg(streamId, myhdr, "Unknown parameter type.\n");
				cmd = REDEBUG_WAIT;
			OR(Param)
				_writeXMsg(streamId, myhdr, "Debugger command \'break\' requires at least one argument.\n");
				cmd = REDEBUG_WAIT;
			END_TRY(Param)

		OR(DbgCmd)
			TTEXT2("w", "where");
			wCnt = 20;
			OPTIONAL_BEGIN(Param)
				TTYPE(TK_INT);
				wCnt = atoi(token->text);
			OPTIONAL_END(Param)
			iLevel = 0;

			i = reDebugStackCurrPtr - 1;
			while (i >=0 && wCnt > 0 && reDebugStackCurr[i][0] != NULL) {
				if (strstr(reDebugStackCurr[i][0] , "ExecAction") != NULL || strstr(reDebugStackCurr[i][0] , "ExecMicroSrvc") != NULL || strstr(reDebugStackCurr[i][0] , "ExecRule") != NULL) {
					snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug: Level %3i",iLevel);
					char *msg = (char *) malloc(strlen(reDebugStackCurr[i][0]) + strlen(reDebugStackCurr[i][1]) + strlen(reDebugStackCurr[i][2])+4);
					sprintf(msg, "%s:%s: %s", reDebugStackCurr[i][0], reDebugStackCurr[i][1], reDebugStackCurr[i][2]);
					_writeXMsg(streamId,  myhdr, msg);
					free(msg);
					if (strstr(reDebugStackCurr[i][0] , "ExecAction") != NULL)
						iLevel++;
					wCnt--;
				}
				i--;
			}
		OR(DbgCmd)
			TTEXT2("l", "list");
			TRY(Param)
				TTEXT2("r", "rule");
				TRY(ParamParam)
					TTYPE(TK_TEXT);
					ptr = strdup(token->text);

					mymsg[0] = '\n';
					mymsg[1] = '\0';
					snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug: Listing %s",ptr);
					RuleIndexListNode *node;
					found = 0;
					while (findNextRule2 (ptr, ruleInx, &node) != NO_MORE_RULES_ERR) {
						found = 1;
						if(node->secondaryIndex) {
							n = node->condIndex->valIndex->len;
							int i;
							for(i=0;i<n;i++) {
								Bucket *b = node->condIndex->valIndex->buckets[i];
								while(b!=NULL) {
									RuleDesc *rd = getRuleDesc(*(int *)b->value);
									char buf[MAX_RULE_LEN];
									ruleToString(buf, MAX_RULE_LEN, rd);
									snprintf(mymsg + strlen(mymsg), MAX_NAME_LEN - strlen(mymsg), "%i: %s\n%s\n", node->ruleIndex, rd->node->base[0] == 's'? "<source>":rd->node->base + 1, buf);
									b = b->next;
								}
							}
						} else {
							RuleDesc *rd = getRuleDesc(node->ruleIndex);
							char buf[MAX_RULE_LEN];
							ruleToString(buf, MAX_RULE_LEN, rd);
							snprintf(mymsg + strlen(mymsg), MAX_NAME_LEN - strlen(mymsg), "\n  %i: %s\n%s\n", node->ruleIndex, rd->node->base[0] == 's'? "<source>":rd->node->base + 1, buf);
						}
						ruleInx ++;
					}
					if (!found) {
						snprintf(mymsg, MAX_NAME_LEN,"Rule %s not found\n", ptr);
					}
					_writeXMsg(streamId, myhdr, mymsg);
						cmd = REDEBUG_WAIT;
					OR(ParamParam)
					_writeXMsg(streamId, myhdr, "Debugger command \'list rule\' requires one argument.\n");
					cmd = REDEBUG_WAIT;
					END_TRY(ParamParam)
			OR(Param)
				TTEXT2("b", "breakpoints");
				snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug: Listing %s",token->text);
				mymsg[0] = '\n';
				mymsg[1] = '\0';
				for(i=0;i<breakPointsInx;i++) {
					if(breakPoints[i].actionName!=NULL) {
						if(breakPoints[i].base!=NULL) {
							snprintf(mymsg + strlen(mymsg),MAX_NAME_LEN - strlen(mymsg), "Breaking at BreakPoint %i:%s %s:%d\n", i , breakPoints[i].actionName, breakPoints[i].base[0]=='s'?"<source>":breakPoints[i].base+1, breakPoints[i].line);
						} else {
							snprintf(mymsg + strlen(mymsg),MAX_NAME_LEN - strlen(mymsg), "Breaking at BreakPoint %i:%s\n", i , breakPoints[i].actionName);
						}
					}
				}
				_writeXMsg(streamId, myhdr, mymsg);
				cmd = REDEBUG_WAIT;
			OR(Param)
				TTEXT("*");
				snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug: Listing %s",token->text);
				Env *cenv = env;
				mymsg[0] = '\n';
				mymsg[1] = '\0';
				found = 0;
				while(cenv!=NULL) {
					n = cenv->current->size;
					for(i = 0;i<n;i++) {
						Bucket *b = cenv->current->buckets[i];
						while(b!=NULL) {
							if(b->key[0] == '*') { /* skip none * variables */
								found = 1;
								char typeString[128];
								typeToString(((Res *)b->value)->exprType, NULL, typeString, 128);
								snprintf(mymsg + strlen(mymsg), MAX_NAME_LEN - strlen(mymsg), "%s of type %s\n", b->key, typeString);
							}
							b = b->next;
						}
					}
					cenv = cenv->previous;
				}
				if (!found) {
					snprintf(mymsg + strlen(mymsg), MAX_NAME_LEN - strlen(mymsg), "<empty>\n");
				}
				_writeXMsg(streamId, myhdr, mymsg);
				cmd = REDEBUG_WAIT;
			OR(Param)
				syncTokenQueue(e, context);
				skipWhitespace(e);
				ABORT(lookAhead(e, 0) != '$');
				snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug: Listing %s",token->text);
				mymsg[0] = '\n';
				mymsg[1] = '\0';
				Hashtable *vars = newHashTable(100);
				for (i= 0; i < coreRuleVarDef .MaxNumOfDVars; i++) {
						if(lookupFromHashTable(vars, coreRuleVarDef.varName[i])==NULL) {
							snprintf(&mymsg[strlen(mymsg)], MAX_NAME_LEN - strlen(mymsg), "$%s\n", coreRuleVarDef.varName[i]);
							insertIntoHashTable(vars, coreRuleVarDef.varName[i], coreRuleVarDef.varName[i]);
						}
				}
				deleteHashTable(vars, NULL);
				_writeXMsg(streamId, myhdr, mymsg);
				cmd = REDEBUG_WAIT;
			OR(Param)
				NEXT_TOKEN_BASIC;
				_writeXMsg(streamId, myhdr, "Unknown parameter type.\n");
				cmd = REDEBUG_WAIT;
			OR(Param)
				_writeXMsg(streamId, myhdr, "Debugger command \'list\' requires at least one argument.\n");
				cmd = REDEBUG_WAIT;
			END_TRY(Param)
		OR(DbgCmd)
			TTEXT2("c", "continue");
			cmd = REDEBUG_STEP_CONTINUE;
		OR(DbgCmd)
			TTEXT2("C", "Continue");
			cmd = REDEBUG_CONTINUE_VERBOSE;
		OR(DbgCmd)
			TTEXT2("del", "delete");
			TRY(Param)
				TTYPE(TK_INT);
				n = atoi(token->text);
				if(breakPoints[n].actionName!= NULL) {
					free(breakPoints[n].actionName);
					if(breakPoints[n].base != NULL) {
						free(breakPoints[n].base);
					}
					breakPoints[n].actionName = NULL;
					breakPoints[n].base = NULL;
					snprintf(mymsg, MAX_NAME_LEN, "Breakpoint %i  deleted\n", n);
				} else {
					snprintf(mymsg, MAX_NAME_LEN, "Breakpoint %i  has not been defined\n", n);
				}
				_writeXMsg(streamId, myhdr, mymsg);
				cmd = REDEBUG_WAIT;
			OR(Param)
				_writeXMsg(streamId, myhdr, "Debugger command \'delete\' requires one argument.\n");
				cmd = REDEBUG_WAIT;
			END_TRY(Param)
		OR(DbgCmd)
			TTEXT2("p", "print");
			Node *n = parseTermRuleGen(e, 1, context);
			if(getNodeType(n) == N_ERROR) {
				errMsgToString(context->errmsg, mymsg + strlen(mymsg), MAX_NAME_LEN - strlen(mymsg));
			} else {
				snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug: Printing ");
				ptr = myhdr + strlen(myhdr);
				i = HEADER_TYPE_LEN - 1 - strlen(myhdr);
				termToString(&ptr, &i, 0, MIN_PREC, n, 0);
				snprintf(ptr, i, "\n");
				if(env != NULL) {
					disableReDebugger(grdf);
					res = computeNode(n, NULL, regionRegionCpEnv(env, r, (RegionRegionCopyFuncType *) regionRegionCpNode), rei, 0, &errmsg, r);
					enableReDebugger(grdf);
					outStr = convertResToString(res);
					snprintf(mymsg, MAX_NAME_LEN, "%s\n", outStr);
					free(outStr);
					if(getNodeType(res) == N_ERROR) {
						errMsgToString(&errmsg, mymsg + strlen(mymsg), MAX_NAME_LEN - strlen(mymsg));
					}
				} else {
					snprintf(mymsg, MAX_NAME_LEN, "Runtime environment: <empty>\n");
				}
			}
			_writeXMsg(streamId, myhdr, mymsg);

			cmd = REDEBUG_WAIT;
		OR(DbgCmd)
			TTEXT2("W", "Where");
			wCnt = 20;
			OPTIONAL_BEGIN(Param)
				TTYPE(TK_INT);
				wCnt = atoi(token->text);
			OPTIONAL_END(Param)
			iLevel = 0;

			i = reDebugStackCurrPtr - 1;
			while (i >=0 && wCnt > 0 && reDebugStackCurr[i][0] != NULL) {
				snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug: Level %3i",iLevel);
				char *msg = (char *) malloc(strlen(reDebugStackCurr[i][0]) + strlen(reDebugStackCurr[i][1]) + strlen(reDebugStackCurr[i][2])+4);
				sprintf(msg, "%s:%s: %s", reDebugStackCurr[i][0], reDebugStackCurr[i][1], reDebugStackCurr[i][2]);
				_writeXMsg(streamId,  myhdr, msg);
				free(msg);
				if (strstr(reDebugStackCurr[i][0] , "ExecAction") != NULL)
					iLevel++;
				wCnt--;
				i--;
			}
			cmd = REDEBUG_WAIT;
		OR(DbgCmd)
			TTEXT2("d", "discontinue");
			cmd = REDEBUG_WAIT;
		OR(DbgCmd)
			snprintf(mymsg, MAX_NAME_LEN, "Unknown Action: %s", readmsg);
			_writeXMsg(streamId, myhdr, mymsg);
			cmd = REDEBUG_WAIT;
		END_TRY(DbgCmd)
    PARSER_END(DbgCmd)
	freeRErrorContent(&errmsg);
		region_free(r);
		deletePointer(e);
    return cmd;
}

int
processBreakPoint(int streamId, int *msgNum, int *seqNum,
		  char *callLabel, char *actionStr, Node *node,
		  Env *env, int curStat, ruleExecInfo_t *rei)
{

  int i;
  char myhdr[HEADER_TYPE_LEN];
  char mymsg[MAX_NAME_LEN];

  snprintf(myhdr, HEADER_TYPE_LEN - 1,   "idbug:%s",callLabel);


  if (breakPointsInx > 0) {
    for (i = 0; i < breakPointsInx; i++) {
    	if (breakPoints[i].actionName!=NULL) {
    		int len = strlen(breakPoints[i].actionName);

       if(strncmp(actionStr, breakPoints[i].actionName, len) == 0 && !isalnum(actionStr[len])) {
    	  if(breakPoints[i].base != NULL &&
    		  (node == NULL || node->expr < breakPoints[i].start || node->expr >= breakPoints[i].finish ||
    				  strcmp(node->base, breakPoints[i].base)!=0)) {
			  continue;
    	  }
    	  char buf[MAX_NAME_LEN];
    	  snprintf(buf,MAX_NAME_LEN, "Breaking at BreakPoint %i:%s\n", i , breakPoints[i].actionName);
    	  generateErrMsg(buf, node->expr, node->base, mymsg);
    	  _writeXMsg(streamId, myhdr, mymsg);
    	  snprintf(mymsg,MAX_NAME_LEN, "%s\n", actionStr);
    	  _writeXMsg(streamId, myhdr, mymsg);
    	  curStat = REDEBUG_WAIT;
    	  return(curStat);
       }
      }
    }
  }
  return(curStat);
}


int
storeInStack(char *hdr, char *action, char* step)
{
  /* char *stackStr;
  char *stackStr2;
  char *s; */
  int i;


  /*stackStr = (char *) malloc(strlen(hdr) + strlen(step) + 5);
  sprintf(stackStr,"%s: %s\n", hdr, step);


  i = reDebugStackFullPtr;
  if (reDebugStackFull[i] != NULL) {
    free(reDebugStackFull[i]);
  }
  reDebugStackFull[i] = stackStr;
  reDebugStackFullPtr = (i + 1 ) % REDEBUG_STACK_SIZE_FULL;
  i = reDebugStackFullPtr;
  if (reDebugStackFull[i] != NULL) {
    free(reDebugStackFull[i]);
    reDebugStackFull[i] = NULL;
  }*/

  if (strcmp(action,"Done") == 0) { /* Pop the stack */
      i = reDebugStackCurrPtr - 1;
      /* if (i < 0)
    	  i = REDEBUG_STACK_SIZE_CURR - 1; */
      while (i >= 0 && reDebugStackCurr[i] != NULL && strcmp(reDebugStackCurr[i][0] , hdr) != 0) {
    	  free(reDebugStackCurr[i][0]);
    	  free(reDebugStackCurr[i][1]);
    	  free(reDebugStackCurr[i][2]);
    	  reDebugStackCurr[i][0] = NULL;
    	  reDebugStackCurr[i][1] = NULL;
    	  reDebugStackCurr[i][2] = NULL;
    	  i = i - 1;
    	  /*if (i < 0)
    		  i = REDEBUG_STACK_SIZE_CURR - 1; */
      }
      /*if (reDebugStackCurr[i] != NULL) {
      free(reDebugStackCurr[i]);
      reDebugStackCurr[i] = NULL;
      }*/
      reDebugStackCurrPtr = i;
      return(0);
  } else {

	  i = reDebugStackCurrPtr;
	  /*if (reDebugStackCurr[i] != NULL) {
		  free(reDebugStackCurr[i]);
	  }*/
	  if(i < REDEBUG_STACK_SIZE_CURR) {
		  reDebugStackCurr[i][0] = strdup(hdr);
		  reDebugStackCurr[i][1] = strdup(action);
		  reDebugStackCurr[i][2] = strdup(step);
		  reDebugStackCurrPtr = i + 1;
	  }
	  return(0);
  }
}


int sendWaitXMsg (int streamId) {
  _writeXMsg(streamId, waitHdr, waitMsg);
  return(0);
}
int cleanUpDebug(int streamId) {
  int i;
  for (i = 0 ; i < REDEBUG_STACK_SIZE_CURR; i++) {
    if (reDebugStackCurr[i] != NULL) {
      free(reDebugStackCurr[i][0]);
      free(reDebugStackCurr[i][1]);
      free(reDebugStackCurr[i][2]);
      reDebugStackCurr[i][0] = NULL;
      reDebugStackCurr[i][1] = NULL;
      reDebugStackCurr[i][2] = NULL;
    }
  }
  for (i = 0 ; i < REDEBUG_STACK_SIZE_FULL; i++) {
    if (reDebugStackFull[i] != NULL) {
      free(reDebugStackFull[i]);
      reDebugStackFull[i] = NULL;
    }
  }
  reDebugStackCurrPtr = 0;
  reDebugStackFullPtr = 0;
  GlobalREDebugFlag = -1;
  return(0);
}

int
reDebug(char *callLabel, int flag, char *action, char *actionStr, Node *node, Env *env, ruleExecInfo_t *rei)
{
  int i, m, s, status, sleepT, j;
  int processedBreakPoint = 0;
  char hdr[HEADER_TYPE_LEN];
  char *readhdr = NULL;
  char *readmsg = NULL;
  char *user = NULL;
  char *addr = NULL;
  static int mNum = 0;
  static int sNum = 0;
  static int curStat = 0; 
  static int reDebugStackPtr = -1;
  static char *reDebugAction = NULL;
  char condRead[MAX_NAME_LEN];
  char myActionStr[10][MAX_NAME_LEN + 10];
  int aNum = 0;
  char seActionStr[10 * MAX_NAME_LEN + 100];
  char timestamp[TIME_LEN];
  rsComm_t *svrComm;
  int waitCnt = 0;
  sleepT = 1;
  svrComm = rei->rsComm;

  if (svrComm == NULL) {
    rodsLog(LOG_ERROR, "Empty svrComm in REI structure for actionStr=%s\n",
	    actionStr);
    return(0);
  }

  generateLogTimestamp(timestamp, TIME_LEN);
  snprintf(hdr, HEADER_TYPE_LEN - 1,   "iaudit:%s:%s", timestamp, callLabel);
  condRead[0] = '\0'; 
  /* rodsLog (LOG_NOTICE,"PPP:%s\n",hdr); */
  snprintf(seActionStr, MAX_NAME_LEN + 10, "%s:%s", action, actionStr);
  if (GlobalREAuditFlag > 0) {
    if (flag == -4) {
      if (rei->uoic != NULL && rei->uoic->userName != NULL && rei->uoic->rodsZone != NULL) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  USER:%s@%s", rei->uoic->userName, rei->uoic->rodsZone);
	aNum++;
      }
      if (rei->doi != NULL && rei->doi->objPath != NULL && strlen(rei->doi->objPath) > 0 ) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  DATA:%s", rei->doi->objPath);
	aNum++;
      }
      if (rei->doi != NULL && rei->doi->filePath != NULL && strlen(rei->doi->filePath) > 0) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  FILE:%s", rei->doi->filePath);
	aNum++;
      }
      if (rei->doinp != NULL && rei->doinp->objPath != NULL && strlen(rei->doinp->objPath) > 0) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  DATAIN:%s",rei->doinp->objPath);
	aNum++;
      }
      if (rei->doi != NULL && rei->doi->rescName != NULL && strlen(rei->doi->rescName) > 0) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  RESC:%s",rei->doi->rescName);
	aNum++;
      }
      if (rei->rgi != NULL && rei->rgi->rescInfo->rescName != NULL && strlen(rei->rgi->rescInfo->rescName) > 0) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  RESC:%s",rei->rgi->rescInfo->rescName);
	aNum++;
      }
      if (rei->doi != NULL && rei->doi->rescGroupName != NULL && strlen(rei->doi->rescGroupName) > 0) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  RESCGRP:%s",rei->doi->rescGroupName);
	aNum++;
      }
      if (rei->rgi != NULL && rei->rgi->rescGroupName != NULL && strlen(rei->rgi->rescGroupName) > 0) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  RESCGRP:%s",rei->rgi->rescGroupName);
	aNum++;
      }
      if (rei->coi != NULL && rei->coi->collName != NULL) {
	snprintf(myActionStr[aNum],MAX_NAME_LEN + 10 , "  COLL:%s", rei->coi->collName);
	aNum++;
      }
      for (j = 0; j < aNum; j++) {
	strncat(seActionStr, myActionStr[j], 10 * MAX_NAME_LEN + 100);
      }
    }
  }

    /* Write audit trail */
	if (GlobalREAuditFlag == 3) {
		i = _writeXMsg(GlobalREAuditFlag, hdr, seActionStr);
	}
  

    /* Send current position for debugging */
    if ( GlobalREDebugFlag > 5 ) {
		/* store in stack */
		storeInStack(callLabel, action, actionStr);

		if (curStat == REDEBUG_CONTINUE && reDebugStackCurrPtr <= reDebugStackPtr && strcmp(action, reDebugAction) == 0) {
			curStat = REDEBUG_WAIT;
		}
		if (curStat != REDEBUG_CONTINUE) {
			char *buf = (char *)malloc(strlen(action)+strlen(actionStr)+2);
			sprintf(buf, "%s:%s", action, actionStr);
			snprintf(hdr, HEADER_TYPE_LEN - 1,   "idbug:%s",callLabel);
			i = _writeXMsg(GlobalREDebugFlag, hdr, buf);
			free(buf);
		}

		while ( GlobalREDebugFlag > 5 ) {
			s = sNum;
			m = mNum;
			/* what should be the condition */
			sprintf(condRead, "(*XSEQNUM >= %d) && (*XADDR != \"%s:%i\") && (*XUSER  == \"%s@%s\") && ((*XHDR == \"CMSG:ALL\") %%%% (*XHDR == \"CMSG:%s:%i\"))",
					s, myHostName, myPID, svrComm->clientUser.userName, svrComm->clientUser.rodsZone, myHostName, myPID);

			/*
			sprintf(condRead, "(*XSEQNUM >= %d)  && ((*XHDR == CMSG:ALL) %%%% (*XHDR == CMSG:%s:%i))",
				s,  myHostName, getpid());
			*/
			status = _readXMsg(GlobalREDebugFlag, condRead, &m, &s, &readhdr, &readmsg, &user, &addr);
			if (status == SYS_UNMATCHED_XMSG_TICKET) {
				cleanUpDebug(GlobalREDebugFlag);
				return(0);
			}
			if (status  >= 0) {
				rodsLog (LOG_NOTICE,"Getting XMsg:%i:%s:%s\n", s,readhdr, readmsg);
				curStat = processXMsg(GlobalREDebugFlag, &m, &s, readhdr, readmsg,
						callLabel,node,
						env, curStat, rei);
				if (readhdr != NULL)
					free(readhdr);
				if (readmsg != NULL)
					free(readmsg);
				if (user != NULL)
					free(user);
				if (addr != NULL)
					free(addr);

				mNum = m;
				sNum = s + 1;
				if (curStat == REDEBUG_WAIT) {
					sendWaitXMsg(GlobalREDebugFlag);
				} else
				if(curStat == REDEBUG_STEP_OVER) {
					reDebugStackPtr = reDebugStackCurrPtr;
					reDebugAction = "";
					curStat = REDEBUG_CONTINUE;
					break;
				} else
				if(curStat == REDEBUG_STEP_OUT) {
					reDebugStackPtr = reDebugStackCurrPtr - 1;
					reDebugAction = "Done";
					curStat = REDEBUG_CONTINUE;
					break;
				} else
				if(curStat == REDEBUG_STEP_CONTINUE) {
					reDebugStackPtr = -1;
					curStat = REDEBUG_CONTINUE;
					break;
				} else if (curStat == REDEBUG_NEXT )
					break;
			} else {
				if (!(curStat == REDEBUG_CONTINUE || curStat == REDEBUG_CONTINUE_VERBOSE)) {

/*#if _POSIX_C_SOURCE >= 199309L
					struct timespec time = {0, 100000000}, rem;
					nanosleep(&time, &rem);
					waitCnt+=10;
#else*/
					sleep(sleepT);
					waitCnt+=100;
/*#endif*/
					if (waitCnt > 6000) {
						sendWaitXMsg(GlobalREDebugFlag);
						waitCnt = 0;
					}
				}
			}
			if (curStat == REDEBUG_CONTINUE || curStat == REDEBUG_CONTINUE_VERBOSE) {
				if (processedBreakPoint == 1)
					break;
				if(strcmp(action, "") != 0 || strstr(callLabel, "ExecAction")==NULL)
					break;
				curStat = processBreakPoint(GlobalREDebugFlag, &m, &s,
					   callLabel, actionStr, node,
					   env, curStat, rei);
				processedBreakPoint = 1;
				if (curStat == REDEBUG_WAIT) {
					sendWaitXMsg(GlobalREDebugFlag);
					continue;
				} else
					break;
			}
		}
	}


    return(0);
}
