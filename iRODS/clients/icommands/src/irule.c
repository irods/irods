/* 
 * irule - The irods utility to execute user composed rules.
*/

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "getUtil.h"
void usage ();

int
parseMsInputParam (int argc, char **argv, int optInd, int ruleGen,
		   execMyRuleInp_t *execMyRuleInp, char *inBuf);

int startsWith(char *str, char *prefix) {
	int i = 0;
	while(str[i]!='\0' && prefix[i]!='\0') {
		if(str[i] != prefix[i]) {
			return 0;
		}
		i++;
	}
	return prefix[i] == '\0';
}

int convertListToMultiString(char *strInput, int input) {
	if(strcmp(strInput, "null")==0) {
		return 0;
	}
	char *src = strdup(strInput);

	char *p = strInput;
	char *psrc = src;

	/* replace % with %% */
	while(*psrc!='\0') {
		if(*psrc == '%') {
			*(p++) = '%';
			*(p++) = '%';
			psrc++;
		} else {
			*(p++) = *(psrc++);
		}
	}
	*p = '\0';

	free(src);

	/* replace , with % and remove extra spaces */
	p = strInput;
	psrc = strInput;
	while(*psrc!='\0') {
		/* variable name */
		while(!isspace(*psrc) && *psrc != '=' && *psrc != ',' && *psrc != '\0') {
			*(p++) = *(psrc++);
		}

		/* skip spaces */
		while(isspace(*psrc)) {
			psrc++;
		}
		if(input) {
			if(*psrc == '=') {
				/* assignment */
				*(p++) = *(psrc++);

				int inString = 0;
				char delim = '\0';
				while(*psrc!='\0') {
					if(inString) {
						if(*psrc == delim) {
							inString = 0;
						} else if(*psrc == '\\') {
							*(p++) = *(psrc++);
							if(*psrc=='\0') {
								return -1;
							}
						}
						*(p++) = *(psrc++);
					} else {
						if(*psrc == ',') {
							*(p++) = '%';
							psrc++;
							break;
						} else {
							if(*psrc == '\'' || *psrc == '\"') {
								inString = 1;
								delim = *psrc;
							}
							*(p++) = *(psrc++);
						}
					}
				}
			} else {
				return -1;
			}
		} else {
			if(*psrc == '\0') {
				break;
			} else if (*psrc == ','){
				*(p++) = '%';
				psrc++;
			} else {
				return -1;
			}
		}
		/* skip spaces */
		while(isspace(*psrc)) {
			psrc++;
		}
	}
	*p = '\0';
	return 0;
}
void appendOutputToInput(msParamArray_t *inpParamArray, char **inpParamNames, int inpParamN, char **outParamNames, int outParamN) {
	int i, k, repeat = 0;
	for(i=0;i<outParamN;i++) {
		if(strcmp(outParamNames[i], "ruleExecOut")!= 0) {
			repeat = 0;
			for(k=0;k<inpParamN;k++) {
				if(strcmp(outParamNames[i], inpParamNames[k]) == 0) {
					repeat = 1;
					break;
				}
			}
			if(!repeat) {
				addMsParam (inpParamArray, outParamNames[i],STR_MS_T,strdup("unspeced"), NULL);
			}
		}

	}

}
int extractVarNames(char **varNames, char *outBuf) {
    if (outBuf == NULL || strcmp (outBuf, "null") == 0) {
    	return 0;
    }
    int n = 0;
	char *p = outBuf;
	char *psrc = p;

	for(;;) {
		if(*psrc == '=') {
			*psrc = '\0';
			varNames[n++] = strdup(p);
			*psrc = '=';
			while(*psrc!='\0' && *psrc!='%') psrc++;
			if(*psrc == '\0') {
				break;
			}
			p = psrc+1;

		} else if(*psrc == '%') {
			if(psrc[1] == '%') {
				psrc++;
			} else {
				*psrc = '\0';
				varNames[n++] = strdup(p);
				*psrc = '%';
				p = psrc+1;
			}
		} else if(*psrc == '\0') {
			varNames[n++] = strdup(p);
			break;
		}
		psrc++;
	}
	return n;
}

char *trimPrefix(char *str) {
	int i = 0;
	while(str[i]!=' ') {
		i++;
	}
	while(str[i]==' ') {
		i++;
	}
	memmove(str, str+i, strlen(str) + 1 - i);
	return str;
}

char *trimSpaces(char *str) {
	char *p = str;
	char *psrc = str;

	while(*psrc != '\0' && isspace(*psrc)) {
		psrc++;
	}

	while(*psrc != '\0') {
		*(p++) = *(psrc++);
	}

	p--;
	while(isspace(*p) && p - str >= 0) {
		p--;
	}

	p++;
	*p = '\0';

	return str;

}
int
printMsParamNew (msParamArray_t *outParamArray, int output)
{
    int i,j;
    keyValPair_t *kVPairs;
    tagStruct_t *tagValues;

    if (outParamArray == NULL) {
    	return (0);
    }
    for (i = 0; i < outParamArray->len; i++) {
    	msParam_t *msParam;

    	msParam = outParamArray->msParam[i];

    	if(strcmp(msParam->label, "ruleExecOut") == 0) {
    		continue;
    	}

    	if (msParam->label != NULL &&
    			msParam->type != NULL &&
    			msParam->inOutStruct != NULL) {
    		if (strcmp (msParam->type, STR_MS_T) == 0)
    			printf("%s = %s\n",msParam->label, (char *) msParam->inOutStruct);
    		else  if (strcmp (msParam->type, INT_MS_T) == 0)
    			printf("%s = %i\n",msParam->label,*(int *) msParam->inOutStruct);
    		else  if (strcmp (msParam->type, DOUBLE_MS_T) == 0)
    			printf("%s = %f\n",msParam->label,*(double *) msParam->inOutStruct);
    		else if (strcmp (msParam->type, KeyValPair_MS_T) == 0 ) {
    			kVPairs = (keyValPair_t *)msParam->inOutStruct;
    			printf ("KVpairs %s: %i\n", msParam->label, kVPairs->len);
    			for (j = 0; j < kVPairs->len; j++) {
    				printf ("       %s = %s\n",kVPairs->keyWord[j],
    						kVPairs->value[j]);
    			}
    		}
    		else if (strcmp (msParam->type, TagStruct_MS_T) == 0 ) {
    			tagValues = (tagStruct_t *) msParam->inOutStruct;
    			printf ("Tags %s: %i\n", msParam->label, tagValues->len);
    			for (j = 0; j < tagValues->len; j++) {
    				printf ("       AttName = %s\n", tagValues->keyWord[j]);
    				printf ("       PreTag  = %s\n", tagValues->preTag[j]);
    				printf ("       PostTag = %s\n", tagValues->postTag[j]);
    			}
    		}
    		else if (strcmp (msParam->type, ExecCmdOut_MS_T) == 0 && output) {
    			execCmdOut_t *execCmdOut;
    			execCmdOut = (execCmdOut_t *) msParam->inOutStruct;
    			if (execCmdOut->stdoutBuf.buf != NULL)
    				printf ("STDOUT = %s",(char *) execCmdOut->stdoutBuf.buf);
    			if (execCmdOut->stderrBuf.buf != NULL)
    				printf ("STRERR = %s", (char *) execCmdOut->stderrBuf.buf);
    		}
    		else {
    			printf ("%s: %s\n",msParam->label, msParam->type);
    		}
    	}
    	if (msParam->inpOutBuf != NULL) {
    		printf ("    outBuf: buf length = %d\n", msParam->inpOutBuf->len);
    	}

    }
    return (0);
}

int
main(int argc, char **argv) {
    int status;
    rodsEnv myEnv;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    char *optStr;
    execMyRuleInp_t execMyRuleInp;
    msParamArray_t *outParamArray = NULL;
    msParamArray_t msParamArray;
    int rulegen;

    int connFlag = 0;
    char saveFile[MAX_NAME_LEN];

    optStr = "ZhlvF:";
   
    status = parseCmdLineOpt (argc, argv, optStr, 1, &myRodsArgs);

    if (status < 0) {
	printf("Use -h for help.\n");
        exit (1);
    }
    if (myRodsArgs.help==True) {
        usage();
        exit(0);
    }

    memset (&execMyRuleInp, 0, sizeof (execMyRuleInp));
    memset (&msParamArray, 0, sizeof (msParamArray));
    execMyRuleInp.inpParamArray = &msParamArray;
    execMyRuleInp.condInput.len=0;

    if (myRodsArgs.test == True) {
       addKeyVal(&execMyRuleInp.condInput,"looptest","true");
    }
    if (myRodsArgs.file == True) { /* input file */
	FILE *fptr;
	int len;
	int gotRule = 0;
	char buf[META_STR_LEN];
	char *inpParamNames[1024];
	char *outParamNames[1024];
	int inpParamN = 0;
	int outParamN = 0;
	/*** RAJA ADDED TO USE INPUT FILE FROM AN iRODS OBJECT ***/
	if (!strncmp(myRodsArgs.fileString,"i:",2)) {
	  status = getRodsEnv (&myEnv);
	  
	  if (status < 0) {
	    rodsLogError (LOG_ERROR, status, "main: getRodsEnv error. ");
	    exit (1);
	  }
	  
	  conn = rcConnect (myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
			    myEnv.rodsZone, 0, &errMsg);
	  
	  if (conn == NULL) {
	    exit (2);
	  }
	  
	  status = clientLogin(conn);
	  if (status != 0) {
	    rcDisconnect(conn);
	    exit (7);
	  }
	  if (status == 0) {
	    char *myargv[3];
	    int myargc, myoptind;
	    rodsPathInp_t rodsPathInp;
	    connFlag = 1;
	    
	    myargv[0] = strdup(myRodsArgs.fileString+2);
	    myargv[1] = saveFile;
	    myargc = 2;
	    myoptind = 0;
	    char *fileType = strrchr(myRodsArgs.fileString, '.');
	    if(fileType == NULL) {
	        printf (
	          "Unsupported input file type\n");
	        exit(10);

	    }
		int rulegen;
		if(strcmp(fileType, ".r") == 0) {
			rulegen = 1;
		} else if(strcmp(fileType, ".ir") == 0 || strcmp(fileType, ".irb") == 0) {
			rulegen = 0;
		}
	    snprintf(saveFile,MAX_NAME_LEN,"/tmp/tmpiruleFile.%i.%i.%s",
	      (unsigned int) time(0),getpid(), rulegen?"r":"ir");
	    status = parseCmdLinePath (myargc,myargv,myoptind,&myEnv,
		       UNKNOWN_OBJ_T, UNKNOWN_FILE_T, 0, &rodsPathInp);
	    status = getUtil (&conn, &myEnv, &myRodsArgs, &rodsPathInp);
	    if (status < 0) {
	      rcDisconnect(conn);
	      exit (3);
	    }
	    myRodsArgs.fileString = saveFile;
	    connFlag = 1;
	  }
	}
	/*** RAJA ADDED TO USE INPUT FILE FROM AN iRODS OBJECT ***/

	fptr = fopen (myRodsArgs.fileString, "r");

        if (fptr == NULL) {
            rodsLog (LOG_ERROR,
              "Cannot open input file %s. ernro = %d\n",
              myRodsArgs.fileString, errno);
	    exit (1);
	}
    char *fileType = strrchr(myRodsArgs.fileString, '.');
    if(fileType == NULL) {
        printf (
          "Unsupported input file type\n");
        exit(10);

    }
	if(strcmp(fileType, ".r") == 0) {
		rulegen = 1;
	} else if(strcmp(fileType, ".ir") == 0 || strcmp(fileType, ".irb") == 0) {
		rulegen = 0;
	} else {
		rodsLog (LOG_ERROR,
          "Unsupported input file type %s\n", fileType);
        exit(10);

	}
	if(rulegen) {
		rstrcpy (execMyRuleInp.myRule, "@external\n", META_STR_LEN);
	}
	while ((len = getLine (fptr, buf, META_STR_LEN)) > 0) {
	    if (myRodsArgs.longOption == True)
	    	puts(buf);

	    if (!rulegen && buf[0] == '#') {
	    	continue;
	    }

	    if(rulegen) {
			if(startsWith(buf, "INPUT") || startsWith(buf, "input")) {
				gotRule = 1;
				trimSpaces(trimPrefix(buf));
			} else if(startsWith(buf, "OUTPUT") || startsWith(buf, "output")) {
				gotRule = 2;
				trimSpaces(trimPrefix(buf));
			}
	    }

		if (gotRule == 0) {
	    	if(!rulegen) {
				/* the input is a rule */
				snprintf (execMyRuleInp.myRule + strlen(execMyRuleInp.myRule), META_STR_LEN - strlen(execMyRuleInp.myRule), "%s\n", buf);
	    	} else {
	    		snprintf (execMyRuleInp.myRule + strlen(execMyRuleInp.myRule), META_STR_LEN - strlen(execMyRuleInp.myRule), "%s\n", buf);
	    	}
	    } else if (gotRule == 1) {
	    	if(rulegen) {
	    		if(convertListToMultiString(buf, 1)!=0) {
	    			rodsLog (LOG_ERROR,
	    					"Input parameter list format error for %s\n", myRodsArgs.fileString);
	    			exit(10);
	    		}
	    	}
	    	inpParamN = extractVarNames(inpParamNames, buf);
	    	parseMsInputParam (argc, argv, optind, rulegen, &execMyRuleInp, buf);
	    } else if (gotRule == 2) {
	    	if(rulegen) {
	    		if(convertListToMultiString(buf, 0)!=0) {
	    			rodsLog (LOG_ERROR,
	    					"Output parameter list format error for %s\n", myRodsArgs.fileString);
	    			exit(10);
	    		}
	    	}
	    	outParamN = extractVarNames(outParamNames, buf);
			if (strcmp (buf, "null") != 0) {
				rstrcpy (execMyRuleInp.outParamDesc, buf, LONG_NAME_LEN);
			}
	        break;
	    } else {
	    	break;
	    }
		if(!rulegen) {
			gotRule++;
		}
	}
	if (myRodsArgs.longOption == True)
	    puts("-----------------------------------------------------------------");

	if (gotRule != 2) {
	    rodsLog (LOG_ERROR, "Incomplete rule input for %s",
	      myRodsArgs.fileString);
	    exit (2);
	} 
	/*** RAJA ADDED TO USE INPUT FILE FROM AN iRODS OBJECT ***/
	if (connFlag == 1) {
	  fclose(fptr);
	  unlink(saveFile);
	}
	/*** RAJA ADDED TO USE INPUT FILE FROM AN iRODS OBJECT ***/
    } else {	/* command line input */
	int nArg = argc - optind;
        if (nArg < 3) {
            rodsLog (LOG_ERROR, "no input");
            printf("Use -h for help.\n");
            exit (3);
        }
	snprintf (execMyRuleInp.myRule, META_STR_LEN, "@external rule { %s }", argv[optind]);
	parseMsInputParam (0,NULL, 0, 1, &execMyRuleInp, argv[optind + 1]);
	if (strcmp (argv[optind + 2], "null") != 0) {
            rstrcpy (execMyRuleInp.outParamDesc, argv[optind + 2], 
	      LONG_NAME_LEN);
	}
    }


    if (connFlag == 0) {
      status = getRodsEnv (&myEnv);
      
      if (status < 0) {
        rodsLogError (LOG_ERROR, status, "main: getRodsEnv error. ");
        exit (1);
      }

      conn = rcConnect (myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
			myEnv.rodsZone, 0, &errMsg);
      
      if (conn == NULL) {
        rodsLogError (LOG_ERROR, errMsg.status, "rcConnect failure %s",
		      errMsg.msg);
        exit (2);
      }

      status = clientLogin(conn);
      if (status != 0) {
	rcDisconnect(conn);
	exit (7);
      }
    }
    if (myRodsArgs.verbose == True) {
        printf ("rcExecMyRule: %s\n", rulegen ? execMyRuleInp.myRule + 10 : execMyRuleInp.myRule);
	printf ("outParamDesc: %s\n", execMyRuleInp.outParamDesc);
    }

    status = rcExecMyRule (conn, &execMyRuleInp, &outParamArray);

   if (myRodsArgs.test == True) {
      printErrorStack (conn->rError);
    }
 
    if (status < 0) {
      msParam_t *mP;
      execCmdOut_t *execCmdOut;

      if(!rulegen) {
          rodsLogError (LOG_ERROR, status, "rcExecMyRule error. The rule engine is running under backward compatible mode. To run the rule(s) under normal mode, try renaming the file extension to \".r\". ");

      } else {
          rodsLogError (LOG_ERROR, status, "rcExecMyRule error. ");

      }
      printErrorStack (conn->rError); 
      if ((mP = getMsParamByType (outParamArray, ExecCmdOut_MS_T)) != NULL) {
	execCmdOut = (execCmdOut_t *) mP->inOutStruct;
	if (execCmdOut->stdoutBuf.buf != NULL) {
		fprintf(stdout,"%s",(char *) execCmdOut->stdoutBuf.buf);
	}
	if (execCmdOut->stderrBuf.buf != NULL) {
		fprintf(stderr,"%s", (char *) execCmdOut->stderrBuf.buf);
	}
      }
	rcDisconnect(conn);
	exit (4);
    }

    if (myRodsArgs.verbose == True) {
        printf ("ExecMyRule completed successfully.    Output \n\n");
        printMsParamNew (outParamArray, 1);
    }
    else {
        printMsParamNew (outParamArray, 0);
      msParam_t *mP;
      execCmdOut_t *execCmdOut;
      if ((mP = getMsParamByType (outParamArray, ExecCmdOut_MS_T)) != NULL) {
	execCmdOut = (execCmdOut_t *) mP->inOutStruct;
	if (execCmdOut->stdoutBuf.buf != NULL) 
	  fprintf(stdout,"%s",(char *) execCmdOut->stdoutBuf.buf);
	if (execCmdOut->stderrBuf.buf != NULL) 
	  fprintf(stderr,"%s", (char *) execCmdOut->stderrBuf.buf);
      }
    }
    if (myRodsArgs.verbose == True && conn->rError != NULL) {
	int i, len;
	rErrMsg_t *errMsg;
	len = conn->rError->len;
        for (i = 0;i < len; i++) {
            errMsg = conn->rError->errMsg[i];
            printf ("%s\n", errMsg->msg);
	}
    }

    rcDisconnect(conn);
    exit(0);	

}

int
splitMultiStr (char *strInput, strArray_t *strArray)
{
    char *startPtr, *endPtr;
    int endReached = 0;

   if (strInput == NULL || strArray == NULL) {
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }

    startPtr = endPtr = strInput;

    while (1) {
      /* RAJA changed JUl 11, 2007 so that two %% will be taken as an input %
	 instead of as a delimiter */
        while (*endPtr != '%' && *endPtr != '\0') {
            endPtr ++;
        }
        if (*endPtr == '%') {
        	if (*(endPtr+1) == '%') {
        		endPtr ++;endPtr ++;
        		continue;
        	}
        	*endPtr = '\0';
        } else {
            endReached = 1;
        }

        char *str = strdup(startPtr);
        char *p = str;
        char *psrc = str;
        while(*psrc!='\0') {
			while(*psrc!='%' && *psrc!='\0') *(p++) = *(psrc++);
			if(*psrc == '%') {
			*(p++) = *(psrc++);
			psrc++;
			}
        }
        *p = '\0';

        addStrArray (strArray, str);

        free(str);

        if (endReached == 1) {
            break;
        }

        endPtr++;
        startPtr = endPtr;
    }

    return (strArray->len);
}



int
parseMsInputParam (int argc, char **argv, int optInd, int ruleGen,
		   execMyRuleInp_t *execMyRuleInp, char *inBuf)
{
    strArray_t strArray;
    int status, i,j;
    char *value;
    int nInput;
    char line[MAX_NAME_LEN];
    int promptF = 0;
    int labelF = 0;
    if (inBuf == NULL || strcmp (inBuf, "null") == 0) {
	execMyRuleInp->inpParamArray = NULL;
	return (0);
    }

    nInput = argc - optInd;
    memset (&strArray, 0, sizeof (strArray));

    status = splitMultiStr (inBuf, &strArray);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "parseMsInputParam: parseMultiStr error, status = %d", status);
	execMyRuleInp->inpParamArray = NULL;
        return (status);
    }
    resizeStrArray (&strArray, MAX_NAME_LEN);
    value = strArray.value;
    /* each string is supposed to have to format label=value */
    for (i = 0; i < nInput; i++ ) {
      /* using the values from the input line following -F <filename> */
      if (!strcmp(argv[optInd + i],"prompt")) {
	promptF = 1;
	break;
      }
      if (!strcmp(argv[optInd + i],"default") || strlen(argv[optInd + i]) == 0) 
	continue;
      else if (*argv[optInd + i] == '*') {
	char *tmpPtr;
	if (i > 0 && labelF == 0)
	  return(CAT_INVALID_ARGUMENT);
	labelF = 1;
	if ((tmpPtr = strstr(argv[optInd + i],"=")) == NULL) 
	  return(CAT_INVALID_ARGUMENT);
	*tmpPtr = '\0';
	for (j = 0; j < strArray.len; j++) {
	  if (strstr(&value[j * strArray.size],argv[optInd + i]) == &value[j * strArray.size]){
	    *tmpPtr = '=';
#if 0	/* fix a not enugh space bug. MW */
	    rstrcpy (&value[j * strArray.size],argv[optInd + i],NAME_LEN);
#else
	    rstrcpy (&value[j * strArray.size],argv[optInd + i],strArray.size);
#endif
	    break;
	  }
	}
	if (j == strArray.len)
	  printf("Ignoring Argument \"%s\"",argv[optInd + i]);
      }
      else {
	char *valPtr = &value[i * strArray.size];
	char *tmpPtr;
	if (labelF == 1)
	  return(CAT_INVALID_ARGUMENT);
	if ((tmpPtr = strstr (valPtr, "=")) != NULL) {
	  tmpPtr++;
#if 0	/* fix a not enugh space bug. MW */
	  rstrcpy (tmpPtr,argv[optInd + i], 
	   (int) NAME_LEN - (tmpPtr - valPtr + 1));
#else
	  rstrcpy (tmpPtr,argv[optInd + i], 
	   strArray.size - (tmpPtr - valPtr + 1));
#endif
	}
      }
    }

    for (i = 0; i < strArray.len; i++) {
	char *valPtr = &value[i * strArray.size];
	char *tmpPtr;
	
	if ((tmpPtr = strstr (valPtr, "=")) != NULL) {
	    *tmpPtr = '\0';
	    tmpPtr++;
	    /** RAJA Jul 12 2007 changed it so that it can take input values from terminal
	    addMsParam (execMyRuleInp->inpParamArray, valPtr, STR_MS_T, 
	                strdup (tmpPtr), NULL);
            ** RAJA Jul 12 2007 changed it so that it can take input values from terminal **/
	    if (*tmpPtr == '$' ) {
	      /* If $ is used as a value in the input file for label=value then
		 the remaining command line arguments are taken as values.
		 If no command line arguments are given then the user is prompted
		 for the input value 
	      */
	      printf("Default %s=%s\n    New %s=",valPtr,tmpPtr+1,valPtr);
	      if (fgets(line,MAX_NAME_LEN,stdin) == NULL)
		return(CAT_INVALID_ARGUMENT);
	      if ((line[strlen(line)-1] = '\n')) 
		line[strlen(line)-1] = '\0';
	      if (strlen(line) == 0)
		addMsParam (execMyRuleInp->inpParamArray, valPtr,STR_MS_T,
			  strdup(tmpPtr+1), NULL);
	      else
		addMsParam (execMyRuleInp->inpParamArray, valPtr, STR_MS_T,
			  strdup(line), NULL);
	    }      
	    else if ( promptF == 1) {
	      /* the user has asked for prompting */
	      printf("Current %s=%s\n    New %s=",valPtr,tmpPtr,valPtr);
	      if (fgets(line,MAX_NAME_LEN,stdin) == NULL)
		return(CAT_INVALID_ARGUMENT);
	      if ((line[strlen(line)-1] = '\n')) 
		line[strlen(line)-1] = '\0';
	      if (strlen(line) == 0)
		addMsParam (execMyRuleInp->inpParamArray, valPtr,STR_MS_T,
			  strdup(tmpPtr), NULL);
	      else
		addMsParam (execMyRuleInp->inpParamArray, valPtr, STR_MS_T,
			  strdup(line), NULL);
	    }
	    else if (*tmpPtr == '\\') {
	      /* first '\'  is skipped. 
		 If you need to use '\' in the first letter add an additional '\'
		 if you have to use '$' in the first letter add a '\'  before that
	      */
	      tmpPtr++;
		char *param = strdup (tmpPtr);
		if(!ruleGen)
		trimQuotes(param);

	      addMsParam (execMyRuleInp->inpParamArray, valPtr, STR_MS_T,
			  param, NULL);
	    }
	    else {
                char *param = strdup (tmpPtr);
		if(!ruleGen)
                trimQuotes(param);

	      addMsParam (execMyRuleInp->inpParamArray, valPtr,STR_MS_T,
			  param, NULL);
	    }
	} else {
	    rodsLog (LOG_ERROR,
             "parseMsInputParam: inpParam %s format error", valPtr);
	}
    }

    return (0);
}
 


void 
usage ()
{
   char *msgs[]={
"Usage : irule [--test] [-v] rule inputParam outParamDesc",
"Usage : irule [--test] [-v] [-l] -F inputFile [prompt | arg_1 arg_2 ...]",
" ",
"Submit a user defined rule to be executed by an iRODS server.",
" ",
"The first form above requires 3 inputs: ",
"    1) rule - This is the rule to be executed.", 
"    2) inputParam - The input parameters. The input values for the rule is",
"       specified here. If there is no input, a string containing \"null\"",
"       must be specified.",
"    3) outParamDesc - Description for the set of output parameters to be ",
"       returned. If there is no output, a string containing \"null\"", 
"       must be specified.",
" ",
"The second form above reads the rule and arguments from the file: inputFile",
"    - Arguments following the inputFile are interpreted as input arguments for the rule.",
"    - These arguments are interpreted in two ways:",
"        1) The arguments require the \"label=value\" format.",
"            - Only named pairs are replaced. Any remaining arguments",
"                 get their values from the inputFile.  All labels start with *.",
"        2) All arguments can be specified as inputs without any labels",
"            - The keyword \"default\" can be given to use the inputFile value.",
"            - Use \\ as the first letter in an argument as an escape.",
" ",
"The inputFile should contain 3 lines of non-comments:",
"    1) The first line specifies the rule.",
"    2) The second line specifies the input arguments as label=value pairs separated by %",
"        - If % is needed in an input value, use %%.",
"        - An input value can begin with $. In such a case the user will be prompted.",
"             A default value can be listed after the $, as in *A=$40.",
"             If prompted, the default value will be used if the user",
"                presses return without giving a replacement.",
"         - The input line can just be the word null.",
"    3) The third line contains output parameters as labels separated by %.",
"         - The output line can just be the word null.",
" ",
"'arg_1' is of the form *arg=value.  For example, using *A=$ as one of",
"the arguments in line 2 of the inputFile, irule will prompt for *A or",
"the user can enter it on the command line:",
"    irule -F filename *A=23",
"     (integer parameters can be unquoted)",
"    or",
"    irule -F filename \"*A='/path/of/interest'\" *B=24",
"    irule -F filename \"*A=\\\"/path/of/interest\\\"\" *B=24",
"     (string parameters must be quoted)",
"     (your shell may require escaping and/or single quotes)",
" ",
"Example rules are given in:   clients/icommands/test/rules3.0/",
" ",
"In either form, the 'rule' that is processed is either a rule name or a",
"rule definition (which may be a complete rule or a subset).",
" ",
"To view the output parameters (outParamDesc), use the -v option.",
" ",
"Options are:",
" --test  - enable test mode so that the microservices are not executed,",
"             instead a loopback is performed",
" -F      - read the named inputFile",
"             if the inputFile begins with the prefix \"i:\"",
"             then the file is fetched from an iRODS server",
" -l      - list file if -F option is used",
" -v      - verbose",
" -h      - this help",
""};
   int i;
   for (i=0;;i++) {
      if (strlen(msgs[i])==0) break;
      printf("%s\n",msgs[i]);
   }
   printReleaseInfo("irule");
}
