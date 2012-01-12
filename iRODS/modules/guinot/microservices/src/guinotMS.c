
/**
 * @file  guinotMS.c
 *
 * @brief Microservices contributed by Romain Guinot
 *
 * To be added later to core afterwards
 * 
 *
 * @author  Romain Guinot
 */

#include <time.h>
#include "rsApiHandler.h"
#include "guinotMS.h"


/*
 Convert a Unix time value (as from getNowStr) to a local
 time format.
*/
#if 0
int
getFormattedLocalTimeFromRodsTime(char *timeStrIn,
				  char *timeStr,
				  const char *timeFormat)
{
  time_t myTime;
  struct tm *mytm;
  if (sizeof(time_t)==4) {
    mytm = localtime (&myTime);  // myTime should have been set by time() first
    snprintf (timeStr, TIME_LEN, timeFormat,
	      mytm->tm_year + 1900, mytm->tm_mon + 1, mytm->tm_mday,
	      mytm->tm_hour, mytm->tm_min, mytm->tm_sec);
    return(0);
  }
}
#endif


/**
 * \fn msiGetFormattedSystemTime(msParam_t* outParam, msParam_t* inpParam,
 *          msParam_t* inpFormatParam, ruleExecInfo_t *rei)
 *
 * \brief Returns the local system time
 *
 * \module guinot
 *
 * \since pre-2.1
 *
 * \author  Romain Guinot
 * \date    2008
 *
 * \note Default output format is system time in seconds. Use 'human' as input 
 *       parameter for human readable format.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[out] outParam - a STR_MS_T containing the time
 * \param[in] inpParam - Optional - a STR_MS_T containing the desired output format (human)
 * \param[in] inpFormatParam - Optional - a STR_MS_T containing printf formatting (if inpParam was human)
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
msiGetFormattedSystemTime(msParam_t* outParam, msParam_t* inpParam,
        msParam_t* inpFormatParam, ruleExecInfo_t *rei)
{
  char *format;
  char *dateFormat;
  char tStr[TIME_LEN];
  time_t myTime;
  struct tm *mytm;
  int status;

  /* For testing mode when used with irule --test */
  RE_TEST_MACRO ("    Calling msiGetFormattedSystemTime");
    
    
  if (rei == NULL || rei->rsComm == NULL) {
    rodsLog (LOG_ERROR, 
	     "msiGetFormattedSystemTime: input rei or rsComm is NULL");
    return (SYS_INTERNAL_NULL_INPUT_ERR);
  }
  format = (char*)inpParam->inOutStruct;
  dateFormat = (char*)inpFormatParam->inOutStruct;
  
  if (!format || strcmp(format, "human")) {
    getNowStr(tStr);
  }
  else {
    myTime = time(NULL);
    mytm = localtime(&myTime);

    snprintf (tStr, TIME_LEN, dateFormat,
    		mytm->tm_year + 1900, mytm->tm_mon + 1, mytm->tm_mday,
    		mytm->tm_hour, mytm->tm_min, mytm->tm_sec);
  }
  status = fillStrInMsParam (outParam,tStr);
  return(status);
}

/**************************************************************************/
