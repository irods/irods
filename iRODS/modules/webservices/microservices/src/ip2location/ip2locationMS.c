/**
 * @file ip2locationMS.c
 *
 * @brief	Gives the location information given an ipaddress
 *
 * This webservice can be used to find location information including lat/long for
 * a given ipaddress. The web service is provided by http://ws.fraudlabs.com/
 * 
 *
 * @author	Arcot Rajasekar / University of California, San Diego
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "rsApiHandler.h"
#include "ip2locationMS.h"
#include "ip2locationH.h" 
#include "ip2location.nsmap"


/**
 * \fn msiIp2location(msParam_t* inIpParam, msParam_t* inLocParam, msParam_t* outLocParam, ruleExecInfo_t* rei )
 *
 * \brief Web-service based microservice for converting IP address to hostname
 * 
 * \module webservices
 * 
 * \since pre-2.1
 * 
 * \author  Arcot Rajasekar
 * \date    2008-05
 * 
 * \brief  This microservice returns host name and details given an IP address,
 *  using the web service provided by http://ws.fraudlabs.com/. It executes a web 
 *  service to convert an IP address to a location.
 *
 * \usage See clients/icommands/test/rules3.0/ 
 *
 * \param[in] inIpParam - This msParam is of type STR_MS_T inputs an ip-address.
 * \param[in] inLocParam - This msParam is of type STR_MS_T is a license string provided by http://ws.fraudlabs.com/
 * \param[out] outLocParam - This msParam is of type STR_MS_T which is a host location information
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
msiIp2location(msParam_t* inIpParam, msParam_t* inLocParam, msParam_t* outLocParam, ruleExecInfo_t* rei )
{

  struct soap *soap = soap_new(); 
  struct _ns1__IP2Location ip;
  struct _ns1__IP2LocationResponse loc;
  char response[10000];
  RE_TEST_MACRO( "    Calling msiIp2location" );

  ip.IP = (char *) inIpParam->inOutStruct;
  ip.LICENSE = (char *) inLocParam->inOutStruct;

  soap_init(soap);
  soap_set_namespaces(soap, ip2location_namespaces);
  if (soap_call___ns1__IP2Location(soap, NULL, NULL, &ip, &loc) == SOAP_OK) {
    sprintf(response,"%s",soap->buf);
    fillMsParam( outLocParam, NULL,
		 STR_MS_T, response, NULL );
    free(loc.IP2LocationResult);
    return(0);
  }
  else {
    sprintf(response,"Error in execution of msiIp2location::%s\n",soap->buf);
    fillMsParam( outLocParam, NULL,
		 STR_MS_T, response, NULL );
    return(0);
  }
}

