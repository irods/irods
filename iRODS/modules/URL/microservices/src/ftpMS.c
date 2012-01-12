/**
 * @file ftpMS.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "ftpMS.h"


#include <stdio.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>




/* custom callback function for the cURL handler */
static size_t createAndWriteToDataObj(void *buffer, size_t size, size_t nmemb, void *stream)
{
	dataObjFtpInp_t *dataObjFtpInp;			/* the "file descriptor" for our destination object */
	dataObjInp_t dataObjInp;				/* input structure for rsDataObjCreate */
	openedDataObjInp_t openedDataObjInp;	/* input structure for rsDataObjWrite */
	bytesBuf_t bytesBuf;					/* input buffer for rsDataObjWrite */
	size_t written;							/* output value */


	/* retrieve dataObjFtpInp_t input */
	dataObjFtpInp = (dataObjFtpInp_t *)stream;

	/* to avoid unpleasant surprises */
	memset(&dataObjInp, 0, sizeof(dataObjInp_t));
	memset(&openedDataObjInp, 0, sizeof(openedDataObjInp_t));


	/* If this is the first call we need to create our data object before writing to it */
	if (dataObjFtpInp && !dataObjFtpInp->l1descInx)
	{
		strcpy(dataObjInp.objPath, dataObjFtpInp->objPath);
		dataObjFtpInp->l1descInx = rsDataObjCreate(dataObjFtpInp->rsComm, &dataObjInp);
		
		/* problem? */
		if (dataObjFtpInp->l1descInx <= 2)
		{
			rodsLog (LOG_ERROR, "createAndWriteToDataObj: rsDataObjCreate failed for %s, status = %d", dataObjInp.objPath, dataObjFtpInp->l1descInx);
			return (dataObjFtpInp->l1descInx);
		}
	}


	/* set up input buffer for rsDataObjWrite */
	bytesBuf.len = (int)(size * nmemb);
	bytesBuf.buf = buffer;


	/* set up input data structure for rsDataObjWrite */
    if( dataObjFtpInp == NULL ) {
        rodsLog( LOG_ERROR, "createAndWriteToDataObj :: dataObjFtpInp is NULL" );
		return -1;
	}
	openedDataObjInp.l1descInx = dataObjFtpInp->l1descInx;
	openedDataObjInp.len = bytesBuf.len;


	/* write to data object */
	written = rsDataObjWrite(dataObjFtpInp->rsComm, &openedDataObjInp, &bytesBuf);

	return (written);
}





/**
 * \fn msiFtpGet(msParam_t *target, msParam_t *destObj, msParam_t *status, ruleExecInfo_t *rei)
 *
 * \brief This microservice gets a remote file using FTP and writes it to an iRODS object.
 *
 * \module URL
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date    2008-09-24
 *
 * \note This microservice uses libcurl to open an ftp session with a remote server and read from a remote file.
 *    The results are written to a newly created iRODS object, one block at a time until the whole file is read.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] target - Required - a STR_MS_T containing the remote URL.
 * \param[in] destObj - Required - a DataObjInp_MS_T or a STR_MS_T which would be taken as the object's path.
 * \param[out] status - a INT_MS_T containing the status.
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
**/
int msiFtpGet(msParam_t *target, msParam_t *destObj, msParam_t *status, ruleExecInfo_t *rei)
{
	CURL *curl;					/* curl handler */
	CURLcode res;

	dataObjFtpInp_t dataObjFtpInp;			/* custom file descriptor for our callback function */

	dataObjInp_t destObjInp, *myDestObjInp;		/* for parsing input params */
	openedDataObjInp_t openedDataObjInp;
	char *target_str;

	int isCurlErr;					/* transfer success/failure boolean */



	/*************************************  INIT **********************************/
	
	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiFtpGet")
	
	/* Sanity checks */
	if (rei == NULL || rei->rsComm == NULL) 
	{
		rodsLog (LOG_ERROR, "msiFtpGet: input rei or rsComm is NULL.");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	/* pad data structures with null chars */
	memset(&dataObjFtpInp, 0, sizeof(dataObjFtpInp_t));
	memset(&destObjInp, 0, sizeof(dataObjInp_t));
	memset(&openedDataObjInp, 0, sizeof(openedDataObjInp_t));



	/********************************** PARAM PARSING  *********************************/

	/* parse target URL */
	if ((target_str = parseMspForStr(target)) == NULL)
	{
		rodsLog (LOG_ERROR, "msiFtpGet: input target is NULL.");
		return (USER__NULL_INPUT_ERR);
	}


	/* Get path of destination object */
	rei->status = parseMspForDataObjInp (destObj, &destObjInp, &myDestObjInp, 0);
	if (rei->status < 0)
	{
		rodsLog (LOG_ERROR, "msiFtpGet: input destObj error. status = %d", rei->status);
		return (rei->status);
	}



	/************************** SET UP AND INVOKE CURL HANDLER **************************/

	/* set up dataObjFtpInp */
	strcpy(dataObjFtpInp.objPath, destObjInp.objPath);
	dataObjFtpInp.l1descInx = 0; /* the object is yet to be created */
	dataObjFtpInp.rsComm = rei->rsComm;


	/* curl easy handler init */
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	isCurlErr = 0;

	if(curl) 
	{
		/* Set up curl easy handler */
		curl_easy_setopt(curl, CURLOPT_URL, target_str);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, createAndWriteToDataObj);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dataObjFtpInp);
		
		/* For debugging only */
// 		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		
		/* let curl do its thing */
		res = curl_easy_perform(curl);
		
		/* cleanup curl handler */
		curl_easy_cleanup(curl);
		
		/* how did it go? */
		if(CURLE_OK != res)
		{
			rodsLog (LOG_ERROR, "msiFtpGet: libcurl error: %d", res);
			isCurlErr = 1;
		}
	}


	/* close irods object */
	if (dataObjFtpInp.l1descInx)
	{
		openedDataObjInp.l1descInx = dataObjFtpInp.l1descInx;
		rei->status = rsDataObjClose(rei->rsComm, &openedDataObjInp);
	}



	/*********************************** DONE ********************************/

	/* cleanup and return status */
	curl_global_cleanup();

	if (isCurlErr)
	{
		/* -1 for now. should add an error code for this */
		rei->status = -1;
	}

	fillIntInMsParam (status, rei->status);
	
	return (rei->status);
}




/**
 * \fn msiTwitterPost(msParam_t *twittername, msParam_t *twitterpass, msParam_t *message, msParam_t *status, ruleExecInfo_t *rei)
 *
 * \brief Posts a message to twitter.com
 *
 * \module URL
 *
 * \since 2.3
 *
 * \author  Antoine de Torcy
 * \date    2009-07-23
 *
 * \note This microservice posts a message on twitter.com, aka a "tweet". 
 *       A valid twitter account name and password must be provided. 
 *       Special characters in the message can affect parsing of the POST form and 
 *       create unexpected results. Avoid if possible, or use quotes.
 *       This is intended for fun and for use in demos. Since your twitter password is
 *       passed unencrypted here, do not use this with a twitter account you do not
 *       wish to be compromised. Or if you do, change your password afterwards. 
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] twittername - Required - a STR_MS_T containing the twitter username.
 * \param[in] twitterpass - Required - a STR_MS_T containing the twitter password.
 * \param[in] message - Required - a STR_MS_T containing the message to post.
 * \param[out] status - An INT_MS_T containing the status.
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
**/
int msiTwitterPost(msParam_t *twittername, msParam_t *twitterpass, msParam_t *message, msParam_t *status, ruleExecInfo_t *rei)
{
	CURL *curl;								/* curl handler */
	CURLcode res;

	char *username, *passwd, *tweet;		/* twitter account and msg. */
	
	char userpwd[LONG_NAME_LEN];			/* input for POST request */
	char form_msg[160];

	int isCurlErr;							/* transfer success/failure boolean */



	/*************************************  INIT **********************************/
	
	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiTwitterPost")
	
	/* Sanity checks */
	if (rei == NULL || rei->rsComm == NULL) 
	{
		rodsLog (LOG_ERROR, "msiTwitterPost: input rei or rsComm is NULL.");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}


	/********************************** PARAM PARSING  *********************************/

	/* parse twitter name */
	if ((username = parseMspForStr(twittername)) == NULL)
	{
		rodsLog (LOG_ERROR, "msiTwitterPost: input twittername is NULL.");
		return (USER__NULL_INPUT_ERR);
	}
	
	/* parse twitter password */
	if ((passwd = parseMspForStr(twitterpass)) == NULL)
	{
		rodsLog (LOG_ERROR, "msiTwitterPost: input twitterpass is NULL.");
		return (USER__NULL_INPUT_ERR);
	}

	/* parse message */
	if ((tweet = parseMspForStr(message)) == NULL)
	{
		rodsLog (LOG_ERROR, "msiTwitterPost: input message is NULL.");
		return (USER__NULL_INPUT_ERR);
	}


	/* prepare form and truncate tweet to 140 chars */
	strcpy(form_msg, "status=");
	strncat(form_msg, tweet, 140);

	/* Prepare userpwd */
	snprintf(userpwd, LONG_NAME_LEN, "%s:%s", username, passwd);


	/************************** SET UP AND INVOKE CURL HANDLER **************************/

	/* curl easy handler init */
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	isCurlErr = 0;

	if(curl) 
	{
		/* Set up curl easy handler */
		curl_easy_setopt(curl, CURLOPT_URL, "http://twitter.com/statuses/update.xml");
		curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, form_msg);
		
		/* For debugging */
// 		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		
		/* let curl do its thing */
		res = curl_easy_perform(curl);
		
		/* cleanup curl handler */
		curl_easy_cleanup(curl);
		
		/* how did it go? */
		if(CURLE_OK != res)
		{
			rodsLog (LOG_ERROR, "msiTwitterPost: libcurl error: %d", res);
			isCurlErr = 1;
		}
	}


	/*********************************** DONE ********************************/

	/* cleanup and return status */
	curl_global_cleanup();

	if (isCurlErr)
	{
		/* -1 for now. should add an error code for this */
		rei->status = -1;
	}

	fillIntInMsParam (status, rei->status);
	
	return (rei->status);
}




/**
 * \fn msiPostThis(msParam_t *url, msParam_t *data, msParam_t *status, ruleExecInfo_t *rei)
 *
**/
int msiPostThis(msParam_t *url, msParam_t *data, msParam_t *status, ruleExecInfo_t *rei)
{
	CURL *curl;								/* curl handler */
	CURLcode res;

	char *myurl, *mydata;					/* input for POST request */


	/*************************************  INIT **********************************/

	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiPostThis")

	/* Sanity checks */
	if (rei == NULL || rei->rsComm == NULL)
	{
		rodsLog (LOG_ERROR, "msiPostThis: input rei or rsComm is NULL.");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}


	/********************************** PARAM PARSING  *********************************/

	/* Parse url */
	if ((myurl = parseMspForStr(url)) == NULL)
	{
		rodsLog (LOG_ERROR, "msiPostThis: input url is NULL.");
		return (USER__NULL_INPUT_ERR);
	}

	/* Parse data, which can be NULL */
	mydata = parseMspForStr(data);


	/************************** SET UP AND INVOKE CURL HANDLER **************************/

	/* cURL easy handler init */
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if(curl)
	{
		/* Set up curl easy handler */
		curl_easy_setopt(curl, CURLOPT_URL, myurl);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, mydata);

		/* For debugging */
// 		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		/* Let curl do its thing */
		res = curl_easy_perform(curl);

		/* Cleanup curl handler */
		curl_easy_cleanup(curl);

		/* How did it go? */
		if(res != CURLE_OK)
		{
			rodsLog (LOG_ERROR, "msiPostThis: cURL error: %s", curl_easy_strerror(res));
			rei->status = SYS_HANDLER_DONE_WITH_ERROR; /* For lack of anything better */
		}
		else
		{
			rei->status = 0;
		}
	}


	/*********************************** DONE ********************************/

	/* Cleanup and return CURLcode */
	curl_global_cleanup();

	fillIntInMsParam (status, rei->status);
	return (rei->status);
}


