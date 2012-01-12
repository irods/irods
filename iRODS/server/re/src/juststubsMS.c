/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "reGlobalsExtern.h"
#include "icatHighLevelRoutines.h"
#include "rsGlobalExtern.h"
#include "dataObjCreate.h"
#include "objMetaOpr.h"
#include "regDataObj.h"
/* #include "reAction.h" */
#include "miscServerFunct.h"


int
msiGetClosestResourceToClient(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   Calling GetClosestResourceToClient\n");
    if (strlen(rei->uoic->authInfo.host) != 0 && strcmp(rei->uoic->authInfo.host,"null")) {
      sprintf(rei->doi->rescName,"closest-resource-to-%s",rei->uoic->authInfo.host);
      if (reLoopBackFlag > 0)
      return(0);
    }
    else {
      if (reLoopBackFlag > 0)
	return(ACTION_FAILED_ERR);
    }
  }
 /**** This is Just a Test Stub  ****/
  return(ACTION_FAILED_ERR);
}


int
msiNullAction(ruleExecInfo_t *rei)
{
  return(0);
}



int
msiDeleteData(ruleExecInfo_t *rei)
{

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == COMMAND_TEST_1 || reTestFlag == HTML_TEST_1) {
      print_doi(rei->doi);
    }
    else {
      rodsLog (LOG_NOTICE,"   Calling chlDelDataObj\n");
      print_doi(rei->doi);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  return(0);

}

int
recover_msiDeleteData(ruleExecInfo_t *rei)
{

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1)
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_chlDelDataObj\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  return(0);

}

int
msiInheritCollACL(ruleExecInfo_t *rei)
{

  /**** This is Just a Test Stub  ****/
  char logicalFileName[MAX_NAME_LEN];
  char logicalDirName[MAX_NAME_LEN];

  splitPathByKey(rei->doi->objPath, 
                              logicalDirName, logicalFileName, '/');
  if (reTestFlag > 0) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling chlInheritCollACL\n");
      rodsLog (LOG_NOTICE,"     From Coll = %s\n",logicalDirName);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  return(0);

}

int
recover_msiInheritCollACL(ruleExecInfo_t *rei)
{

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1)
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiInheritCollACL\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  return(0);

}

int
msiInheritDescriptiveMetadataFromColl(ruleExecInfo_t *rei)
{

  /**** This is Just a Test Stub  ****/
  char logicalFileName[MAX_NAME_LEN];
  char logicalDirName[MAX_NAME_LEN];

  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      splitPathByKey(rei->doi->objPath, 
		     logicalDirName, logicalFileName, '/');
      rodsLog (LOG_NOTICE,"   Calling chlInheritDescriptiveMetadataFromColl\n");
      rodsLog (LOG_NOTICE,"     From Coll = %s\n",logicalDirName);
    }
    if (reLoopBackFlag > 0)
    return(0);
  }
  /**** This is Just a Test Stub  ****/

  return(0);

}

int
recover_msiInheritDescriptiveMetadataFromColl(ruleExecInfo_t *rei)
{

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1)
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiInheritDescriptiveMetadataFromColl\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);

}
int
msiComputeChecksum(ruleExecInfo_t *rei)
{

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   Calling msiComputeChecksum\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  
  return(0);

}

int
recover_msiComputeChecksum(ruleExecInfo_t *rei)
{

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1)
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiComputeChecksum\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  return(0);

}


int
msiAddACLForDataToUser(msParam_t* xuser, msParam_t* xaccess, ruleExecInfo_t *rei)
{

  char *user, *access;

  user = (char *) xuser->inOutStruct;
  access= (char *) xaccess->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling msiAddACLForDataToUser to add Access Control\n");
      rodsLog (LOG_NOTICE,"     To     User = %s With Access = %s\n",user,access);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  return(0);

}

int
recover_msiAddACLForDataToUser(msParam_t* xuser, msParam_t* xaccess, ruleExecInfo_t *rei)
{

  char *user, *access;

  user = (char *) xuser->inOutStruct;
  access= (char *) xaccess->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1)
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiAddACLForDataToUser\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  return(0);

}




int 
msiTurnAuditTrailOn(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   Calling TurnAuditTrailOn\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  return(0);

}

int
recover_msiTurnAuditTrailOn(ruleExecInfo_t *rei)
{

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1)
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiTurnAuditTrailOn\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  return(0);
}

int 
msiTurnAuditTrailOff(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   Calling TurnAuditTrailOff\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  return(0);

}

int
recover_msiTurnAuditTrailOff(ruleExecInfo_t *rei)
{

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1)
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiTurnAuditTrailOff\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  return(0);
}

int
msiGetDefaultResourceForData(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   Calling GetDefaultResourceForData\n");
    strcpy(rei->doi->rescName,"default-resource");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  strcpy(rei->doi->rescName,"default-resource");
  return(0);
}

int msiExtractMetadataForFitsImage(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   Calling ExtractMetadataForFitsImage\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int msiExtractMetadataForDicomData(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   Calling ExtractMetadataForDicomData\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int msiExtractMetadataForEmail(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   Calling ExtractMetadataForEmail\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int msiExtractMetadataForCongressionalRecords(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   Calling ExtractMetadataForCongressionalRecords\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int msiCheckQuota(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   Calling CheckQuota\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int msiReplicateData(msParam_t* xrescName, ruleExecInfo_t *rei)
{
  char *rescName;

  rescName = (char *) xrescName->inOutStruct;
  
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling ReplicateData\n");
      rodsLog (LOG_NOTICE,"     in Resc = %\n",rescName);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int msiNotifyByEmail(msParam_t* xemail, msParam_t* xmessage, ruleExecInfo_t *rei)
{
  char *email;
  char *message;

  email = (char *) xemail->inOutStruct;
  message = (char *) xmessage->inOutStruct;


  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling msiNotifyByEmail\n");
      rodsLog (LOG_NOTICE,"     To Email=%s With Message =%s",email,message);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int msiLinkObjectToCollection(msParam_t* xcollName, ruleExecInfo_t *rei)
{
  char *collName;

  collName = (char *) xcollName->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling LinkObjectToCollection\n");
      rodsLog (LOG_NOTICE,"     To Collection=%s",collName);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int msiCreateThumbNail(msParam_t* xrescName, ruleExecInfo_t *rei)
{
  char *rescName;

  rescName = (char *) xrescName->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling CreateThumbNail\n");
      rodsLog (LOG_NOTICE,"     To Resc = %\n",rescName);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int msiCreateAbstract(msParam_t* xrescName, ruleExecInfo_t *rei)
{
  char *rescName;

  rescName = (char *) xrescName->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling CreateAbstract\n");
      rodsLog (LOG_NOTICE,"     To Resc = %\n",rescName);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}


int msiOutSource(msParam_t* xremoteHostInfo, msParam_t* xactionList, ruleExecInfo_t *rei)
{
  char *remoteHostInfo;
  char *actionList;

  remoteHostInfo = (char *) xremoteHostInfo->inOutStruct;
  actionList = (char *) xactionList->inOutStruct;

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == COMMAND_TEST_1) {
      fprintf(stdout,"  OutSourcing  Execution for %s to Host %s \n",actionList, remoteHostInfo);
    }
    else if (reTestFlag == HTML_TEST_1) {
      fprintf(stdout,">>>>OutSourcing Execution of %s to Host %s<BR>\n",actionList, remoteHostInfo);
    }
    else if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling msiOutSource\n");
      rodsLog (LOG_NOTICE,"     For Actions = %s\n",actionList);
      rodsLog (LOG_NOTICE,"     To  Host = %s\n",remoteHostInfo);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int msiAsynchExecution(msParam_t* xactionList, ruleExecInfo_t *rei)
{
  char *actionList;

  actionList = (char *) xactionList->inOutStruct;
  
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == COMMAND_TEST_1) {
      fprintf(stdout,"  Initiating Asynchronous   Execution for %s \n",actionList);
    }
    else if (reTestFlag == HTML_TEST_1) {
      fprintf(stdout,">>>>Initiating Asynchronous   Execution for %s<BR>\n",actionList);
    }
    else if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling msiAsynchExecution\n");
      rodsLog (LOG_NOTICE,"     For Actions = %s\n",actionList);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int msiParallelExecution(msParam_t* xactionList, ruleExecInfo_t *rei)
{
  char *actionList;

  actionList = (char *) xactionList->inOutStruct;
  
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == COMMAND_TEST_1) {
      fprintf(stdout,"  Initiating Parallel   Execution for %s \n",actionList);
    }
    else if (reTestFlag == HTML_TEST_1) {
      fprintf(stdout,">>>>Initiating Parallel   Execution for %s<BR>\n",actionList);
    }
    else if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling msiParallelExecution\n");
      rodsLog (LOG_NOTICE,"     For Actions = %s\n",actionList);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int orExec(msParam_t* xactionList, msParam_t *orSuccNum, ruleExecInfo_t *rei)
{
  char *actionList;

  actionList = (char *) xactionList->inOutStruct;
 
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == COMMAND_TEST_1) {
      fprintf(stdout,"  Initiating Disjunctive  Execution for %s \n",actionList);
    }
    else if (reTestFlag == HTML_TEST_1) {
      fprintf(stdout,">>>>Initiating Disjunctive  Execution for %s<BR>\n",actionList);
    }
    else if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling orExec\n");
      rodsLog (LOG_NOTICE,"     For Actions = %s\n",actionList);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}


int notExec(msParam_t* xactionList, ruleExecInfo_t *rei)
{
  
  char *actionList;

  actionList = (char *) xactionList->inOutStruct;

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == COMMAND_TEST_1) {
      fprintf(stdout,"  Initiating Negation  Execution for %s \n",actionList);
    }
    else if (reTestFlag == HTML_TEST_1) {
      fprintf(stdout,">>>>Initiating Negation Execution for %s<BR>\n",actionList);
    }
    else if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling notExec\n");
      rodsLog (LOG_NOTICE,"     For Actions = %s\n",actionList);
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}


int recover_msiExtractMetadataForFitsImage(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiExtractMetadataForFitsImage\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int recover_msiExtractMetadataForDicomData(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiExtractMetadataForDicomData\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int recover_msiExtractMetadataForEmail(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiExtractMetadataForEmail\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int recover_msiExtractMetadataForCongressionalRecords(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiExtractMetadataForCongressionalRecords\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int recover_msiCheckQuota(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiCheckQuota\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int recover_msiReplicateData(msParam_t* xrescName, ruleExecInfo_t *rei)
{
  char *rescName;

  rescName = (char *) xrescName->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiReplicateData\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int recover_msiNotifyByEmail(msParam_t* xemail, msParam_t* xmessage, ruleExecInfo_t *rei)
{
  char *email;
  char *message;

  email = (char *) xemail->inOutStruct;
  message = (char *) xmessage->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiNotifyByEmail\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int recover_msiLinkObjectToCollection(msParam_t* xcollName, ruleExecInfo_t *rei)
{
  char *collName;

  collName = (char *) xcollName->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiLinkObjectToCollection\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int recover_msiCreateThumbNail(msParam_t* xrescName, ruleExecInfo_t *rei)
{
  char *rescName;

  rescName = (char *) xrescName->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiCreateThumbNail\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int recover_msiCreateAbstract(msParam_t* xrescName, ruleExecInfo_t *rei)
{
  char *rescName;

  rescName = (char *) xrescName->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiCreateAbstract\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int recover_msiSendMail(msParam_t* xtoAddr, msParam_t* xsubjectLine, msParam_t* xbody, ruleExecInfo_t *rei)
{
  char *toAddr;
  char *subjectLine;
  char *body;

  toAddr = (char *) xtoAddr->inOutStruct;
  subjectLine = (char *) xsubjectLine->inOutStruct;
  body = (char *) xbody->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiSendMailCollByAdmin\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}



int recover_msiOutSource(msParam_t* xremoteHostInfo, msParam_t* xactionList, ruleExecInfo_t *rei)
{
  char *remoteHostInfo;
  char *actionList;

  remoteHostInfo = (char *) xremoteHostInfo->inOutStruct;
  actionList = (char *) xactionList->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiOutSource\n");
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}


int recover_msiAsynchExecution(msParam_t* xactionList, ruleExecInfo_t *rei)
{
  char *actionList;

  actionList = (char *) xactionList->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiAsynchExecution\n");
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}


int recover_msiParallelExecution(msParam_t* xactionList, ruleExecInfo_t *rei)
{
  char *actionList;

  actionList = (char *) xactionList->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiParallelExecution\n");
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int recover_orExec(msParam_t* xactionList, msParam_t *orSuccNum, ruleExecInfo_t *rei)
{
  char *actionList;

  actionList = (char *) xactionList->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_orExec\n");
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}

int recover_notExec(msParam_t* xactionList, ruleExecInfo_t *rei)
{
  char *actionList;

  actionList = (char *) xactionList->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_notExec\n");
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}
int writeExec(msParam_t* where, msParam_t* format, msParam_t* argsList, ruleExecInfo_t *rei)
{
  return(0);
}

int remoteExec(msParam_t* where, msParam_t* workflow, 
	       msParam_t* recoverWorkFlow, ruleExecInfo_t *rei)
{
  return(0);
}
int parallelExec(msParam_t* condition, msParam_t* workflow, 
		 msParam_t* recoverWorkFlow, ruleExecInfo_t *rei)
{
  return(0);
}
int oneOfExec(msParam_t* setOfWFS, msParam_t* recoverSetOfWFS,
	      ruleExecInfo_t *rei)
{
  return(0);
}
int someOfExec(msParam_t* number, msParam_t* setOfWFS, msParam_t* recoverSetOfWFS,
	      ruleExecInfo_t *rei)
{
  return(0);
}
int readExec( msParam_t* where, msParam_t* format, msParam_t* argsList, ruleExecInfo_t *rei)
{
  return(0);
}


int recover_msiCreateUser(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiCreateUser\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}


int recover_msiCreateCollByAdmin(msParam_t* xparColl, msParam_t* xchildName, ruleExecInfo_t *rei)
{
  char *parColl;
  char *childName;

  parColl = (char *) xparColl->inOutStruct;
  childName = (char *) xchildName->inOutStruct;
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) 
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiCreateCollByAdmin\n");
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  return(0);
}
int msiGetNewObjDescriptor(ruleExecInfo_t *rei)
{

  int i;

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling msiGetNewObjDescriptor\n");
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
  i = allocL1desc ();
  if (i < 0) 
    return(i);
  rei->l1descInx = i;
  i = fillL1desc(i, rei->doinp, rei->doi, NEWLY_CREATED_COPY,rei->doinp->dataSize);
  if (i < 0)
    return(i);
  return(0);
}

int msiPhyDataObjCreate(ruleExecInfo_t *rei)
{

  int status;
  dataObjInfo_t *myDataObjInfo;


  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling msiPhyDataObjCreate\n");
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/
    
  myDataObjInfo = L1desc[rei->l1descInx].dataObjInfo;

  status = getFilePathName(rei->rsComm, rei->doi, L1desc[rei->l1descInx].dataObjInp);
  if (status < 0)
    return(0);
  status = l3Create (rei->rsComm, rei->l1descInx);
  if (status <= 0) {
	rodsLog (LOG_NOTICE,
	  "msiPhyDataObjCreate: l3Create of %s failed, status = %d",
		 myDataObjInfo->filePath, status);
	return (status);
  } 
  else {
    L1desc[rei->l1descInx].l3descInx = status;
  }
  return(0);
}

int msiSetResourceList(ruleExecInfo_t *rei)
{
  int status;
  rescGrpInfo_t *myRescGrpInfo = NULL;

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   Calling msiSetResourceList\n");
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  /* I am just doing below what Mike wrote **/
  /* it might be a useful thing to geth this info from other rules
     and RCAT */
  status = getRescInfo (rei->rsComm, 
   NULL, &rei->doinp->condInput, &myRescGrpInfo);
  if (status < 0)
    return(status);
  sortResc (&myRescGrpInfo, &rei->doinp->condInput, NULL);
  rstrcpy (rei->doi->rescName, myRescGrpInfo->rescInfo->rescName, NAME_LEN);
  rei->rgi = myRescGrpInfo;
  return(0);
}


int recover_msiGetNewObjDescriptor(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiGetNewObjDescriptor\n");
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/


  freeL1desc (rei->l1descInx); /* free the  obj descriptor */
  return(0);
}

int recover_msiPhyDataObjCreate(ruleExecInfo_t *rei)
{

  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiPhyDataObjCreate\n");
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/


  /*WHAT SHOULD THIS DO:  remove the physical file from device***/
  

  return(0);
}



int recover_msiSetResourceList(ruleExecInfo_t *rei)
{
  /**** This is Just a Test Stub  ****/
  if (reTestFlag > 0 ) {
    if (reTestFlag == LOG_TEST_1) {
      rodsLog (LOG_NOTICE,"   ROLLBACK:Calling recover_msiSetResourceList\n");
    }
    if (reLoopBackFlag > 0)
      return(0);
  }
  /**** This is Just a Test Stub  ****/

  /* WHAT SHOULD THIS DO:  ????  */
  return(0);
}
