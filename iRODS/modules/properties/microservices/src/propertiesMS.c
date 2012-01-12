/**
 * @file propertiesMS.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file  propertiesMS.c
 *
 * @brief Manage a list of keyword-value pair properties.
 *
 * Properties can be used to store metadata extracted from a file or
 * database, or to build up a list of options to control how a
 * microservice processes data.
 *
 * @author  David R. Nadeau / University of California, San Diego
 */

#include "rsApiHandler.h"
#include "propertiesMS.h"




/**
 * \fn msiPropertiesNew( msParam_t *listParam, ruleExecInfo_t *rei )
 *
 * \brief Create a new empty property list
 *
 * \module properties
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date    2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[out] listParam - a KeyValPair_MS_T, the newly created property list
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
msiPropertiesNew( msParam_t *listParam, ruleExecInfo_t *rei )
{
	RE_TEST_MACRO( "    Calling msiPropertiesNew" );

	/* Create empty list */
	fillMsParam( listParam, NULL,
		KeyValPair_MS_T, mallocAndZero( sizeof(keyValPair_t) ), NULL );
	return 0;
}

/**
 * \fn msiPropertiesClear( msParam_t *listParam, ruleExecInfo_t *rei )
 *
 * \brief Clear a property list
 *
 * \module properties
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date     2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] listParam - a KeyValPair_MS_T, the property list to clear
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect property list is cleared
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int
msiPropertiesClear( msParam_t *listParam, ruleExecInfo_t *rei )
{
	RE_TEST_MACRO( "    Calling msiPropertiesClear" );

	/* Check parameters */
	if ( strcmp( listParam->type, KeyValPair_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;

	/* Clear list */
	clearKeyVal( (keyValPair_t*)listParam->inOutStruct );
	return 0;
}

/**
 * \fn msiPropertiesClone( msParam_t *listParam, msParam_t *cloneParam, ruleExecInfo_t *rei )
 *
 * \brief Clone a property list, returning a new property list
 *
 * \module properties
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date    2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] listParam - a KeyValPair_MS_T, the property list to clone
 * \param[out] cloneParam - a KeyValPair_MS_T, the returned clone (new property list)
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect a new peoperty list is created
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int
msiPropertiesClone( msParam_t *listParam, msParam_t *cloneParam, ruleExecInfo_t *rei )
{
	RE_TEST_MACRO( "    Calling msiPropertiesClone" );

	/* Check parameters */
	if ( strcmp( listParam->type, KeyValPair_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;

	fillMsParam( cloneParam, NULL,
		KeyValPair_MS_T, mallocAndZero( sizeof(keyValPair_t) ), NULL );
	replKeyVal( (keyValPair_t*)listParam->inOutStruct, (keyValPair_t*)cloneParam->inOutStruct );
	return 0;
}



/**
 * \fn msiPropertiesAdd( msParam_t *listParam, msParam_t* keywordParam, msParam_t* valueParam, ruleExecInfo_t *rei )
 *
 * \brief Add a property and value to a property list.  
 *
 * \module properties
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date    2007
 *
 * \remark Terrell Russell - msi documentation, 2009-06-22
 *
 * \note  If the property is already in the list, its value is changed.
 *    Otherwise the property is added.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] listParam - a KeyValPair_MS_T, the property list to be added to
 * \param[in] keywordParam - a STR_MS_T, a keyword to add
 * \param[in] valueParam - a STR_MS_T, a value to add
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect property list changed
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiPropertiesSet
**/
int
msiPropertiesAdd( msParam_t *listParam, msParam_t* keywordParam, msParam_t* valueParam, ruleExecInfo_t *rei )
{
	RE_TEST_MACRO( "    Calling msiPropertiesAdd" );

	/* Check parameters */
	if ( strcmp( listParam->type, KeyValPair_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;
	if ( strcmp( keywordParam->type, STR_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;
	if ( strcmp( valueParam->type, STR_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;

	/* Add entry */
	addKeyVal( (keyValPair_t*)listParam->inOutStruct,
		(char*)keywordParam->inOutStruct,
		(char*)valueParam->inOutStruct );
	return 0;
}

/**
 * \fn msiPropertiesRemove( msParam_t *listParam, msParam_t* keywordParam, ruleExecInfo_t *rei )
 *
 * \brief Remove a property from the list.
 *
 * \module properties
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date    2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] listParam - a KeyValPair_MS_T, the property list to be removed from
 * \param[in] keywordParam - a STR_MS_T, a keyword to remove
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect property list changed
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int
msiPropertiesRemove( msParam_t *listParam, msParam_t* keywordParam, ruleExecInfo_t *rei )
{
	RE_TEST_MACRO( "    Calling msiPropertiesRemove" );

	/* Check parameters */
	if ( strcmp( listParam->type, KeyValPair_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;
	if ( strcmp( keywordParam->type, STR_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;

	/* Remove entry */
	rmKeyVal( (keyValPair_t*)listParam->inOutStruct,
		(char*)keywordParam->inOutStruct );
	return 0;
}

/**
 * \fn msiPropertiesGet( msParam_t *listParam, msParam_t* keywordParam, msParam_t* valueParam, ruleExecInfo_t *rei )
 *
 * \brief Get the value of a property in a property list.
 *
 * \module properties
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date    2007
 *
 * \note The property list is left unmodified.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] listParam - a KeyValPair_MS_T, the property list to be queried
 * \param[in] keywordParam - a STR_MS_T, a keyword to get
 * \param[out] valueParam - a STR_MS_T, the returned value
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
 * \pre  none
 * \post none
 * \sa none
**/
int
msiPropertiesGet( msParam_t *listParam, msParam_t* keywordParam, msParam_t* valueParam, ruleExecInfo_t *rei )
{
	char* value = NULL;

	RE_TEST_MACRO( "    Calling msiPropertiesGet" );

	/* Check parameters */
	if ( strcmp( listParam->type, KeyValPair_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;
	if ( strcmp( keywordParam->type, STR_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;

	/* Get value and return it */
	value = getValByKey( (keyValPair_t*)listParam->inOutStruct,
		(char*)keywordParam->inOutStruct );
	fillMsParam( valueParam, NULL, STR_MS_T, value, NULL );
	return 0;
}

/**
 * \fn msiPropertiesSet( msParam_t *listParam, msParam_t* keywordParam, msParam_t* valueParam, ruleExecInfo_t *rei )
 *
 * \brief Set the value of a property in a property list.  
 *
 * \module properties
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date     2007
 *
 * \note  If the property is already
 * in the list, its value is changed.  Otherwise the property is added. Same as {@link #msiPropertiesAdd}.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] listParam - a KeyValPair_MS_T, the property list to set within
 * \param[in] keywordParam - a STR_MS_T, a keyword to set
 * \param[out] valueParam - a STR_MS_T, a value
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect  propert value changed
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa msiPropertiesAdd
**/
int
msiPropertiesSet( msParam_t *listParam, msParam_t* keywordParam, msParam_t* valueParam, ruleExecInfo_t *rei )
{
	return msiPropertiesAdd( listParam, keywordParam, valueParam, rei );
}

/**
 * \fn msiPropertiesExists( msParam_t *listParam, msParam_t* keywordParam, msParam_t* trueFalseParam, ruleExecInfo_t *rei )
 *
 * \brief check for a property in a list
 *
 * \module properties
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date    2007
 *
 * \remark Terrell Russell - msi documentation, 2009-06-22
 *
 * \note Return true (integer 1) if the keyword has a property value in the property list,
 * and false (integer 0) otherwise.  The property list is unmodified.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in,out] listParam - a KeyValPair_MS_T, the property list to look in
 * \param[in] keywordParam - a STR_MS_T, a keyword to set
 * \param[out] trueFalseParam - a INT_MS_T, true if set
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
 * \sa  none
**/
int
msiPropertiesExists( msParam_t *listParam, msParam_t* keywordParam, msParam_t* trueFalseParam, ruleExecInfo_t *rei )
{
	char* value = NULL;

	RE_TEST_MACRO( "    Calling msiPropertiesExists" );

	/* Check parameters */
	if ( strcmp( listParam->type, KeyValPair_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;
	if ( strcmp( keywordParam->type, STR_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;

	/* Get value and return true if exists */
	value = getValByKey( (keyValPair_t*)listParam->inOutStruct,
		(char*)keywordParam->inOutStruct );
	fillIntInMsParam( trueFalseParam, (value==NULL) ? 0 : 1 );
	free( (char*)value );
	return 0;
}





/**
 * \fn msiPropertiesToString( msParam_t *listParam, msParam_t* stringParam, ruleExecInfo_t *rei )
 *
 * \brief Convert a property list into a string buffer.  The property list is left unmodified.
 *
 * \module properties
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date     2007
 *
 * \note Convert a property list into a string buffer.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] listParam - a KeyValPair_MS_T, the property list
 * \param[out] stringParam - a STR_MS_T, a string buffer
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
msiPropertiesToString( msParam_t *listParam, msParam_t* stringParam, ruleExecInfo_t *rei )
{
	char* string = NULL;

	RE_TEST_MACRO( "    Calling msiPropertiesToString" );

	/* Check parameters */
	if ( strcmp( listParam->type, KeyValPair_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;

	/* Create and return string */
	keyValToString( (keyValPair_t*)listParam->inOutStruct, &string );
	fillStrInMsParam( stringParam, string );
	return 0;
}

/**
 * \fn msiPropertiesFromString( msParam_t *stringParam, msParam_t* listParam, ruleExecInfo_t *rei )
 *
 * \brief Parse a string into a new property list.  The existing property list, if any, is deleted.
 *
 * \module properties
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date     2007
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] stringParam - a STR_MS_T, a string buffer
 * \param[out] listParam - a KeyValPair_MS_T, the property list with the strings added
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
msiPropertiesFromString( msParam_t *stringParam, msParam_t* listParam, ruleExecInfo_t *rei )
{
	keyValPair_t* list = NULL;

	RE_TEST_MACRO( "    Calling msiPropertiesToString" );

	/* Check parameters */
	if ( strcmp( stringParam->type, STR_MS_T ) != 0 )
		return USER_PARAM_TYPE_ERR;

	/* Parse string and return list */
	if ( keyValFromString( (char*)stringParam->inOutStruct, &list ) == 0 )
		fillMsParam( listParam, NULL, KeyValPair_MS_T, list, NULL );
	return 0;
}

