/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	stockQuoteMS.c
 *
 * @brief	Access to stock quotation web services
 *
 * This microservice handles comminication with http://www.webserviceX.NET
 * and provides stock quotation - delayed by the web server. 
 * 
 *
 * @author	Arcot Rajasekar / University of California, San Diego
 */

#include "rsApiHandler.h"
#include "stockQuoteMS.h"
#include "stockQuoteH.h" 
#include "stockQuote.nsmap"


/**
 * \fn msiGetQuote(msParam_t* inSymbolParam, msParam_t* outQuoteParam, ruleExecInfo_t* rei )
 *
 * \brief  Returns stock quotation (delayed by web service) using web service provided by http://www.webserviceX.NET
 * 
 * \module webservices
 * 
 * \since pre-2.1
 * 
 * \author   Arcot Rajasekar
 * \date     2008-05
 * 
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inSymbolParam - a msParam of type STR_MS_T which is a stock symbol.
 * \param[out] outQuoteParam - a msParam of type STR_MS_T which is a stock quotation as a float printed onto string.
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
 * \sa  none
**/
int
msiGetQuote(msParam_t* inSymbolParam, msParam_t* outQuoteParam, ruleExecInfo_t* rei )
{

  struct soap *soap = soap_new(); 
  struct _ns1__GetQuote sym;
  struct _ns1__GetQuoteResponse quote;
  char response[10000];
  
  RE_TEST_MACRO( "    Calling msiGetQuote" );
  
  sym.symbol = (char *) inSymbolParam->inOutStruct;

  soap_init(soap);
  soap_set_namespaces(soap, stockQuote_namespaces);  
  /*  if (soap_call___ns3__GetQuote(soap, NULL, NULL, &sym, &quote) == SOAP_OK) { */
  if (soap_call___ns2__GetQuote(soap, NULL, NULL, &sym, &quote) == SOAP_OK)   { 
    fillMsParam( outQuoteParam, NULL,
		 STR_MS_T, quote.GetQuoteResult, NULL );
    free (quote.GetQuoteResult);
    return(0);
  }
  else {
    sprintf(response,"Error in execution of stockQuote::%s\n",soap->buf);
    fillMsParam( outQuoteParam, NULL,
		 STR_MS_T, response, NULL );
    return(0);
  }
}

