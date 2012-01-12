/**
 * @file	xsltMS.c
 *
 */ 

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "xsltMS.h"

#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

extern int xmlLoadExtDtdDefaultValue;



/**
 * \fn msiXsltApply (msParam_t *xsltObj, msParam_t *xmlObj, msParam_t *msParamOut, ruleExecInfo_t *rei)
 *
 * \brief   This function applies an XSL stylesheet to an XML file, both existing iRODS objects.
 *
 * \module XML 
 *
 * \since pre-2.1
 *
 * \author  Antoine de Torcy
 * \date    2008/05/29
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xsltObj - a msParam of type DataObjInp_MS_T or STR_MS_T
 * \param[in] xmlObj - a msParam of type DataObjInp_MS_T or STR_MS_T
 * \param[out] msParamOut - a msParam of operation status BUF_LEN_MS_T
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
int
msiXsltApply(msParam_t *xsltObj, msParam_t *xmlObj, msParam_t *msParamOut, ruleExecInfo_t *rei)
{
	/* for parsing msParams and to open iRODS objects */
	dataObjInp_t xmlDataObjInp, *myXmlDataObjInp;
	dataObjInp_t xsltDataObjInp, *myXsltDataObjInp;
	int xmlObjID, xsltObjID;

	/* for getting size of objects to read from */
	rodsObjStat_t *rodsObjStatOut = NULL;

	/* for reading from iRODS objects */
	openedDataObjInp_t openedDataObjInp;
	bytesBuf_t *xmlBuf = NULL, *xsltBuf = NULL;

	/* misc. to avoid repeating rei->rsComm */
	rsComm_t *rsComm;

	/* for xml parsing */
	char *outStr;
	int outLen;
	xsltStylesheetPtr style = NULL;
	xmlDocPtr xslSheet, doc, res;

	/* for output buffer */
	bytesBuf_t *mybuf;


	/*********************************  USUAL INIT PROCEDURE **********************************/
	
	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiXsltApply")


	/* Sanity checks */
	if (rei == NULL || rei->rsComm == NULL)
	{
		rodsLog (LOG_ERROR, "msiXsltApply: input rei or rsComm is NULL.");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	rsComm = rei->rsComm;


	
	/********************************** RETRIEVE INPUT PARAMS **********************************/

	/* Get xsltObj: the XSLT stylesheet */
	rei->status = parseMspForDataObjInp (xsltObj, &xsltDataObjInp, &myXsltDataObjInp, 0);
	if (rei->status < 0)
	{
		rodsLog (LOG_ERROR, "msiXsltApply: input xsltObj error. status = %d", rei->status);
		return (rei->status);
	}


	/* Get xmlObj: the XML document */
	rei->status = parseMspForDataObjInp (xmlObj, &xmlDataObjInp, &myXmlDataObjInp, 0);
	if (rei->status < 0)
	{
		rodsLog (LOG_ERROR, "msiXsltApply: input xmlObj error. status = %d", rei->status);
		return (rei->status);
	}



	/******************************** GET CONTENTS OF IRODS OBJS ********************************/

	/* Open XSLT file */
	if ((xsltObjID = rsDataObjOpen(rsComm, &xsltDataObjInp)) < 0) 
	{
		rodsLog (LOG_ERROR, "msiXsltApply: Cannot open XSLT data object. status = %d", xsltObjID);
		return (xsltObjID);
	}


	/* Get size of XSLT file */
	rei->status = rsObjStat (rsComm, &xsltDataObjInp, &rodsObjStatOut);


	/* xsltBuf init */
	/* memory for xsltBuf->buf is allocated in rsFileRead() */
	xsltBuf = (bytesBuf_t *) malloc (sizeof (bytesBuf_t));
	memset (xsltBuf, 0, sizeof (bytesBuf_t));


	/* Read XSLT file */
	memset (&openedDataObjInp, 0, sizeof (openedDataObjInp_t));
	openedDataObjInp.l1descInx = xsltObjID;
	openedDataObjInp.len = (int)rodsObjStatOut->objSize;

	rei->status = rsDataObjRead (rsComm, &openedDataObjInp, xsltBuf);
	
	/* Make sure that the result is null terminated */
	if (strlen((char*)xsltBuf->buf) > (size_t)openedDataObjInp.len)
	{
		((char*)xsltBuf->buf)[openedDataObjInp.len-1]='\0';
	}


	/* Close XSLT file */
	rei->status = rsDataObjClose (rsComm, &openedDataObjInp);


	/* Cleanup. Needed before using rodsObjStatOut for a new rsObjStat() call */
	freeRodsObjStat (rodsObjStatOut);


	/* Open XML file */
	if ((xmlObjID = rsDataObjOpen(rsComm, &xmlDataObjInp)) < 0) 
	{
		rodsLog (LOG_ERROR, "msiXsltApply: Cannot open XML data object. status = %d", xmlObjID);
		return (xmlObjID);
	}


	/* Get size of XML file */
	rei->status = rsObjStat (rsComm, &xmlDataObjInp, &rodsObjStatOut);


	/* xmlBuf init */
	/* memory for xmlBuf->buf is allocated in rsFileRead() */
	xmlBuf = (bytesBuf_t *) malloc (sizeof (bytesBuf_t));
	memset (xmlBuf, 0, sizeof (bytesBuf_t));


	/* Read XML file */
	memset (&openedDataObjInp, 0, sizeof (openedDataObjInp_t));
	openedDataObjInp.l1descInx = xmlObjID;
	openedDataObjInp.len = (int)rodsObjStatOut->objSize;

	rei->status = rsDataObjRead (rsComm, &openedDataObjInp, xmlBuf);

	/* Make sure that the result is null terminated */
	if (strlen((char*)xmlBuf->buf) > (size_t)openedDataObjInp.len)
	{
		((char*)xmlBuf->buf)[openedDataObjInp.len-1]='\0';
	}


	/* Close XML file */
	rei->status = rsDataObjClose (rsComm, &openedDataObjInp);

	/* cleanup */
	freeRodsObjStat (rodsObjStatOut);



	/******************************** PARSE XML DOCS AND APPLY XSL ********************************/

	xmlSubstituteEntitiesDefault(1);
	xmlLoadExtDtdDefaultValue = 1;


	/* Parse xsltBuf.buf into an xmlDocPtr, and the xmlDocPtr into an xsltStylesheetPtr */
	xslSheet = xmlParseDoc((xmlChar*)xsltBuf->buf);
	style = xsltParseStylesheetDoc(xslSheet);

	/* Parse xmlBuf.buf into an xmlDocPtr */
	doc = xmlParseDoc((xmlChar*)xmlBuf->buf);

	/* And the magic happens */
	res = xsltApplyStylesheet(style, doc, NULL);

	/* Save result XML document to a string */
	rei->status = xsltSaveResultToString((xmlChar**)&outStr, &outLen, res, style);


	/* cleanup of all xml parsing stuff */
	xsltFreeStylesheet(style);
	xmlFreeDoc(res);
	xmlFreeDoc(doc);

        xsltCleanupGlobals();
        xmlCleanupParser();



	/************************************** WE'RE DONE **************************************/


	/* copy the result string into an output buffer */
	mybuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
	mybuf->buf = (void *)outStr;
	mybuf->len = strlen(outStr);


	/* send results out to msParamOut */
	fillBufLenInMsParam (msParamOut, mybuf->len, mybuf);

	return (rei->status);
}

