/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reGlobalsExtern.h"
#include "reHelpers1.h"

int
getActionRecoveryList(char *ruleAction, char *ruleRecovery,
		      char *actionArray[],char *recoveryArray[])
{
  int i,j,k;
  char *t1, *t2, *t3, *t4;
  char action[MAX_ACTION_SIZE];
  char recovery[MAX_ACTION_SIZE];

  i  = 0;
  t1 = ruleAction;
  t3 = ruleRecovery;
  action[0] = '\0';
  recovery[0] = '\0';
  j = getNextAction(t1, action, &t2);
  k = getNextAction(t3, recovery, &t4);
  while(j == 0) {
    /*
    actionArray[i] = strdup(action);
    recoveryArray[i] = strdup(recovery);
    */
    actionArray[i] = (char *) malloc(MAX_ACTION_SIZE);
    recoveryArray[i] = (char *) malloc(MAX_ACTION_SIZE);
    strcpy(actionArray[i],action);
    strcpy(recoveryArray[i],recovery);

    i++;
    if (i == MAX_ACTION_IN_RULE)
      return (MAX_NUM_OF_ACTION_IN_RULE_EXCEEDED);
    t1 = t2;
    t3 = t4;
    action[0] = '\0';
    recovery[0] = '\0';
    j = getNextAction(t1, action, &t2);
    if (k == 0)
      k = getNextAction(t3, recovery, &t4);
  }
  return(i);
}

int
parseAction(char *inAction,char *action, char *args[], int *argc)
{
  char  *t, *s;
  int i = 0;
  /*int j,k;*/
  char *act;
  char *e,  *t1;
  /*char t2;*/
  int found = 0;

  act = strdup(inAction);
  if ((t = strstr(act,"(")) != NULL) {
    *t = '\0';
    t++;
    s = t;
    e = t;
    strcpy(action,act);
    while  (found == 0) {
      if (i == MAX_NUM_OF_ARGS_IN_ACTION)
	return(MAX_NUM_OF_ARGS_IN_ACTION_EXCEEDED);
      if ((t = strstr(s,",")) != NULL ) {
	*t = '\0';
	t1 = t + 1;
	if (goodExpr(e) == 0) {
#if 0
	  args[i] = e;	/* XXXXX this is why act cannot be freed */
	  trimWS(args[i]);
	  trimQuotes(args[i]);
#else
          trimWS(e);
          trimQuotes(e);
          args[i] = strdup (e);
#endif
	  i++;
	  e=t1;
	  s=t1;
	}
	else {
	  *t = ',';
	  s = t1+1;
	}
      }
      else {
#if 0
	args[i] = e;
	args[i][strlen(args[i])-1] = '\0';
#else
	e[strlen(e) - 1]  = '\0';
	trimWS(e);
	trimQuotes(e);
        args[i] = strdup (e);
#endif
	i++;
	found = 1;
      }
    }
    *argc = i;
    /* XXXX This need to be freed */
    free (act);	/* fix mem leak mw 02/07/07 */
    return(0);
  }
  else {
    strcpy(action,act);
    *argc = 0;
    /* XXXX This need to be freed */
    free (act);	/* fix mem leak mw 02/07/07 */
    return(0);
  }
}

int
getNextAction(char *listOfAction, char *action, char **restPtr)
{

  /*  char *t, *s;*/

  if (listOfAction== NULL || strlen(listOfAction) == 0) {
    return(-1);
  }

  splitActions(listOfAction,action, restPtr);
  return(0);
}

int
splitActions(char *expr, char *expr1, char **expr2)
{

  int found;
  char *e, *t, *t1;
  found = 0;
  e = expr;
  while  (found == 0) {
    if ((t = strstr(e,"##")) != NULL ) {
      *t = '\0';
      t1 = t + 2;
      if (goodExpr(expr) == 0) {
	rstrcpy(expr1,expr,MAX_ACTION_SIZE);
        *expr2  = t1;
	found = 1;
      }
      else {
	*t = '#';
	e = t1;
      }
    }
    else {
      rstrcpy(expr1,expr,MAX_ACTION_SIZE);
      *expr2 = NULL;
      found = 1;
    }
  }
  trimWS(expr1);
  return(0);
}

