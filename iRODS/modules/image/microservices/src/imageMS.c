/**
 * @file	imageMS.c
 *
 * @brief	Manipulate images to change their file format,
 * 		get their properties, or change their size,
 * 		orientation, and colors.
 *
 * @author	David R. Nadeau / University of California, San Diego
 *
 * Copyright(c), The Regents of the University of California.
 * For more information, please refer to files in the
 * COPYRIGHT directory.
 */

/* ImageMagick */
/* 	Due to conflicts between ImageMagick and iRODS includes, */
/* 	ImageMagick needs to be included first. */
#include <magick/MagickCore.h>

/* iRODS */
#include "imageMS.h"
#include "imageMSutil.h"
#include "rsApiHandler.h"
#include "apiHeaderAll.h"
#include "fileLseek.h"


/**
 * \fn msiImageConvert( msParam_t *sourceParam, msParam_t* sourceProp,
 *  msParam_t *destParam, msParam_t *destProp, ruleExecInfo_t *rei )
 *
 * \brief   Read a source image file and write it out as a new image file in a chosen format.
 *
 * \module image
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date     2007
 *
 * \note The source and destination image files may be specified as:
 *    \li A string file path
 *    \li An integer file descriptor for an open file
 *    \li A data object
 *
 * \note The destination file will be created if needed.
 *
 * \note The source and destination files have optional property lists.
 * The source property list may select the file format to use
 * and which image in a file to read (if the file contains multiple
 * images).  The destination property list may select the file
 * format to write and compression flags to use.  EXIF tags
 * in the destination property list will be added to the image
 * if they don't conflict with the output format and if the
 * output format supports them.
 *    
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] sourceParam - the source file
 * \param[in] sourceProp - the source properties
 * \param[in] destParam - the destination file
 * \param[in] destProp - the destination properties
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
msiImageConvert( msParam_t *sourceParam, msParam_t* sourceProp,
	msParam_t *destParam, msParam_t *destProp, ruleExecInfo_t *rei )
{
	rsComm_t *rsComm = NULL;
	char* format = NULL;

	ImageFileParameter_t source;
	ImageFileParameter_t destination;

	dataObjInp_t sourceObject;
	dataObjInp_t destinationObject;

	source.dataObject      = &sourceObject;
	destination.dataObject = &destinationObject;


	RE_TEST_MACRO( "    Calling msiImageConvert" );

	/*
	 * Parse arguments.
	 *	Get the communications handle
	 *	Get source file.
	 *	Get destination file.
	 *	Get format name.
	 */
	if ( rei == NULL || rei->rsComm == NULL )
	{
		rodsLog( LOG_ERROR, "msiImageConvert:  input rei or rsComm is NULL" );
		return SYS_INTERNAL_NULL_INPUT_ERR;
	}
	rsComm = rei->rsComm;

	if ( (rei->status = _ImageGetFileParameter( rsComm, "msiImageConvert:  source file",
		sourceParam, &source )) < 0 )
		return rei->status;	/* Error */

	source.properties = (keyValPair_t*)mallocAndZero( sizeof( keyValPair_t ) );
	if ( (rei->status = _ImageGetPropertyListParameter( rsComm, "msiImageConvert:  source properties",
		sourceProp, &(source.properties) )) < 0 )
	{
		free( (char*)source.properties );
		return rei->status;	/* Error */
	}

	if ( (rei->status = _ImageGetFileParameter( rsComm, "msiImageConvert:  destination file",
		destParam, &destination )) < 0 )
	{
		free( (char*)source.properties );
		return rei->status;	/* Error */
	}

	destination.properties = (keyValPair_t*)mallocAndZero( sizeof( keyValPair_t ) );
	if ( (rei->status = _ImageGetPropertyListParameter( rsComm, "msiImageConvert:  destination properties",
		destProp, &(destination.properties) )) < 0 )
	{
		free( (char*)source.properties );
		free( (char*)destination.properties );
		return rei->status;	/* Error */
	}


	/*
	 * Initialize ImageMagick.
	 */
	MagickCoreGenesis( "", MagickFalse );


	/*
	 * Process files.
	 *	Read in the image.
	 *	Write out the image.
	 */
	rei->status = _ImageReadFile( rsComm, "msiImageConvert:  source file",
		&source );
	if ( rei->status >= 0 )
	{
		/* Read success.  Write. */
		destination.image = source.image;
		rei->status = _ImageWriteFile( rsComm, "msiImageConvert:  destination file",
			&destination );
	}


	/*
	 * Finalize ImageMagick.
	 */
	if ( source.image != NULL )
		DestroyImage( source.image );
	MagickCoreTerminus( );

	free( (char*)source.properties );
	free( (char*)destination.properties );
	return rei->status;

}


/**
 * \fn msiImageGetProperties( msParam_t *sourceParam, msParam_t* sourceProp,
 * msParam_t *listParam, ruleExecInfo_t *rei )
 *
 * \brief   Get the properties of an image file.
 *
 * \module image
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date    2007
 *
 * \note The source and destination image files may be specified as:
 *    \li A string file path
 *    \li An integer file descriptor for an open file
 *    \li A data object
 *    
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] sourceParam - the source file
 * \param[in] sourceProp - the source properties
 * \param[out] listParam - the returned properties list
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
msiImageGetProperties( msParam_t *sourceParam, msParam_t* sourceProp,
	msParam_t *listParam, ruleExecInfo_t *rei )
{
	rsComm_t *rsComm = NULL;
	ImageFileParameter_t source;
	dataObjInp_t sourceObject;
	keyValPair_t* list = NULL;

	source.dataObject = &sourceObject;


	RE_TEST_MACRO( "    Calling msiImageGetProperties" );


	/*
	 * Parse arguments.
	 *	Get the communications handle
	 *	Get source file.
	 */
	if ( rei == NULL || rei->rsComm == NULL )
	{
		rodsLog( LOG_ERROR,
			"msiImageGetProperties:  input rei or rsComm is NULL" );
		return SYS_INTERNAL_NULL_INPUT_ERR;
	}
	rsComm = rei->rsComm;

	if ( (rei->status = _ImageGetFileParameter( rsComm, "msiImageGetProperties:  source file",
		sourceParam, &source )) < 0 )
		return rei->status;	/* Error */

	source.properties = (keyValPair_t*)mallocAndZero( sizeof( keyValPair_t ) );
	if ( (rei->status = _ImageGetPropertyListParameter( rsComm, "msiImageGetProperties:  source properties",
		sourceProp, &(source.properties) )) < 0 )
	{
		free( (char*)source.properties );
		return rei->status;	/* Error */
	}



	/*
	 * Initialize the property list.
	 */
	list = (keyValPair_t*)mallocAndZero( sizeof(keyValPair_t) );
	fillMsParam( listParam, NULL, KeyValPair_MS_T, list, NULL );
	if ( source.type == IMAGEFILEPARAMETER_PATH )
	{
		/* Save the filename as a property. */
		addKeyVal( list, "file.Name", source.path );
	}


	/*
	 * Initialize ImageMagick.
	 */
	MagickCoreGenesis( "", MagickFalse );


	/*
	 * Process files.
	 *	Read in the image.
	 *	Get its properties.
	 */
	rei->status = _ImageReadFile( rsComm, "msiImageGetProperties:  source file",
		&source );
	if ( rei->status >= 0 )
	{
		/* Read success.  Get properties. */
		rei->status = _ImageGetProperties( &source, list );
	}


	/*
	 * Finalize ImageMagick.
	 */
	if ( source.image != NULL )
		DestroyImage( source.image );
	MagickCoreTerminus( );

	return rei->status;
}

/**
 * \fn msiImageScale( msParam_t* sourceParam, msParam_t* sourceProp,
 * msParam_t* xScaleFactor, msParam_t* yScaleFactor,
 * msParam_t* destParam, msParam_t* destProp, ruleExecInfo_t* rei )
 *
 * \brief   Read a source image file, scale it up or down in size, and write it
 *    out as a new image file in a chosen format.
 *
 * \module image
 *
 * \since pre-2.1
 *
 * \author  David R. Nadeau / University of California, San Diego
 * \date    2007
 *
 * \note The source and destination image files may be specified as:
 *    \li A string file path
 *    \li An integer file descriptor for an open file
 *    \li A data object
 *    
 * \note The destination file will be created if needed.
 *
 * \note The source and destination files have optional property lists.
 * The source property list may select the file format to use
 * and which image in a file to read (if the file contains multiple
 * images).  The destination property list may select the file
 * format to write and compression flags to use.  EXIF tags
 * in the destination property list will be added to the image
 * if they don't conflict with the output format and if the
 * output format supports them.
 *
 * \note The source image is scaled by the given X and Y scale factors.
 *
 * \usage See clients/icommands/test/rules3.0/
 *
 * \param[in] sourceParam - the source file
 * \param[in] sourceProp - the source properties
 * \param[in] xScaleFactor - the X scale factor
 * \param[in] yScaleFactor - the Y scale factor
 * \param[in] destParam - the destination file
 * \param[in] destProp - the destination properties
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
msiImageScale( msParam_t* sourceParam, msParam_t* sourceProp,
	msParam_t* xScaleFactor, msParam_t* yScaleFactor,
	msParam_t* destParam, msParam_t* destProp, ruleExecInfo_t* rei )
{
	rsComm_t *rsComm = NULL;
	char* format = NULL;

	ImageFileParameter_t source;
	ImageFileParameter_t destination;

	double xScale = 1.0;
	double yScale = 1.0;

	dataObjInp_t sourceObject;
	dataObjInp_t destinationObject;

	source.dataObject      = &sourceObject;
	destination.dataObject = &destinationObject;


	RE_TEST_MACRO( "    Calling msiImageScale" );

	/*
	 * Parse arguments.
	 *	Get the communications handle
	 *	Get source file.
	 *	Get destination file.
	 *	Get format name.
	 */
	if ( rei == NULL || rei->rsComm == NULL )
	{
		rodsLog( LOG_ERROR, "msiImageScale:  input rei or rsComm is NULL" );
		return SYS_INTERNAL_NULL_INPUT_ERR;
	}
	rsComm = rei->rsComm;

	if ( (rei->status = _ImageGetFileParameter( rsComm, "msiImageScale:  source file",
		sourceParam, &source )) < 0 )
		return rei->status;	/* Error */

	source.properties = (keyValPair_t*)mallocAndZero( sizeof( keyValPair_t ) );
	if ( (rei->status = _ImageGetPropertyListParameter( rsComm, "msiImageScale:  source properties",
		sourceProp, &(source.properties) )) < 0 )
	{
		free( (char*)source.properties );
		return rei->status;	/* Error */
	}

	if ( (rei->status = _ImageGetDouble( rsComm, "msiImageScale:  source X scale",
		xScaleFactor, &xScale )) < 0 )
	{
		free( (char*)source.properties );
		return rei->status;	/* Error */
	}

	if ( (rei->status = _ImageGetDouble( rsComm, "msiImageScale:  source Y scale",
		yScaleFactor, &yScale )) < 0 )
	{
		free( (char*)source.properties );
		return rei->status;	/* Error */
	}

	if ( (rei->status = _ImageGetFileParameter( rsComm, "msiImageScale:  destination file",
		destParam, &destination )) < 0 )
	{
		free( (char*)source.properties );
		return rei->status;	/* Error */
	}

	destination.properties = (keyValPair_t*)mallocAndZero( sizeof( keyValPair_t ) );
	if ( (rei->status = _ImageGetPropertyListParameter( rsComm, "msiImageScale:  destination properties",
		destProp, &(destination.properties) )) < 0 )
	{
		free( (char*)source.properties );
		free( (char*)destination.properties );
		return rei->status;	/* Error */
	}


	/*
	 * Initialize ImageMagick.
	 */
	MagickCoreGenesis( "", MagickFalse );


	/*
	 * Process files.
	 *	Read in the image.
	 *	Write out the image.
	 */
	rei->status = _ImageReadFile( rsComm, "msiImageScale:  source file",
		&source );
	if ( rei->status >= 0 )
	{
		/* Read success.  Scale. */

		/* Write */
		destination.image = source.image;
		rei->status = _ImageWriteFile( rsComm, "msiImageScale:  destination file",
			&destination );
	}


	/*
	 * Finalize ImageMagick.
	 */
	if ( source.image != NULL )
		DestroyImage( source.image );
	MagickCoreTerminus( );

	free( (char*)source.properties );
	free( (char*)destination.properties );
	return rei->status;
}

