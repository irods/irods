/**
 * @file currencyConvertorMS.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	currencyConvertorMS.c
 *
 * @brief	convert currency from one country to another
 * 
 * currencyConvertor can be used to convert currencies from one to another via the
 * web service provided by http://www.webserviceX.NET/
 *
 * @author	Arcot Rajasekar / University of California, San Diego
 */

#include "rsApiHandler.h"
#include "currencyConvertorMS.h"
#include "currencyConvertorH.h" 
#include "currencyConvertor.nsmap"


char *countryCodeNames[] = {
  "AFA",
  "ALL",
  "DZD",
  "ARS",
  "AWG",
  "AUD",
  "BSD",
  "BHD",
  "BDT",
  "BBD",
  "BZD",
  "BMD",
  "BTN",
  "BOB",
  "BWP",
  "BRL",
  "GBP",
  "BND",
  "BIF",
  "XOF",
  "XAF",
  "KHR",
  "CAD",
  "CVE",
  "KYD",
  "CLP",
  "CNY",
  "COP",
  "KMF",
  "CRC",
  "HRK",
  "CUP",
  "CYP",
  "CZK",
  "DKK",
  "DJF",
  "DOP",
  "XCD",
  "EGP",
  "SVC",
  "EEK",
  "ETB",
  "EUR",
  "FKP",
  "GMD",
  "GHC",
  "GIP",
  "XAU",
  "GTQ",
  "GNF",
  "GYD",
  "HTG",
  "HNL",
  "HKD",
  "HUF",
  "ISK",
  "INR",
  "IDR",
  "IQD",
  "ILS",
  "JMD",
  "JPY",
  "JOD",
  "KZT",
  "KES",
  "KRW",
  "KWD",
  "LAK",
  "LVL",
  "LBP",
  "LSL",
  "LRD",
  "LYD",
  "LTL",
  "MOP",
  "MKD",
  "MGF",
  "MWK",
  "MYR",
  "MVR",
  "MTL",
  "MRO",
  "MUR",
  "MXN",
  "MDL",
  "MNT",
  "MAD",
  "MZM",
  "MMK",
  "NAD",
  "NPR",
  "ANG",
  "NZD",
  "NIO",
  "NGN",
  "KPW",
  "NOK",
  "OMR",
  "XPF",
  "PKR",
  "XPD",
  "PAB",
  "PGK",
  "PYG",
  "PEN",
  "PHP",
  "XPT",
  "PLN",
  "QAR",
  "ROL",
  "RUB",
  "WST",
  "STD",
  "SAR",
  "SCR",
  "SLL",
  "XAG",
  "SGD",
  "SKK",
  "SIT",
  "SBD",
  "SOS",
  "ZAR",
  "LKR",
  "SHP",
  "SDD",
  "SRG",
  "SZL",
  "SEK",
  "CHF",
  "SYP",
  "TWD",
  "TZS",
  "THB",
  "TOP",
  "TTD",
  "TND",
  "TRL",
  "USD",
  "AED",
  "UGX",
  "UAH",
  "UYU",
  "VUV",
  "VEB",
  "VND",
  "YER",
  "YUM",
  "ZMK",
  "ZWD",
  "TRY"
};



int NumofCountryCodes = sizeof(countryCodeNames)/ sizeof(char *);



int getIdFromCountryCodes(char *incc) {


  int i;

  for (i=0; i < NumofCountryCodes; i++) {
    if (!strcmp(incc,countryCodeNames[i]))
      return(i);
  }
  return(-100);
}


/**
 * \fn msiConvertCurrency (msParam_t *inConvertFromParam, msParam_t *inConvertToParam, msParam_t *outRateParam, ruleExecInfo_t *rei)
 *
 * \brief   This microservice returns conversion rates for currencies from one country to 
 * another, using web service provided by http://www.webserviceX.NET
 *
 * \module webservices
 *
 * \since pre-2.1
 *
 * \author  Arcot Rajasekar
 * \date  May 2008
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] inConvertFromParam - a msParam of type STR_MS_T; 3-letter country code enumerated in  structure char *countryCodeNames[]
 * \param[in] inConvertToParam - a msParam of type STR_MS_T; 3-letter country code (same as above)
 * \param[out] outRateParam - a msParam of operation status STR_MS_T; float number printed onto string
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect  none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int
msiConvertCurrency(msParam_t* inConvertFromParam, msParam_t* inConvertToParam, 
	    msParam_t* outRateParam, 
	    ruleExecInfo_t* rei )
{

  struct soap *soap = soap_new(); 
  struct _ns1__ConversionRate pair;
  struct _ns1__ConversionRateResponse rate;
  char response[10000];
  /*  enum ns1__Currency a , b; */

  RE_TEST_MACRO( "    Calling msiConvertCurrency" );
  
  

  soap_init(soap);
  soap_set_namespaces(soap, currencyConvertor_namespaces);  

  /*
  soap_s2ns1__Currency(soap,(char *) inConvertFromParam->inOutStruct, &a);
  soap_s2ns1__Currency(soap,(char *) inConvertToParam->inOutStruct, &b);
  pair.FromCurrency = 138;
  pair.ToCurrency = 16;
  */
  pair.FromCurrency = (ns1__Currency) getIdFromCountryCodes((char *)inConvertFromParam->inOutStruct); 
  pair.ToCurrency = (ns1__Currency) getIdFromCountryCodes((char *)inConvertToParam->inOutStruct);

  if (soap_call___ns1__ConversionRate(soap, NULL, NULL, &pair, &rate) == SOAP_OK) {
    sprintf(response,"%f",rate.ConversionRateResult);
    /*    sprintf(response,"%s",soap->buf);*/
    fillMsParam( outRateParam, NULL,
		 STR_MS_T, response, NULL );
    return(0);
  }
  else {
    sprintf(response,"Error in execution of msiConvertCurrency::%s\n",soap->buf);
    fillMsParam( outRateParam, NULL,
		 STR_MS_T, response, NULL );
    return(0);
  }
}

