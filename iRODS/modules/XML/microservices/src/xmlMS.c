/**
 * @file	xmlMS.c
 *
 */ 

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "xmlMS.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlschemas.h>






/* get the first child node with a given name */
static xmlNodePtr
getChildNodeByName(xmlNodePtr cur, char *name)
{
        if (cur == NULL || name == NULL)
	{
		return NULL;
	}

        for(cur=cur->children; cur != NULL; cur=cur->next)
	{
                if(cur->name && (strcmp((char*)cur->name, name) == 0))
		{
                        return cur;
                }
        }

	return NULL;
}



/* custom handler to catch XML validation errors and print them to a buffer */
static int 
myErrorCallback(bytesBuf_t *errBuf, const char* errMsg, ...)
{
	va_list ap;
	char tmpStr[MAX_NAME_LEN];
	int written;

	va_start(ap, errMsg);
	written = vsnprintf(tmpStr, MAX_NAME_LEN, errMsg, ap);
	va_end(ap);

	appendToByteBuf(errBuf, tmpStr);
	return (written);
}


/**
 * \fn msiLoadMetadataFromXml (msParam_t *targetObj, msParam_t *xmlObj, ruleExecInfo_t *rei)
 *
 * \brief   This microservice parses an XML iRODS file to extract metadata tags.
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
 * \param[in] targetObj - Optional - a msParam of type DataObjInp_MS_T or STR_MS_T
 * \param[in] xmlObj - a msParam of type DataObjInp_MS_T or STR_MS_T
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


#if defined(LIBXML_XPATH_ENABLED) && \
    defined(LIBXML_SAX1_ENABLED) && \
    defined(LIBXML_OUTPUT_ENABLED)

int 
msiLoadMetadataFromXml(msParam_t *targetObj, msParam_t *xmlObj, ruleExecInfo_t *rei) 
{
	/* for parsing msParams and to open iRODS objects */
	dataObjInp_t xmlDataObjInp, *myXmlDataObjInp;
	dataObjInp_t targetObjInp, *myTargetObjInp;
	int xmlObjID;

	/* for getting size of objects to read from */
	rodsObjStat_t *rodsObjStatOut = NULL;

	/* for reading from iRODS objects */
	openedDataObjInp_t openedDataObjInp;
	bytesBuf_t *xmlBuf;

	/* misc. to avoid repeating rei->rsComm */
	rsComm_t *rsComm;

	/* for xml parsing */
	xmlDocPtr doc;

	/* for XPath evaluation */
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	xmlChar xpathExpr[] = "//AVU";
	xmlNodeSetPtr nodes;
	int avuNbr, i;
	
	/* for new AVU creation */
	modAVUMetadataInp_t modAVUMetadataInp;
	int max_attr_len = 2700;
	char attrStr[max_attr_len];



	/*********************************  USUAL INIT PROCEDURE **********************************/
	
	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiLoadMetadataFromXml")


	/* Sanity checks */
	if (rei == NULL || rei->rsComm == NULL)
	{
		rodsLog (LOG_ERROR, "msiLoadMetadataFromXml: input rei or rsComm is NULL.");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	rsComm = rei->rsComm;


	
	/********************************** RETRIEVE INPUT PARAMS **************************************/

	/* Get path of target object */
	rei->status = parseMspForDataObjInp (targetObj, &targetObjInp, &myTargetObjInp, 0);
	if (rei->status < 0)
	{
		rodsLog (LOG_ERROR, "msiLoadMetadataFromXml: input targetObj error. status = %d", rei->status);
		return (rei->status);
	}


	/* Get path of XML document */
	rei->status = parseMspForDataObjInp (xmlObj, &xmlDataObjInp, &myXmlDataObjInp, 0);
	if (rei->status < 0)
	{
		rodsLog (LOG_ERROR, "msiLoadMetadataFromXml: input xmlObj error. status = %d", rei->status);
		return (rei->status);
	}



	/******************************** OPEN AND READ FROM XML OBJECT ********************************/

	/* Open XML file */
	if ((xmlObjID = rsDataObjOpen(rsComm, &xmlDataObjInp)) < 0) 
	{
		rodsLog (LOG_ERROR, "msiLoadMetadataFromXml: Cannot open XML data object. status = %d", xmlObjID);
		return (xmlObjID);
	}


	/* Get size of XML file */
	rei->status = rsObjStat (rsComm, &xmlDataObjInp, &rodsObjStatOut);
	if( NULL == rodsObjStatOut ) { // JMC cppcheck  nullptr ref
		rodsLog( LOG_ERROR, "msiXmlDocSchemaValidate: null &rodsObjStatOut" );
		return ( rei->status );
	}

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



	/******************************** PARSE XML DOCUMENT ********************************/

	xmlSubstituteEntitiesDefault(1);
	xmlLoadExtDtdDefaultValue = 1;


	/* Parse xmlBuf.buf into an xmlDocPtr */
	doc = xmlParseDoc((xmlChar*)xmlBuf->buf);


	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) 
	{
		rodsLog (LOG_ERROR, "msiLoadMetadataFromXml: Unable to create new XPath context.");
		xmlFreeDoc(doc); 
		return(-1);
	}


	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
	if(xpathObj == NULL) 
	{
		rodsLog (LOG_ERROR, "msiLoadMetadataFromXml: Unable to evaluate XPath expression \"%s\".", xpathExpr);
		xmlXPathFreeContext(xpathCtx); 
		xmlFreeDoc(doc); 
		return(-1);
	}

	/* How many AVU nodes did we get? */
	if ((nodes = xpathObj->nodesetval) != NULL)
	{
		avuNbr = nodes->nodeNr;
	}
	else 
	{
		avuNbr = 0;
	}



	/******************************** CREATE AVU TRIPLETS ********************************/

	/* Add a new AVU for each node. It's ok to process the nodes in forward order since we're not modifying them */
	for(i = 0; i < avuNbr; i++) 
	{
		if (nodes->nodeTab[i])
		{
//			/* temporary: Add index number to avoid duplicating attribute names */
//			memset(attrStr, '\0', MAX_NAME_LEN);
//			snprintf(attrStr, MAX_NAME_LEN - 1, "%04d: %s", i+1, (char*)xmlNodeGetContent(getChildNodeByName(nodes->nodeTab[i], "Attribute")) );

			/* Truncate if needed. No prefix. */
			memset(attrStr, '\0', max_attr_len);
			snprintf(attrStr, max_attr_len - 1, "%s", (char*)xmlNodeGetContent(getChildNodeByName(nodes->nodeTab[i], "Attribute")) );

			/* init modAVU input */
			memset (&modAVUMetadataInp, 0, sizeof(modAVUMetadataInp_t));
			modAVUMetadataInp.arg0 = "add";
			modAVUMetadataInp.arg1 = "-d";

			/* Use target object if one was provided, otherwise look for it in the XML doc */
			if (myTargetObjInp->objPath != NULL && strlen(myTargetObjInp->objPath) > 0)
			{
				modAVUMetadataInp.arg2 = myTargetObjInp->objPath;
			}
			else
			{
				modAVUMetadataInp.arg2 = (char*)xmlNodeGetContent(getChildNodeByName(nodes->nodeTab[i], "Target"));
			}

			modAVUMetadataInp.arg3 = attrStr;
			modAVUMetadataInp.arg4 = (char*)xmlNodeGetContent(getChildNodeByName(nodes->nodeTab[i], "Value"));
			modAVUMetadataInp.arg5 = (char*)xmlNodeGetContent(getChildNodeByName(nodes->nodeTab[i], "Unit"));
			
			/* invoke rsModAVUMetadata() */
			rei->status = rsModAVUMetadata (rsComm, &modAVUMetadataInp);
		}
	}



	/************************************** WE'RE DONE **************************************/

	/* cleanup of all xml parsing stuff */
	xmlFreeDoc(doc);
        xmlCleanupParser();

	return (rei->status);
}


/* Default stub if no XPath support */
#else
/**
 * \fn msiLoadMetadataFromXml (msParam_t *targetObj, msParam_t *xmlObj, ruleExecInfo_t *rei)
**/
int 
msiLoadMetadataFromXml(msParam_t *targetObj, msParam_t *xmlObj, ruleExecInfo_t *rei) 
{
	rodsLog (LOG_ERROR, "msiLoadMetadataFromXml: XPath support not compiled in.");
	return (SYS_NOT_SUPPORTED);
}
#endif



/**
 * \fn msiXmlDocSchemaValidate(msParam_t *xmlObj, msParam_t *xsdObj, msParam_t *status, ruleExecInfo_t *rei)
 *
 * \brief  This microservice validates an XML file against an XSD schema, both iRODS objects.
 * 
 * \module xml
 * 
 * \since pre-2.1
 * 
 * \author  Antoine de Torcy
 * \date    2008/05/29
 * 
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] xmlObj - a msParam of type DataObjInp_MS_T or STR_MS_T which is irods path of the XML object.
 * \param[in] xsdObj - a msParam of type DataObjInp_MS_T or STR_MS_T which is irods path of the XSD object.
 * \param[out] status - a msParam of type INT_MS_T which is a validation result.
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
msiXmlDocSchemaValidate(msParam_t *xmlObj, msParam_t *xsdObj, msParam_t *status, ruleExecInfo_t *rei) 
{
	/* for parsing msParams and to open iRODS objects */
	dataObjInp_t xmlObjInp, *myXmlObjInp;
	dataObjInp_t xsdObjInp, *myXsdObjInp;
	int xmlObjID, xsdObjID;

	/* for getting size of objects to read from */
	rodsObjStat_t *rodsObjStatOut = NULL;

	/* for reading from iRODS objects */
	openedDataObjInp_t openedDataObjInp;
	bytesBuf_t *xmlBuf = NULL;
	char *tail;

	/* for xml parsing and validating */
	xmlDocPtr doc, xsd_doc;
	xmlSchemaParserCtxtPtr parser_ctxt;
	xmlSchemaPtr schema;
	xmlSchemaValidCtxtPtr valid_ctxt;
	bytesBuf_t *errBuf;

	/* misc. to avoid repeating rei->rsComm */
	rsComm_t *rsComm;



	/*************************************  USUAL INIT PROCEDURE **********************************/
	
	/* For testing mode when used with irule --test */
	RE_TEST_MACRO ("    Calling msiXmlDocSchemaValidate")


	/* Sanity checks */
	if (rei == NULL || rei->rsComm == NULL)
	{
		rodsLog (LOG_ERROR, "msiXmlDocSchemaValidate: input rei or rsComm is NULL.");
		return (SYS_INTERNAL_NULL_INPUT_ERR);
	}

	rsComm = rei->rsComm;



	/************************************ ADDITIONAL INIT SETTINGS *********************************/

	/* XML constants */
	xmlSubstituteEntitiesDefault(1);
	xmlLoadExtDtdDefaultValue = 1;


	/* allocate memory for output error buffer */
	errBuf = (bytesBuf_t *)malloc(sizeof(bytesBuf_t));
	errBuf->buf = strdup("");
	errBuf->len = strlen((char*)errBuf->buf);

	/* Default status is failure, overwrite if success */
	fillBufLenInMsParam (status, -1, NULL);

	
	/********************************** RETRIEVE INPUT PARAMS **************************************/

	/* Get path of XML document */
	rei->status = parseMspForDataObjInp (xmlObj, &xmlObjInp, &myXmlObjInp, 0);
	if (rei->status < 0)
	{
		rodsLog (LOG_ERROR, "msiXmlDocSchemaValidate: input xmlObj error. status = %d", rei->status);
		free( errBuf->buf ); // JMC cppcheck - leak
		free( errBuf ); // JMC cppcheck - leak
		return (rei->status);
	}


	/* Get path of schema */
	rei->status = parseMspForDataObjInp (xsdObj, &xsdObjInp, &myXsdObjInp, 0);
	if (rei->status < 0)
	{
		rodsLog (LOG_ERROR, "msiXmlDocSchemaValidate: input xsdObj error. status = %d", rei->status);
		free( errBuf->buf ); // JMC cppcheck - leak
		free( errBuf ); // JMC cppcheck - leak
		return (rei->status);
	}


	/******************************** OPEN AND READ FROM XML OBJECT ********************************/

	/* Open XML file */
	if ((xmlObjID = rsDataObjOpen(rsComm, &xmlObjInp)) < 0) 
	{
		rodsLog (LOG_ERROR, "msiXmlDocSchemaValidate: Cannot open XML data object. status = %d", xmlObjID);
		free( errBuf->buf ); // JMC cppcheck - leak
		free( errBuf ); // JMC cppcheck - leak
		return (xmlObjID);
	}


	/* Get size of XML file */
	rei->status = rsObjStat (rsComm, &xmlObjInp, &rodsObjStatOut);
	if( NULL == rodsObjStatOut ) { // JMC cppcheck  nullptr ref
		rodsLog( LOG_ERROR, "msiXmlDocSchemaValidate: null &rodsObjStatOut" );
		free( errBuf ); // JMC cppcheck - leak
		return ( rei->status );
	}

	/* xmlBuf init */
	/* memory for xmlBuf->buf is allocated in rsFileRead() */
	xmlBuf = (bytesBuf_t *) malloc (sizeof (bytesBuf_t));
	memset (xmlBuf, 0, sizeof (bytesBuf_t));


	/* Read content of XML file */
	memset (&openedDataObjInp, 0, sizeof (openedDataObjInp_t));
	openedDataObjInp.l1descInx = xmlObjID;
	openedDataObjInp.len = (int)rodsObjStatOut->objSize + 1;	/* extra byte to add a null char */

	rei->status = rsDataObjRead (rsComm, &openedDataObjInp, xmlBuf);

	/* add terminating null character */
	tail = (char*)xmlBuf->buf;
	tail[openedDataObjInp.len - 1] = '\0';


	/* Close XML file */
	rei->status = rsDataObjClose (rsComm, &openedDataObjInp);

	/* cleanup */
	freeRodsObjStat (rodsObjStatOut);



	/*************************************** PARSE XML DOCUMENT **************************************/

	/* Parse xmlBuf.buf into an xmlDocPtr */
	doc = xmlParseDoc((xmlChar*)xmlBuf->buf);
	clearBBuf(xmlBuf);

	if (doc == NULL)
	{
		rodsLog (LOG_ERROR, "msiXmlDocSchemaValidate: XML document cannot be loaded or is not well-formed.");

	    xmlCleanupParser();

		free( errBuf->buf ); // JMC cppcheck - leak
		free( errBuf ); // JMC cppcheck - leak
	    return (USER_INPUT_FORMAT_ERR);
	}



	/******************************** OPEN AND READ FROM XSD OBJECT ********************************/

	/* Open schema file */
	if ((xsdObjID = rsDataObjOpen(rsComm, &xsdObjInp)) < 0) 
	{
		rodsLog (LOG_ERROR, "msiXmlDocSchemaValidate: Cannot open XSD data object. status = %d", xsdObjID);

		xmlFreeDoc(doc);
	    xmlCleanupParser();

		free( errBuf->buf ); // JMC cppcheck - leak
		free( errBuf ); // JMC cppcheck - leak
		return (xsdObjID);
	}


	/* Get size of schema file */
	rei->status = rsObjStat (rsComm, &xsdObjInp, &rodsObjStatOut);


	/* Read entire schema file */
	memset (&openedDataObjInp, 0, sizeof (openedDataObjInp_t));
	openedDataObjInp.l1descInx = xsdObjID;
	openedDataObjInp.len = (int)rodsObjStatOut->objSize + 1;	/* to add null char */

	rei->status = rsDataObjRead (rsComm, &openedDataObjInp, xmlBuf);

	/* add terminating null character */
	tail = (char*)xmlBuf->buf;
	tail[openedDataObjInp.len - 1] = '\0';


	/* Close schema file */
	rei->status = rsDataObjClose (rsComm, &openedDataObjInp);

	/* cleanup */
	freeRodsObjStat (rodsObjStatOut);



	/*************************************** PARSE XSD DOCUMENT **************************************/

	/* Parse xmlBuf.buf into an xmlDocPtr */
	xsd_doc = xmlParseDoc((xmlChar*)xmlBuf->buf);
	clearBBuf(xmlBuf);

	if (xsd_doc == NULL)
	{
		rodsLog (LOG_ERROR, "msiXmlDocSchemaValidate: XML Schema cannot be loaded or is not well-formed.");

		xmlFreeDoc(doc);
		xmlCleanupParser();

		free( errBuf->buf ); // JMC cppcheck - leak
		free( errBuf ); // JMC cppcheck - leak
	    return (USER_INPUT_FORMAT_ERR);
	}



	/**************************************** VALIDATE DOCUMENT **************************************/

	/* Create a parser context */
	parser_ctxt = xmlSchemaNewDocParserCtxt(xsd_doc);
	if (parser_ctxt == NULL)
	{
		rodsLog (LOG_ERROR, "msiXmlDocSchemaValidate: Unable to create a parser context for the schema.");

		xmlFreeDoc(xsd_doc);
		xmlFreeDoc(doc);
	    xmlCleanupParser();

		free( errBuf->buf ); // JMC cppcheck - leak
		free( errBuf ); // JMC cppcheck - leak
	    return (USER_INPUT_FORMAT_ERR);
	}


	/* Parse the XML schema */
	schema = xmlSchemaParse(parser_ctxt);
	if (schema == NULL) 
	{
		rodsLog (LOG_ERROR, "msiXmlDocSchemaValidate: Invalid schema.");

		xmlSchemaFreeParserCtxt(parser_ctxt);
		xmlFreeDoc(doc);
		xmlFreeDoc(xsd_doc);
        xmlCleanupParser();

		free( errBuf->buf ); // JMC cppcheck - leak
		free( errBuf ); // JMC cppcheck - leak
	    return (USER_INPUT_FORMAT_ERR);
	}


	/* Create a validation context */
	valid_ctxt = xmlSchemaNewValidCtxt(schema);
	if (valid_ctxt == NULL) 
	{
		rodsLog (LOG_ERROR, "msiXmlDocSchemaValidate: Unable to create a validation context for the schema.");

		xmlSchemaFree(schema);
		xmlSchemaFreeParserCtxt(parser_ctxt);
		xmlFreeDoc(xsd_doc);
		xmlFreeDoc(doc);
	    xmlCleanupParser();

		free( errBuf->buf ); // JMC cppcheck - leak
		free( errBuf ); // JMC cppcheck - leak
	    return (USER_INPUT_FORMAT_ERR);
	}


	/* Set myErrorCallback() as the default handler for error messages and warnings */
	xmlSchemaSetValidErrors(valid_ctxt, (xmlSchemaValidityErrorFunc)myErrorCallback, (xmlSchemaValidityWarningFunc)myErrorCallback, errBuf);


	/* Validate XML doc */
	rei->status = xmlSchemaValidateDoc(valid_ctxt, doc);



	/******************************************* WE'RE DONE ******************************************/

	/* return both error code and messages through status */
	resetMsParam (status);
	fillBufLenInMsParam (status, rei->status, errBuf);


	/* cleanup of all xml parsing stuff */
	xmlSchemaFreeValidCtxt(valid_ctxt);
	xmlSchemaFree(schema);
	xmlSchemaFreeParserCtxt(parser_ctxt);
	xmlFreeDoc(doc);
	xmlFreeDoc(xsd_doc);
	xmlCleanupParser();

	free( errBuf->buf ); // JMC cppcheck - leak
	free( errBuf ); // JMC cppcheck - leak
	return (rei->status);
}


