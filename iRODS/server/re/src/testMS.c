/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "reGlobalsExtern.h"
#include "icatHighLevelRoutines.h"

int
print_hello(ruleExecInfo_t *rei)
{
  /***
  rodsLog(LOG_NOTICE, "TTTTT Hello\n");
  rodsLogAndErrorMsg(LOG_NOTICE, &(rei->rsComm->rError),-1, "VVVVV Hello\n");
  ***/
  RE_TEST_MACRO ("Test for print_hello\n");
  fprintf(stdout, "Hello\n");
  _writeString("stdout", "Hello\n", rei);
  return(0);
}
int
recover_print_hello(ruleExecInfo_t *rei)
{
  RE_TEST_MACRO ("\b\b\b\b\b     \b\b\b\b\b");
    fprintf(stdout,"\b\b\b\b\b     \b\b\b\b\b");
  return(0);
}

int
print_doi(dataObjInfo_t *doi)
{
  if (reTestFlag == COMMAND_TEST_1) {
    fprintf(stdout,"     objPath = %s\n",doi->objPath);
    fprintf(stdout,"     rescName= %s\n",doi->rescName);
    fprintf(stdout,"     dataType= %s\n",doi->dataType);
    fprintf(stdout,"     dataSize= %lld\n",doi->dataSize);
  }
  else if(reTestFlag == HTML_TEST_1) {
    fprintf(stdout," <UL>\n");
    fprintf(stdout,"  <LI>     objPath = %s\n",doi->objPath);
    fprintf(stdout,"  <LI>     rescName= %s\n",doi->rescName);
    fprintf(stdout,"  <LI>     dataType= %s\n",doi->dataType);
    fprintf(stdout,"  <LI>     dataSize= %lld\n",doi->dataSize);
    fprintf(stdout," </UL>\n");
  }
  else {
    rodsLog (LOG_NOTICE,"     objPath = %s\n",doi->objPath);
    rodsLog (LOG_NOTICE,"     rescName= %s\n",doi->rescName);
    rodsLog (LOG_NOTICE,"     dataType= %s\n",doi->dataType);
    rodsLog (LOG_NOTICE,"     dataSize= %lld\n",doi->dataSize);
  }
  return(0);
}


int
print_uoi(userInfo_t *uoi)
{
  if (reTestFlag == COMMAND_TEST_1) {
    fprintf(stdout,"     userName = %s\n",uoi->userName);
    fprintf(stdout,"     rodsZone= %s\n",uoi->rodsZone);
    fprintf(stdout,"     userType= %s\n",uoi->userType);
  }
  else if(reTestFlag == HTML_TEST_1) {
    fprintf(stdout," <UL>\n");
    fprintf(stdout,"  <LI>     userName= %s\n",uoi->userName);
    fprintf(stdout,"  <LI>     rodsZone= %s\n",uoi->rodsZone);
    fprintf(stdout,"  <LI>     userType= %s\n",uoi->userType);

    fprintf(stdout," </UL>\n");
  }
  else {
    rodsLog (LOG_NOTICE,"     userName= %s\n",uoi->userName);
    rodsLog (LOG_NOTICE,"     rodsZone= %s\n",uoi->rodsZone);
    rodsLog (LOG_NOTICE,"     userType= %s\n",uoi->userType);

  }
  return(0);
}

int msiAW1(msParam_t* mPIn, msParam_t* mPOut2, ruleExecInfo_t *rei)
{
  char *In;
  char  *Out2;

  In  = (char *) mPIn->inOutStruct;

  Out2 = (char *) mPOut2->inOutStruct;

  rodsLog (LOG_NOTICE,"ALPHA: ------>  In:%s\n", In);
  mPOut2->type = strdup(STR_MS_T);
  mPOut2->inOutStruct = strdup("Microservice_1");
  return(0);

}


int msiCutBufferInHalf(msParam_t* mPIn, ruleExecInfo_t *rei)
{

  RE_TEST_MACRO ("Test for msiCutBufferInHalf\n");

  if (mPIn == NULL || mPIn->inpOutBuf == NULL ) {
    rodsLog (LOG_ERROR, "msiCutBufferInHalf: input is NULL.");
    return (USER__NULL_INPUT_ERR);
  }
  mPIn->inpOutBuf->len = (mPIn->inpOutBuf->len) / 2;
  return(0);

}


/**
 * \fn msiDoSomething(msParam_t *inParam, msParam_t *outParam, ruleExecInfo_t *rei)
 *
 * \brief Placeholder for microservice code to test.
 *
 * \module core
 *
 * \since 3.0.x
 *
 * \author  Antoine de Torcy
 * \date    2011-06-29
 *
 *
 * \note  This empty microservice is to be filled with your own code. It can serve as a
 * 			platform for quickly testing server API functions and related code. Input and
 * 			output parameters can be of any type, depending on how they are parsed and set up
 * 			in msiDoSomething. A quick and dirty way to examine critical variables without firing gdb
 * 			is to create a keyValPair_t*, dump data in it throughout the code with addKeyVal(), return
 * 			it through outParam and follow with writeKeyValPairs.
 *
 * \usage		doSomething {
 * 					msiDoSomething("", *keyValOut);
 * 					writeKeyValPairs("stdout", *keyValOut, ": ");
 * 					}
 * 				INPUT null
 * 				OUTPUT ruleExecOut
 *
 *
 * \param[in] inParam - Any type. A STR_MS_T can be used to pass multiple parameters
 * 				in the format keyWd1=value1++++keyWd2=value2++++keyWd3=value3...
 * \param[out] outParam - A KeyValPair_MS_T (by default).
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence None
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified None
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
 * \bug  no known bugs
**/
int
msiDoSomething(msParam_t *inParam, msParam_t *outParam, ruleExecInfo_t *rei)
{
	keyValPair_t *myKeyVal;						/* will contain results */

	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiDoSomething")

	/* Sanity checks */
	if (rei == NULL || rei->rsComm == NULL)
	{
		rodsLog (LOG_ERROR, "msiDoSomething: input rei or rsComm is NULL.");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* myKeyVal init */
	myKeyVal = (keyValPair_t*) malloc (sizeof(keyValPair_t));
	memset (myKeyVal, 0, sizeof(keyValPair_t));


	/***************************/
	/******** YOUR CODE ********/
	/***************************/

	/* Return myKeyVal through outParam */
	outParam->type = strdup(KeyValPair_MS_T);
	outParam->inOutStruct = (void*) myKeyVal;

	return 0;
}
