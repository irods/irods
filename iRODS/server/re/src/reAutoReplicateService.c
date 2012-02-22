/**
 * @file  reAutoReplicateService.c
 *
 */

#include "apiHeaderAll.h"
#include "rsApiHandler.h"
#include "objMetaOpr.h"
#include "dataObjOpr.h"


typedef struct __replicas_check_status_
{
   int registered;
   char checksum[200];
   int chksum_status;
   int repl_num;
   char resc[200];
}ReplicaCheckStatusStruct;

static int repl_storage_error;

static int process_single_obj(rsComm_t *conn, char *parColl, char *fileName,
        int required_num_replicas, char *grpRescForReplication, char *emailToNotify);
static int get_resource_path(rsComm_t *conn, char *rescName, char *rescPath);

void UnixSendEmail(char *toAddr, char *subjectLine, char *msgBody)
{
   char fileName[1024];
   char mailStr[100];
   FILE *fd;
   int t;
   char *t1, *t2;

   if((toAddr==NULL) || (strlen(toAddr) == 0))
     return;

   srand(time(0));
   t = rand()%281;
   sprintf(fileName, "/tmp/rodstmpmail%d.txt", t);

   fd = fopen(fileName, "w");
   if(fd == NULL)
     return;

#ifdef solaris_platform
   if((subjectLine != NULL) && (strlen(subjectLine) > 0))
   {
      fprintf(fd,"Subject:%s\n\n", subjectLine);
   }
#endif

   t1 = msgBody;
   while (t1 != NULL) {
      if ((t2 = strstr(t1,"\\n")) != NULL)
        *t2 = '\0';
      fprintf(fd,"%s\n",t1);
      if (t2 != NULL) {
        *t2 = '\\';
        t1 = t2+2;
      }
      else
        t1 = NULL;
    }
    fclose(fd);

#ifdef solaris_platform
    sprintf(mailStr,"cat %s| mail  %s",fileName,toAddr);
#else /* tested for linux - not sure how other platforms operate for subject */
    if((subjectLine != NULL) && (strlen(subjectLine) > 0))
      sprintf(mailStr,"cat %s| mail -s '%s'  %s",fileName, subjectLine, toAddr);
    else
      sprintf(mailStr,"cat %s| mail  %s",fileName, toAddr);
#endif
    system(mailStr);
    sprintf(mailStr,"rm %s",fileName);
    system(mailStr);
}

static int  _myAutoReplicateService(rsComm_t *conn, char *topColl, int recursiveFlag,
          int requiredNumReplicas, char *rescGroup,
          char *emailToNotify)
{
   /* query the collection. and go through each object and process each copy. */
   genQueryInp_t genQueryInp;
   int i;
   genQueryOut_t *genQueryOut = NULL;
   char query_str[2048];
   sqlResult_t *collNameStruct, *dataNameStruct;
   char *collName, *dataName;
   int t;
   int loop_stop;

   rodsLog(LOG_NOTICE,"_myAutoReplicateService(): topColl=%s, recursiveFlag=%d, requiredNumReplicas=%d, rescGroup=%s, emailToNotify=NULL\n",
           topColl, recursiveFlag, requiredNumReplicas, rescGroup);

   if((emailToNotify!=NULL)&&(strlen(emailToNotify)>0))
   {
      fprintf(stderr,"_myAutoReplicateService(): topColl=%s, recursiveFlag=%d, requiredNumReplicas=%d, rescGroup=%s, emailToNotify=%s\n", 
           topColl, recursiveFlag, requiredNumReplicas, rescGroup, emailToNotify);
   }

   if(recursiveFlag)
   {
      sprintf(query_str, "select COLL_NAME, DATA_NAME where COLL_NAME like '%s%%'", topColl);
   }
   else
   {
      sprintf(query_str, "select COLL_NAME, DATA_NAME where COLL_NAME = '%s'", topColl);
   }

   memset (&genQueryInp, 0, sizeof (genQueryInp_t));
   t = fillGenQueryInpFromStrCond(query_str, &genQueryInp);
   if (t < 0)
   {
      rodsLog (LOG_ERROR, "_myAutoReplicateService(): fillGenQueryInpFromStrCond() failed. err code=%d", t);
      return(t);
   }

   genQueryInp.maxRows= MAX_SQL_ROWS;
   genQueryInp.continueInx=0;
   t = rsGenQuery (conn, &genQueryInp, &genQueryOut);
   if (t < 0)
   {
      if(t == CAT_NO_ROWS_FOUND)   /* no data is found */
      {
         rodsLog(LOG_ERROR, "_myAutoReplicateService():rsGenQuery(): no data is found. The service ended.");
         return 0;
      }
      rodsLog(LOG_ERROR, "_myAutoReplicateService():rsGenQuery(): failed. err code=%d. The service quit.", t);
      return(t);
   } 

   repl_storage_error = 0;
   loop_stop = 0;
   do
   {
      /* fprintf(stderr, "rowCnt=%d\n", genQueryOut->rowCnt); */
      for(i=0;i<genQueryOut->rowCnt; i++) {
         collNameStruct = getSqlResultByInx (genQueryOut, COL_COLL_NAME);
         dataNameStruct = getSqlResultByInx (genQueryOut, COL_DATA_NAME);

         collName = &collNameStruct->value[collNameStruct->len*i];
         dataName = &dataNameStruct->value[dataNameStruct->len*i];

         t = process_single_obj(conn, collName, dataName, requiredNumReplicas, rescGroup, emailToNotify);
         rodsLog(LOG_DEBUG, "_myAutoReplicateService(): finished processing obj %s/%s\n", collName, dataName);

         if(t == SYS_OUT_OF_FILE_DESC) {   /* this is an fatal error. the service quit. */
            rodsLog(LOG_ERROR, "_myAutoReplicateService():process_single_obj() returned SYS_OUT_OF_FILE_DESC. The service quit.");
            return t;
         }

         if((t < 0) && (repl_storage_error == 1)) {
            rodsLog(LOG_ERROR, "_myAutoReplicateService():process_single_obj() returned a storage eror. The service quit.");
            loop_stop = 1;
         }
      }

      if(loop_stop ==0) {
         /* fprintf(stderr, "t=%d, continueInx=%d\n", t, genQueryOut->continueInx); */
         if(genQueryOut->continueInx == 0) {
            loop_stop = 1;
         }
         else {
            genQueryInp.continueInx=genQueryOut->continueInx;
            t = rsGenQuery (conn, &genQueryInp, &genQueryOut);
            loop_stop = 0;
         }
      }
   }
   while ((t == 0) && (loop_stop == 0));

   freeGenQueryOut(&genQueryOut);

   rodsLog(LOG_NOTICE,"_myAutoReplicateService(): topColl=%s ended.\n", topColl);
   
   return 0;
}

/**
 * \fn msiAutoReplicateService(msParam_t *xColl, msParam_t *xRecursive, 
 *         msParam_t *xRequireNumReplicas, msParam_t *xRescGroup, 
 *           msParam_t *xEmailAccountToNotify,
 *         ruleExecInfo_t *rei)
 *
 * \brief This microservice is used to handle digital preservation rule through checking data integrity and making necessary repair(s).
 *
 * \module core 
 *
 * \since 2.2
 *
 * \author  Bing Zhu
 * \date    2009-07
 *
 * \note   This microservice is supposed to be run as a periodic service to check if a designated number of 
 * required good copies of dataset(s) from a selected collection is in the system. 
 *   \li For a registered copy, it checks if the copy still exits. If the local file is removed by the data
 *     owner, the registered copy will be deleted from iRODS.
 *   \li For each replica, wether it is a registered dataset or a vaulted dataset, the service computes
 *     the checksum and verifies the replica is still good.
 *   \li If a bad copy is detected, the copy is deleted.
 *   \li Finally, the service creates necessary replicas to meet the required number of copies.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xColl - a STR_MS_T containing the collection or object name
 * \param[in] xRecursive - a STR_MS_T determining whether should be run recursively
 *                      \li true - will run recursively
 *                      \li false - default - will not run recursively
 * \param[in] xRequireNumReplicas - a STR_MS_T specifying the number of required replicas iRODS
 *                      \li must be at least 1
 * \param[in] xRescGroup - a STR_MS_T containing the target resource group name
 * \param[in] xEmailAccountToNotify - Optional - a STR_MS_T containing the notification email address
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence COL_D_DATA_CHECKSUM
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre none
 * \post none
 * \sa none
**/
int msiAutoReplicateService(msParam_t *xColl, msParam_t *xRecursive, 
          msParam_t *xRequireNumReplicas, msParam_t *xRescGroup, 
          msParam_t *xEmailAccountToNotify,
          ruleExecInfo_t *rei)
{
   char *sColl;
   char *sTmpstr;
   int  nRecursive;
   int  nRequiredNumOfReplica;
   char *sRescGroup;
   char *emailAccount;
   int t;

   sColl = (char *)xColl->inOutStruct;
   if((sColl == NULL) || (strcmp(sColl, "null")==0))
   {
      rodsLog (LOG_ERROR, "msiAutoReplicateService(): xColl is null.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }

   sTmpstr = (char *)xRecursive->inOutStruct;
   if((sTmpstr == NULL)|| (strcmp(sTmpstr, "null") == 0))
   {
      rodsLog (LOG_ERROR, "msiAutoReplicateService(): xRecursive is null.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }
   else
   {
      if(strcmp(sTmpstr, "true") == 0)
      {
         nRecursive = 1;
      }
      else
      {
         nRecursive = 0;
      }
   }

   sTmpstr = (char *)xRequireNumReplicas->inOutStruct;
   if((sTmpstr == NULL) || (strcmp(sTmpstr, "null") == 0))
   {
      rodsLog (LOG_ERROR, "msiAutoReplicateService(): xRequireNumReplicas is null.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }
   nRequiredNumOfReplica = atoi(sTmpstr);
   if(nRequiredNumOfReplica <= 0)
   {
      rodsLog (LOG_ERROR, "msiAutoReplicateService(): xRequireNumReplicas must be at least 1.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }

   emailAccount = (char *)xEmailAccountToNotify->inOutStruct;
   if((emailAccount==NULL) || (strcmp(emailAccount, "null")==0))
   {
      emailAccount = NULL;
   }

   sRescGroup = (char *)xRescGroup->inOutStruct;
   if(sRescGroup == NULL)
   {
      rodsLog(LOG_NOTICE, "msiAutoReplicateService(): sRescGroup is null.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }

   t = _myAutoReplicateService(rei->rsComm, sColl, nRecursive, nRequiredNumOfReplica, sRescGroup, emailAccount);

   return t;
}

static int process_single_obj(rsComm_t *conn, char *parColl, char *fileName,
        int required_num_replicas, char *grpRescForReplication, char *emailToNotify)
{
   genQueryInp_t genQueryInp;
   int i;
   genQueryOut_t *genQueryOut = NULL;
   sqlResult_t *replNumStruct, *rescStruct, *grpRescStruct, *dataPathStruct, *chkSumStruct;
   char *replNum, *rescName, *grpRescName, *dataPathName, *chkSum;
   int t;

   int i1a[10];
   int i1b[10];
   int i2a[10];
   char *condVal[2];
   char v1[200], v2[200];
   char vault_path[2048];

   char *chksum_str=NULL;
   dataObjInp_t myDataObjInp;
   dataObjInfo_t *myDataObjInfo=NULL;
   unregDataObj_t myUnregDataObjInp;
   transferStat_t *transStat=NULL;

   ReplicaCheckStatusStruct *pReplicaStatus;
   int nReplicas;

   char tmpstr[1024];

   int at_least_one_copy_is_good = 0;
   int newN;
   int rn;

   int validKwFlags;
   char *outBadKeyWd;
   msParam_t msGrpRescStr;   /* it can be a single resc name or a paired values. */

   /* fprintf(stderr,"msiAutoReplicateService():process_single_obj()\n");  */

   memset (&genQueryInp, 0, sizeof (genQueryInp_t));
   i1a[0]=COL_DATA_REPL_NUM;
   i1b[0]=0;
   i1a[1] = COL_D_RESC_NAME;
   i1b[1] = 0;
   i1a[2] = COL_D_RESC_GROUP_NAME;
   i1b[2] = 0;
   i1a[3] = COL_D_DATA_PATH;
   i1b[3] = 0;
   i1a[4] = COL_D_DATA_CHECKSUM;
   i1b[4] = 0;
   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = 5;

   i2a[0] = COL_COLL_NAME;
   i2a[1] = COL_DATA_NAME;
   genQueryInp.sqlCondInp.inx = i2a;
   sprintf(v1,"='%s'", parColl);
   condVal[0]=v1;
   sprintf(v2, "='%s'", fileName);
   condVal[1] = v2;
   genQueryInp.sqlCondInp.value = condVal;
   genQueryInp.sqlCondInp.len = 2;

   genQueryInp.maxRows= 10;
   genQueryInp.continueInx=0;
   t = rsGenQuery (conn, &genQueryInp, &genQueryOut);
   if(t < 0)
   {
      rodsLog(LOG_NOTICE,"msiAutoReplicateService():process_single_obj(): rsGenQuery failed errocode=%d", t);
      if(t == CAT_NO_ROWS_FOUND)   /* no data is found */
       return 0;

      return(t);
   }

   if(genQueryOut->rowCnt <= 0)
   {
      rodsLog(LOG_ERROR, "msiAutoReplicateService():process_single_obj(): return 0 record from calling rsGenQuery() for objid=%s/%s", parColl, fileName);
      return 0;
   }

   nReplicas = genQueryOut->rowCnt;
   pReplicaStatus = (ReplicaCheckStatusStruct *)calloc(nReplicas, sizeof(ReplicaCheckStatusStruct));

   for(i=0;i<nReplicas;i++)
   {
      pReplicaStatus[i].registered = 0;
      pReplicaStatus[i].checksum[0] = '\0';
   }

   for(i=0;i<genQueryOut->rowCnt; i++)
   {
      replNumStruct = getSqlResultByInx (genQueryOut, COL_DATA_REPL_NUM);
      replNum = &replNumStruct->value[replNumStruct->len*i];
      pReplicaStatus[i].repl_num = atoi(replNum);
   
      rescStruct = getSqlResultByInx (genQueryOut, COL_D_RESC_NAME);
      rescName = &rescStruct->value[rescStruct->len*i];
   
      grpRescStruct = getSqlResultByInx (genQueryOut, COL_D_RESC_GROUP_NAME);
      grpRescName = &grpRescStruct->value[grpRescStruct->len*i];
   
      dataPathStruct = getSqlResultByInx(genQueryOut, COL_D_DATA_PATH);
      dataPathName = &dataPathStruct->value[dataPathStruct->len*i];
   
      chkSumStruct = getSqlResultByInx (genQueryOut, COL_D_DATA_CHECKSUM);
      chkSum = &chkSumStruct->value[chkSumStruct->len*i];
   
      vault_path[0] = '\0';
      t = get_resource_path(conn, rescName, vault_path);
      if(t < 0)
      {
         rodsLog(LOG_NOTICE,"msiAutoReplicateService():process_single_obj():get_resource_path failed, status=%d", t);
		 free( pReplicaStatus ); // JMc cppcheck - leak
         return t;
      }
      
      if(strncmp(dataPathName, vault_path, strlen(vault_path)) != 0)
      {
         /* fprintf(stderr,"AB1-> %s/%s, v=%s, is a registered copy.\n", parColl,fileName, replNum); */
         pReplicaStatus[i].registered = 1;
      }
      else
      {
         pReplicaStatus[i].registered = 0;
      }

      if((chkSum != NULL)&&(strlen(chkSum) > 0))
      {
         strcpy(pReplicaStatus[i].checksum, chkSum);
      }
   }
   freeGenQueryOut(&genQueryOut);

   /* check replica status */
   at_least_one_copy_is_good = 0;
   for(i=0;i<nReplicas;i++)
   {
      /* check the file existence first */
      memset (&myDataObjInp, 0, sizeof(dataObjInp_t));
      snprintf (myDataObjInp.objPath, MAX_NAME_LEN, "%s/%s",parColl, fileName);
      myDataObjInp.openFlags = O_RDONLY;
      sprintf(tmpstr, "%d", pReplicaStatus[i].repl_num);
      addKeyVal (&myDataObjInp.condInput, REPL_NUM_KW, tmpstr);
      rn = pReplicaStatus[i].repl_num;
      t = rsDataObjOpen(conn, &myDataObjInp);
      if(t < 0)
      {
         /* fprintf(stderr,"%d. %s/%s, %d failed to open and has checksum status=%d\n", i, parColl, fileName, rn, t); */
         if(t == SYS_OUT_OF_FILE_DESC) {
		    free( pReplicaStatus ); // JMc cppcheck - leak
            return t;
         }
         else {
            pReplicaStatus[i].chksum_status = t;
         }
      }
      else {
         openedDataObjInp_t myDataObjCloseInp;
         memset (&myDataObjCloseInp, 0, sizeof(openedDataObjInp_t));
         myDataObjCloseInp.l1descInx = t;
         t = rsDataObjClose(conn, &myDataObjCloseInp);

         chksum_str =  NULL;
         /* compute checksum rsDataObjChksum() */
         memset(&myDataObjInp, 0, sizeof(dataObjInp_t));
         sprintf(myDataObjInp.objPath, "%s/%s", parColl, fileName);
         addKeyVal (&myDataObjInp.condInput, FORCE_CHKSUM_KW, "");
         sprintf(tmpstr, "%d", pReplicaStatus[i].repl_num);
         addKeyVal (&myDataObjInp.condInput, REPL_NUM_KW, tmpstr);
         t = rsDataObjChksum(conn, &myDataObjInp, &chksum_str);
         pReplicaStatus[i].chksum_status = t;     /* t == USER_CHKSUM_MISMATCH means a bad copy */
         /* fprintf(stderr,"%d. %s/%s, %d has checksum status=%d\n", i, parColl, fileName, rn, t); */
         if(t >= 0) {
            if(strlen(pReplicaStatus[i].checksum) > 0) {
               if(strcmp(pReplicaStatus[i].checksum, chksum_str) != 0)    /* mismatch */
               {
                  pReplicaStatus[i].chksum_status = USER_CHKSUM_MISMATCH;
               }
               else {
                  at_least_one_copy_is_good = 1;
               }
            }
            else {
               at_least_one_copy_is_good = 1;
            }
         }
      }
   }

   /* if there is none good copies left. In some casee, the checksum failed due
      to the fact that the server is down. We leave this case to be taken care of next time. */
   if(at_least_one_copy_is_good == 0)
   {
      rodsLog(LOG_ERROR, "msiAutoReplicateService():process_single_obj(): Obj='%s/%s': Wanring: The system detects that all copies might be corrupted.", parColl, fileName);

#ifndef windows_platform
      if((emailToNotify!=NULL)&&(strlen(emailToNotify)>0))
      {
         char msg_sub[1024], msg_body[1024];
         strcpy(msg_sub, "iRODS msiAutoReplicateService() error");
         sprintf(msg_body, "msiAutoReplicateService():process_single_obj(): Obj='%s/%s': at least one storage server is down or all copies are corrupted.",
                   parColl, fileName);
         UnixSendEmail(emailToNotify, msg_sub, msg_body);
      }
#endif

      free( pReplicaStatus ); // JMc cppcheck - leak
      return 0;
   }

   /* since we have at least one copy is good. We delete those bad copies (USER_CHKSUM_MISMATCH). */
   newN = nReplicas;
   for(i=0;i<nReplicas;i++)
   {
      memset(&myDataObjInp, 0, sizeof(dataObjInp_t));
      sprintf(myDataObjInp.objPath, "%s/%s", parColl, fileName);
      sprintf(tmpstr, "%d", pReplicaStatus[i].repl_num);
      addKeyVal (&myDataObjInp.condInput, REPL_NUM_KW, tmpstr);
      rn = pReplicaStatus[i].repl_num;
      if(pReplicaStatus[i].registered == 1)
      {
         /* here is the catch. iRODS. */
         int adchksum;
         /* fprintf(stderr,"CD->%s/%s, v=%d, is a registered copy.\n", parColl, fileName, rn); */
         adchksum = ((int)(pReplicaStatus[i].chksum_status/1000))*1000;
         if((pReplicaStatus[i].chksum_status == USER_CHKSUM_MISMATCH)||(pReplicaStatus[i].chksum_status==UNIX_FILE_OPEN_ERR)||(adchksum==UNIX_FILE_OPEN_ERR))
           /* USER_CHKSUM_MISMATCH  -> indicates the registered file is changed.
            * UNIX_FILE_OPEN_ERR --> indicates the registered file is removed by original owner.
            * -510002 is transformed from UNIX open error.
            */
         {
            rodsLog(LOG_NOTICE,"msiAutoReplicateService():process_single_obj(): registered copy will be removed: %s, repl=%d", myDataObjInp.objPath, rn);
            t = getDataObjInfo(conn, &myDataObjInp, &myDataObjInfo, NULL, 0);
            if(t >= 0) {
               myUnregDataObjInp.dataObjInfo = myDataObjInfo;
               myUnregDataObjInp.condInput = &myDataObjInp.condInput;
               t = rsUnregDataObj(conn, &myUnregDataObjInp);
               if(t >= 0) {
                  newN = newN -1;
               }
               else  {
                  rodsLog(LOG_ERROR, "msiAutoReplicateService():rsUnregDataObj(): failed for %s/%s:%d. erStat=%d", parColl, fileName, rn, t);
                  return t;
               }
            }
            else {
               rodsLog(LOG_ERROR, "msiAutoReplicateService():getDataObjInfo(): failed for %s/%s:%d. erStat=%d", parColl, fileName, rn, t);
               return t;
            }
         }
         else
         {
            rodsLog(LOG_ERROR,"%s:%d, the registered copy has errored checksum status=%d.", myDataObjInp.objPath, rn, pReplicaStatus[i].chksum_status);
            return t;
         }
      }
      else   /* the data file is in vault */
      {
         if(pReplicaStatus[i].chksum_status == USER_CHKSUM_MISMATCH)
         {
            t = rsDataObjUnlink(conn, &myDataObjInp);
            if(t >= 0) {
              newN = newN -1;
            }
            else  {
               rodsLog(LOG_ERROR, "msiAutoReplicateService():rsDataObjUnlink() for %s:%d failed. errStat=%d", myDataObjInp.objPath, rn, t);
               free( pReplicaStatus ); // JMc cppcheck - leak
               return t;
            }
         }
      }
   }

   fillStrInMsParam(&msGrpRescStr, grpRescForReplication);

   /* make necessary copies based on the required number of copies */
   if(newN < required_num_replicas)
   {
      rodsLog(LOG_NOTICE,"msiAutoReplicateService():process_single_obj(): making necessary %d copies as required.", (required_num_replicas-newN));
      for(i=0;i<(required_num_replicas-newN);i++)
      {
         memset(&myDataObjInp, 0, sizeof(dataObjInp_t));
         snprintf (myDataObjInp.objPath, MAX_NAME_LEN, "%s/%s", parColl, fileName);
         /* addKeyVal(&myDataObjInp.condInput, DEST_RESC_NAME_KW, grpRescForReplication); */
         validKwFlags = OBJ_PATH_FLAG | DEST_RESC_NAME_FLAG | NUM_THREADS_FLAG |
                        BACKUP_RESC_NAME_FLAG | RESC_NAME_FLAG | UPDATE_REPL_FLAG |
                        REPL_NUM_FLAG | ALL_FLAG | IRODS_ADMIN_FLAG | VERIFY_CHKSUM_FLAG |
                        RBUDP_TRANSFER_FLAG | RBUDP_SEND_RATE_FLAG | RBUDP_PACK_SIZE_FLAG;
         t = parseMsKeyValStrForDataObjInp(&msGrpRescStr, &myDataObjInp, DEST_RESC_NAME_KW, validKwFlags, &outBadKeyWd);
         if(t < 0)
         {
            if(outBadKeyWd != NULL)
            {
               rodsLog(LOG_ERROR, "msiAutoReplicateService():rsDataObjRepl(): input keyWd - %s error. status = %d", outBadKeyWd, t);
               free(outBadKeyWd);
            }
            else
            {
               rodsLog(LOG_ERROR, "msiAutoReplicateService():rsDataObjRepl(): input msKeyValStr error. status = %d", t);
            }

            free( pReplicaStatus ); // JMc cppcheck - leak
            return t;
         }        

         t = rsDataObjRepl(conn, &myDataObjInp, &transStat);
         if(t < 0)
         {
            rodsLog(LOG_ERROR, "msiAutoReplicateService():rsDataObjRepl() failed for %s/%s:%d into '%s'. err code=%d.", parColl, fileName, rn, grpRescForReplication, t);
            repl_storage_error = 1;
            free( pReplicaStatus ); // JMc cppcheck - leak
            return t;
         }
         if (transStat != NULL) free (transStat);
      }
   }

   free( pReplicaStatus ); // JMC cppcheck - leak
   return 0;
}

static int get_resource_path(rsComm_t *conn, char *rescName, char *rescPath)
{
   genQueryInp_t genQueryInp;
   int i1a[10];
   int i1b[10];
   int i2a[10];
   char *condVal[2];
   char v1[200];
   genQueryOut_t *genQueryOut = NULL;
   sqlResult_t *vaultPathSTruct;
   char *vaultPath;
   int t;

   memset (&genQueryInp, 0, sizeof (genQueryInp_t));

   i1a[0] = COL_R_VAULT_PATH;
   i1b[0]=0;
   genQueryInp.selectInp.inx = i1a;
   genQueryInp.selectInp.value = i1b;
   genQueryInp.selectInp.len = 1;

   i2a[0] = COL_R_RESC_NAME;
   genQueryInp.sqlCondInp.inx = i2a;
   sprintf(v1,"='%s'", rescName);
   condVal[0]=v1;
   genQueryInp.sqlCondInp.value = condVal;
   genQueryInp.sqlCondInp.len = 1;

   genQueryInp.maxRows= 2;
   genQueryInp.continueInx=0;
   t = rsGenQuery (conn, &genQueryInp, &genQueryOut);
   if(t < 0)
   {
      if(t == CAT_NO_ROWS_FOUND)   /* no data is found */
       return 0;

      return(t);
   } 

   if(genQueryOut->rowCnt < 0)
   {
      return -1;
   }

   vaultPathSTruct = getSqlResultByInx (genQueryOut, COL_R_VAULT_PATH);
   vaultPath = &vaultPathSTruct->value[0];
   strcpy(rescPath, vaultPath);

   freeGenQueryOut(&genQueryOut);

   return 0;
}

/**
 * \fn msiDataObjAutoMove(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3, 
 *                      msParam_t *inpParam4, msParam_t *inpParam5, ruleExecInfo_t *rei)
 *
 * \brief This microservice is used to automatically move the newly created file into a destination collection.
 *
 * \module core
 *
 * \since 2.2
 *
 * \author  Bing Zhu
 * \date    2009-07
 *
 * \note This microservice changes the ownership for the dataset(s) being moved.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inpParam1 - a STR_MS_T containing the object name with path. It usually comes from query as "$objPat
 *                          like /zone/../%" in the deployed microservice
 * \param[in] inpParam2 - a STR_MS_T containing the leading collection name to be truncated
 * \param[in] inpParam3 - a STR_MS_T containing the destination collection
 * \param[in] inpParam4 - a STR_MS_T containing the new owner
 * \param[in] inpParam5 - a STR_MS_T containing a flag for whether the checksum should be computed
                        \li true - default - will compute the checksum
                        \li false - will not compute the checksum
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 upon success
 * \pre none
 * \post none
 * \sa none
**/
int msiDataObjAutoMove(msParam_t *inpParam1, msParam_t *inpParam2, msParam_t *inpParam3, 
                       msParam_t *inpParam4, msParam_t *inpParam5, ruleExecInfo_t *rei)
{
   char *obj_path, *truct_path, *dest_coll, *new_owner;
   char *new_truct_path;
   char *new_obj_path;
   int  t;
   int  new_truct_path_len;
   rsComm_t *rsconn;
   char  mdest_coll[MAX_NAME_LEN];

   char  query_str[2048];
   genQueryInp_t genQueryInp;
   genQueryOut_t *genQueryOut = NULL;

   char new_obj_parent[MAX_NAME_LEN];
   char obj_name[MAX_NAME_LEN];

   collInp_t collCreateInp;
   dataObjCopyInp_t dataObjRenameInp;
   modAccessControlInp_t myModAccessCntlInp;
  
   dataObjInp_t myDataObjInp;
   char own_perm[20], null_perm[20];
   char user_name[NAME_LEN], zone_name[NAME_LEN];

   char *sTmpstr;
   int compute_checksum = 0;
   char *chksum_str=NULL;
   char tmpstr[1024];

   strcpy(own_perm, "own");
   strcpy(null_perm, "null");

   if (rei == NULL || rei->rsComm == NULL) {
        rodsLog (LOG_ERROR,
          "msiDataObjAutoMove: input rei or rei->rsComm is NULL");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
   }

   rsconn = rei->rsComm;

   if(inpParam1 == NULL) {
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: input objpath (inpParam1) is NULL.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }
   obj_path = (char *)inpParam1->inOutStruct;
   if((obj_path==NULL)||(strlen(obj_path)==0)) {
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: input objpath (inpParam1->inOutStruct) is NULL.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }

   if(inpParam2 == NULL) {
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: input truct_path (inpParam2) is NULL.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }
   truct_path = (char *)inpParam2->inOutStruct;
   if((truct_path==NULL)||(strlen(truct_path)==0)) {
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: input truct_path (inpParam2->inOutStruct) is NULL.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }

   if(inpParam3 == NULL) {
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: input dest_coll (inpParam3) is NULL.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }
   dest_coll = (char *)inpParam3->inOutStruct;
   if((dest_coll==NULL)||(strlen(dest_coll)==0)) {
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: input dest_coll (inpParam3->inOutStruct) is NULL.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }

   if(inpParam4 == NULL) {
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: input new_owner (inpParam4) is NULL.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }
   new_owner = (char *)inpParam4->inOutStruct;
   if(new_owner != NULL)
   {
      if(strlen(new_owner) == 0) {
         new_owner = NULL;
      }
      else if(strcmp(new_owner, "null") == 0) {
         new_owner = NULL;
      }
   }
   if(new_owner != NULL)
   {
      user_name[0] = '\0';
      zone_name[0] = '\0';
      t = parseUserName(new_owner, user_name, zone_name);
      if(t < 0) {
         rodsLog(LOG_ERROR, "msiDataObjAutoMove: parseUserName() failed. errStatus=%d.", t);
         return t;
      }
      if(strlen(zone_name) == 0) {
         strcpy(zone_name, rei->uoip->rodsZone);
      }
   }

   if(inpParam5 == NULL) {
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: input compute_checksum (inpParam5) is NULL.");
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }
   sTmpstr = (char *)inpParam5->inOutStruct;
   compute_checksum = 1;   /* default to true */
   if((sTmpstr != NULL)&&(strlen(sTmpstr) >= 0))
   {
      if(strcmp(sTmpstr, "false") == 0)
        compute_checksum = 0;
   }

   if(compute_checksum == 1) {
      chksum_str =  NULL;
      memset(&myDataObjInp, 0, sizeof(dataObjInp_t));
      strncpy(myDataObjInp.objPath, obj_path, MAX_NAME_LEN);
      addKeyVal (&myDataObjInp.condInput, VERIFY_CHKSUM_KW, "");
      sprintf(tmpstr, "%d", 0);
      addKeyVal (&myDataObjInp.condInput, REPL_NUM_KW, tmpstr);
      t = rsDataObjChksum(rsconn, &myDataObjInp, &chksum_str);
      if(t < 0) {
         rodsLog(LOG_ERROR, "msiDataObjAutoMove: rsDataObjChksum() for '%s' failed. errStatus=%d.", obj_path, t);
         return t;
      }
   }

   if(new_owner != NULL)
   {
      /* add ownership */
      memset(&myModAccessCntlInp, 0, sizeof(modAccessControlInp_t));
      myModAccessCntlInp.recursiveFlag = False;
      myModAccessCntlInp.accessLevel = own_perm;
      myModAccessCntlInp.userName = user_name;
      myModAccessCntlInp.zone = zone_name;
      myModAccessCntlInp.path = obj_path;
      t = rsModAccessControl(rsconn, &myModAccessCntlInp);
      if(t < 0) {
         rodsLog(LOG_ERROR, "msiDataObjAutoMove: rsModAccessControl() add new owner for '%s' failed. errStatus=%d.", obj_path, t);
         return t;
      }

   }

   t = strlen(truct_path);
   new_truct_path = (char *)calloc(t+2, sizeof(char));
   if(truct_path[t-1] != '/') {
      strcpy(new_truct_path, truct_path);
      new_truct_path_len = t;
   }
   else {
      strcpy(new_truct_path, truct_path);
      new_truct_path[t] = '/';
      new_truct_path[t+1] = '\0';
      new_truct_path_len = t + 1;
   }
   if(strncmp(new_truct_path, obj_path, t) != 0) {
      /* when the object is not match, we don't move */
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: The object path, %s, is not in the specified collection, %s.", obj_path, new_truct_path);
      return SYS_INTERNAL_NULL_INPUT_ERR;
   }

   t = strlen(dest_coll);
   new_obj_path = (char *)calloc(t+strlen(obj_path), sizeof(char));
   strcpy(mdest_coll, dest_coll);
   if(dest_coll[t-1] == '/')
   {
      mdest_coll[t-1] = '\0';
   }
   sprintf(new_obj_path, "%s/%s", mdest_coll, &(obj_path[new_truct_path_len+1]));
   sprintf(query_str, "SELECT COLL_NAME WHERE COLL_NAME like '%s%%'", mdest_coll);

   /* check if the dest_coll exists */
   memset (&genQueryInp, 0, sizeof (genQueryInp_t));
   t = fillGenQueryInpFromStrCond(query_str, &genQueryInp);
   if(t < 0)
   {
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: fillGenQueryInpFromStrCond() failed. errStatus=%d", t);
	  free( new_obj_path ); // JMC cppcheck - leak
	  free( new_truct_path ); // JMC cppcheck - leak
      return t;
   }
   genQueryInp.maxRows= MAX_SQL_ROWS;
   genQueryInp.continueInx=0;
   t = rsGenQuery(rsconn, &genQueryInp, &genQueryOut);
   if(t < 0) {
      if(t == CAT_NO_ROWS_FOUND) {
         rodsLog(LOG_ERROR, "msiDataObjAutoMove: The destination collection '%s' does not exist.", dest_coll);
      }
      else {
         rodsLog(LOG_ERROR, "msiDataObjAutoMove: rsGenQuery() failed. errStatus=%d", t);
      }
	  free( new_obj_path ); // JMC cppcheck - leak
	  free( new_truct_path ); // JMC cppcheck - leak
      return t;
   }

   /* separate new_obj_path with path and name */
   t = splitPathByKey(new_obj_path, new_obj_parent, obj_name, '/');
   if(t < 0) {
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: splitPathByKey() failed for splitting '%s'. errStatus=%d.", new_obj_path, t);
	  free( new_obj_path ); // JMC cppcheck - leak
	  free( new_truct_path ); // JMC cppcheck - leak
      return t;
   }
   
   /* fprintf(stderr,"msiDataObjAutoMove: newpar=%s, obj_name=%s, from=%s\n", new_obj_parent, obj_name, obj_path); */

   /* create the dires in new_obj_path 'imkidr -p'*/
   if(strlen(new_obj_parent) > strlen(mdest_coll))
   {
      memset (&collCreateInp, 0, sizeof (collCreateInp));
      rstrcpy (collCreateInp.collName, new_obj_parent, MAX_NAME_LEN);
      addKeyVal (&collCreateInp.condInput, RECURSIVE_OPR__KW, "");    /* always have '-p' option. */
      t = rsCollCreate(rsconn, &collCreateInp);
      if(t < 0)
      {
         rodsLog(LOG_ERROR, "msiDataObjAutoMove: rsCollCreate() failed for %s. errStatus=%d.", new_obj_parent, t);
	     free( new_obj_path ); // JMC cppcheck - leak
	  free( new_truct_path ); // JMC cppcheck - leak
         return t;
      }
   }

   fprintf(stderr,"new_obj_path=%s, obj_path=%s\n", new_obj_path, obj_path);
   /* renamed the obj_path to new_obj_path */
   memset(&dataObjRenameInp, 0, sizeof(dataObjCopyInp_t));
   rstrcpy(dataObjRenameInp.destDataObjInp.objPath, new_obj_path, MAX_NAME_LEN);
   rstrcpy(dataObjRenameInp.srcDataObjInp.objPath, obj_path, MAX_NAME_LEN);
   t = rsDataObjRename(rsconn, &dataObjRenameInp);
   if(t < 0) {
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: rsDataObjRename() failed. errStatus=%d.", t);
	  free( new_obj_path ); // JMC cppcheck - leak
	  free( new_truct_path ); // JMC cppcheck - leak
      return t;
   }

   memset(&myModAccessCntlInp, 0, sizeof(modAccessControlInp_t));
   myModAccessCntlInp.recursiveFlag = False;
   myModAccessCntlInp.accessLevel = null_perm;
   myModAccessCntlInp.userName = rei->uoic->userName;
   myModAccessCntlInp.zone = zone_name;
   myModAccessCntlInp.path = new_obj_path;
   t = rsModAccessControl(rsconn, &myModAccessCntlInp);
   if(t < 0) {
      rodsLog(LOG_ERROR, "msiDataObjAutoMove: rsModAccessControl() remove user for '%s' failed. errStatus=%d.", obj_path, t);
   }

   /* set new owner for the irods file */
#if 0
   if(new_owner != NULL)
   {
      if(strlen(new_obj_parent) > strlen(mdest_coll))  /* a collection */
      {
         char *ta, *tb;
         char ttab_coll[MAX_NAME_LEN];
         char ttab[MAX_NAME_LEN];

         strcpy(ttab, new_obj_parent);
         t = strlen(mdest_coll);
         ta = &(ttab[t+1]);
         tb = strchr(ta, '/');
         if(tb != NULL)
           *tb = '\0';
         sprintf(ttab_coll, "%s/%s", mdest_coll, ta);  /* ta has the first coll name under 'mdest_coll'. */

         memset(&myModAccessCntlInp, 0, sizeof(modAccessControlInp_t));
         myModAccessCntlInp.recursiveFlag = True;
         myModAccessCntlInp.accessLevel = own_perm;
         myModAccessCntlInp.userName = user_name;
         myModAccessCntlInp.zone = zone_name;
         myModAccessCntlInp.path = ttab_coll;
         t = rsModAccessControl(rsconn, &myModAccessCntlInp);
         if(t < 0) {
            rodsLog(LOG_ERROR, "msiDataObjAutoMove: rsModAccessControl() for '%s' failed. errStatus=%d.", ttab_coll, t);
            return t;
         }
      }
   }
#endif

   free( new_truct_path ); // JMC cppcheck - leak
   return 0;
}
