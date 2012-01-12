/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file  nedMS.c
 *
 * @brief   Access to web services from NVO for NASA/IPAC Extragalactic Database (NED)
 *
 * These microservices handle communication with http://voservices.net/NED/ws_v2_0/NED.asmx
 * and provide answers to queries to the NED database. 
 * 
 * @author  Arcot Rajasekar / University of California, San Diego, Jan 2008
 */

#include "rsApiHandler.h"
#include "nedMS.h"
#include "nedH.h" 
#include "ned.nsmap"


/**
 * \fn msiObjByName(msParam_t* inObjByNameParam, msParam_t* outRaParam, msParam_t* outDecParam, msParam_t* outTypParam, ruleExecInfo_t* rei )
 *
 * \brief  This microservice executes a web service to retrieve astronomy image by name.
 *
 * \module webservices
 *
 * \since pre-2.1
 *
 * \author   Arcot Rajasekar
 * \date     2008-05
 * 
 * \note  It returns position and type of an astronomical object given a 
 *      name from the NASA/IPAC Extragalactic Database (NED) using web service at
 *      http://voservices.net/NED/ws_v2_0/NED.asmx
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inObjByNameParam - a msParam of type STR_MS_T which is an astronomical object name.
 * \param[out] outRaParam - a msParam of type STR_MS_T which is a Right Ascension as float as string.
 * \param[out] outDecParam - a msParam of type STR_MS_T which is a Declination as float as string.
 * \param[out] outTypParam - a msParam of type STR_MS_T which is a type of object (eg star, galaxy,...).
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
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int
msiObjByName(msParam_t* inObjByNameParam, 
	     msParam_t* outRaParam, 
	     msParam_t* outDecParam, 
	     msParam_t* outTypParam, 
	     ruleExecInfo_t* rei )
{

  struct soap *soap = soap_new(); 
  struct _ns1__ObjByName objByName; 
  struct _ns1__ObjByNameResponse objByNameResponse;
  char response [1000];


  
  RE_TEST_MACRO( "    Calling msiObjByName" );
  
  objByName.objname = (char *) inObjByNameParam->inOutStruct;

  soap_init(soap);
  soap_set_namespaces(soap, ned_namespaces);  

  if (soap_call___ns2__ObjByName(soap, NULL, NULL, &objByName, &objByNameResponse) == SOAP_OK) {
    sprintf(response,"%f",objByNameResponse.ObjByNameResult->ra);
    fillMsParam( outRaParam, NULL, STR_MS_T, response, NULL );
    sprintf(response,"%f",objByNameResponse.ObjByNameResult->dec);
    fillMsParam( outDecParam, NULL, STR_MS_T, response, NULL );
    fillMsParam( outTypParam, NULL,
		 STR_MS_T, objByNameResponse.ObjByNameResult->objtype, NULL );
    return(0);
  }
  else {
    snprintf(response,999, "Error in execution of msiObjByName::%s\n",soap->buf);
    fillMsParam( outTypParam, NULL,
		 STR_MS_T, response, NULL );
    return(0);
  }
}

