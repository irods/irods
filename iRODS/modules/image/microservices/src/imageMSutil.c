/*
 * @file  imageMSutil.c
 *
 * @brief Helper functions for manipulating images for the image microservices.
 *
 * @author  David R. Nadeau / University of California, San Diego
 *
 * Copyright(c), The Regents of the University of California.
 * For more information, please refer to files in the
 * COPYRIGHT directory.
 */

/* ImageMagick */
/*  Due to conflicts between ImageMagick and iRODS includes, */
/*  ImageMagick needs to be included first. */
#include <magick/MagickCore.h>

/* iRODS */
#include "imageMS.h"
#include "imageMSutil.h"
#include "rsApiHandler.h"
#include "apiHeaderAll.h"
#include "dataObjOpen.h"
#include "dataObjLseek.h"
#include "dataObjClose.h"
#include "fileLseek.h"
#include "dataObjInpOut.h"



/*
 * Helper functions for parsing parameters
 */

/**
 * @brief		Get and check a file parameter.
 *
 * The file parameter can be one of the following types:
 *
 * 	@li A string file path.
 * 	@li A string file descriptor.
 * 	@li An integer file descriptor.
 * 	@li A data object parameter set.
 *
 * The parameter is parsed and stored in the given ImageFileParameter structure
 * and a status code returned.  On a non-zero status code, the
 * result structure is undefined.
 *
 * @param[in]	rsComm		the communications link
 * @param[in]	messageBase	the base message for errors
 * @param[in]	param		an msParam_t* for the argument to be parsed
 * @param[out]	result		an ImageFileParameter_t* for the returned argument
 * @return			a status code, 0 on success
 */
int
_ImageGetFileParameter( rsComm_t* rsComm, char* messageBase,
	msParam_t* param, ImageFileParameter_t *result )
{
	dataObjInp_t *resultObject;
	int status;


	/* If the incoming parameter is NULL, it's an error. */
	if ( param == NULL )
	{
		status = SYS_INTERNAL_NULL_INPUT_ERR;
		rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
			"%s:  NULL file argument",
			messageBase );
		return status;
	}

	/* If the parameter is an integer, we have a file descriptor. */
	/* The descriptor must be positive. */
	if ( strcmp( param->type, INT_MS_T ) == 0 )
	{
		result->fd   = *(int *)param->inOutStruct;
		result->type = IMAGEFILEPARAMETER_FD;
		if ( result->fd <= 0 )
		{
			status = USER_PARAM_TYPE_ERR;
			rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
				"%s:  file argument descriptor %d is negative",
				messageBase, result->fd );
			return status;
		}
		return 0;
	}


	/* If the parameter is a string, it may be a file path or */
	/* a string representation of a file descriptor.  It cannot */
	/* be NULL. */
	if ( strcmp( param->type, STR_MS_T ) == 0 )
	{
		char* endParse = NULL;

		result->path = (char*)param->inOutStruct;
		result->type = IMAGEFILEPARAMETER_PATH;
		if ( result->path == NULL || strcmp( result->path, "null" ) == 0 )
		{
			status = SYS_NULL_INPUT;
			rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
				"%s:  file argument is a 'null' path",
				messageBase );
			return status;
		}

		/* Is it a file descriptor number?  If so, it */
		/* must be positive. */
		result->fd = (int)strtol( result->path, &endParse, 10 );
		if ( result->path != endParse )
		{
			result->path = NULL;
			result->type = IMAGEFILEPARAMETER_FD;
			if ( result->fd <= 0 )
			{
				status = USER_PARAM_TYPE_ERR;
				rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
					"%s:  file argument descriptor %d is negative",
					messageBase, result->fd );
				return status;
			}
		}
		return 0;
	}


	/* Otherwise parse it as a object or as key-value pairs. */
	status = parseMspForDataObjInp( param, result->dataObject, &resultObject, 0 );
	if ( status < 0 )
	{
		rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
			"%s:  file argument problem, status = %d",
			messageBase, status );
		return status;
	}

	result->type = IMAGEFILEPARAMETER_OBJECT;
	result->dataObject = resultObject;
	return 0;
}

/**
 * @brief		Get, check, and parse a property list parameter.
 *
 * The property list must be a string representation of properties.
 *
 * @param[in]	rsComm		the communications link
 * @param[in]	messageBase	the base message for errors
 * @param[in]	param		the msParam_t* of the input parameter
 * @param[out]	result		the keyValPair_t** property list
 * @return			the status code, 0 on success
 */
int
_ImageGetPropertyListParameter( rsComm_t* rsComm, char* messageBase,
	msParam_t* param, keyValPair_t** result )
{
	/* If the incoming parameter is NULL, do nothing. */
	/* This isn't an error.  The user is allowed to leave */
	/* property lists unspecified. */
	if ( param == NULL )
		return 0;

	/* If the parameter is not a string, it's an error */
	if ( strcmp( param->type, STR_MS_T ) != 0 )
	{
		int status = USER_PARAM_TYPE_ERR;
		rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
			"%s:  property list argument is not a string",
		       messageBase );
		return status;
	}

	/* Parse the string. */
	keyValFromString( (char*)param->inOutStruct, result );
	return 0;
}

/**
 * @brief		Get, check, and parse a double parameter.
 *
 * The parameter must be an integer, double, or string, all of
 * which are converted to a returned double.
 *
 * @param[in]	rsComm		the communications link
 * @param[in]	messageBase	the base message for errors
 * @param[in]	param		the msParam_t* of the input parameter
 * @param[out]	result		the returned double* value
 * @return			the status code, 0 on success
 */
int
_ImageGetDouble( rsComm_t* rsComm, char* messageBase,
	msParam_t* param, double* result )
{
	/* If the incoming parameter is NULL, it's an error. */
	if ( param == NULL )
	{
		int status = USER_PARAM_TYPE_ERR;
		rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
			"%s:  double argument is null",
		       messageBase );
		return status;
	}

	/* If the parameter is an integer, cast it and */
	/* convert to a double. */
	if ( strcmp( param->type, INT_MS_T ) != 0 )
	{
		*result = (double)(*(int *)param->inOutStruct);
		return 0;
	}

	/* If the parameter is a double, cast it and */
	/* convert to a double. */
	if ( strcmp( param->type, DOUBLE_MS_T ) != 0 )
	{
		*result = *(double *)param->inOutStruct;
		return 0;
	}

	/* If the parameter is not a string, it's an error */
	if ( strcmp( param->type, STR_MS_T ) != 0 )
	{
		int status = USER_PARAM_TYPE_ERR;
		rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
			"%s:  format argument is not a string",
		       messageBase );
		return status;
	}

	/* Parse the string. */
	*result = atof( (char*)param->inOutStruct );
	return 0;
}





/*
 * Helper functions for reading and writing files.
 */

/**
 * @brief		Open and read an image file.
 *
 * The file is opened, if not already open, and read from the
 * start of the file.  The file is interpreted as an image
 * and an image object is added to the file parameter.
 *
 * @param[in]	rsComm		the communications link
 * @param[in]	messageBase	the base message for errors
 * @param[in,out]	file		the ImageFileParameter_t* for the file
 * @return			the status code, 0 on success
 */
int
_ImageReadFile( rsComm_t* rsComm, char* messageBase,
	ImageFileParameter_t* file )
{
/*	fileLseekInp_t seekParam;	*/
	fileLseekOut_t* seekResult = NULL;
/*	dataObjReadInp_t readParam;
	dataObjCloseInp_t closeParam; */
	dataObjInp_t openParam;

	int fd = -1;
	int fileWasOpened = FALSE;
	int fileLength = 0;
	bytesBuf_t data;
	char* format = NULL;
	int status;
	
	openedDataObjInp_t seekParam_of_another_color;
	openedDataObjInp_t closeParam_of_another_color;
	openedDataObjInp_t readParam_of_another_color;

	ImageInfo* info       = NULL;
	ExceptionInfo* errors = NULL;

	memset( &openParam,  0, sizeof(dataObjInp_t) );
/*	memset( &seekParam,  0, sizeof(fileLseekInp_t) );
	memset( &readParam,  0, sizeof(dataObjReadInp_t) );
	memset( &closeParam, 0, sizeof(dataObjCloseInp_t) ); */
	memset( &data,       0, sizeof(bytesBuf_t) );
	
	memset( &seekParam_of_another_color, 0, sizeof(openedDataObjInp_t));
	memset( &closeParam_of_another_color, 0, sizeof(openedDataObjInp_t));
	memset( &readParam_of_another_color, 0, sizeof(openedDataObjInp_t));


	/* If the file argument is a file path, */
	/* open the file. */
	if ( file->type == IMAGEFILEPARAMETER_PATH )
	{
		rstrcpy( openParam.objPath, file->path, MAX_NAME_LEN );
		openParam.openFlags = O_RDONLY;

		status = rsDataObjOpen( rsComm, &openParam );
		if ( status < 0 )
		{
			rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
				"%s:  could not open file, status = %d",
				messageBase, status );
			return status;
		}
		fd = status;
		fileWasOpened = TRUE;
	}

	/* If the file argument is a data object, */
	/* open the file. */
	else if ( file->type == IMAGEFILEPARAMETER_OBJECT )
	{
		/* Open the file. */
		status = rsDataObjOpen( rsComm, file->dataObject );
		if ( status < 0 )
		{
			rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
				"%s:  could not open file, status = %d",
				messageBase, status );
			return status;
		}
		fd = status;
		fileWasOpened = TRUE;
	}

	/* Otherwise the file argument is a file */
	/* descriptor and the file is already open. */
	else
	{
		fd = file->fd;
		fileWasOpened = FALSE;
	}


	/* Seek to the end of the file to get its size. */
	/*seekParam.fileInx = fd;
	seekParam.offset  = 0;
	seekParam.whence  = SEEK_END; */
	
	/* Put the same values into the structure that the function is expecting. */
	seekParam_of_another_color.l1descInx = fd;
	seekParam_of_another_color.offset = 0;
	seekParam_of_another_color.whence = SEEK_END;
	
	status = rsDataObjLseek( rsComm, &seekParam_of_another_color, &seekResult );
	if ( status < 0 )
	{
		rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
			"%s:  could not seek to end of file, status = %d",
			messageBase, status );

		/* Try to close the file we opened, ignoring errors. */
		if ( fileWasOpened )
		{
		/*	closeParam.l1descInx = fd;	*/		
			closeParam_of_another_color.l1descInx = fd;
			rsDataObjClose( rsComm, &closeParam_of_another_color );
		}
		return status;
	}
	fileLength = seekResult->offset;

	/* Reset to the start. */
	/*seekParam.offset  = 0;
	seekParam.whence  = SEEK_SET; */
	
	seekParam_of_another_color.offset = 0;
	seekParam_of_another_color.whence = SEEK_SET;
	
	status = rsDataObjLseek( rsComm, &seekParam_of_another_color, &seekResult );
	if ( status < 0 )
	{
		rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
			"%s:  could not seek to start of file, status = %d",
			messageBase, status );

		/* Try to close the file we opened, ignoring errors.*/
		if ( fileWasOpened )
		{
		/*	closeParam.l1descInx = fd;	*/
			closeParam_of_another_color.l1descInx = fd;
			rsDataObjClose( rsComm, &closeParam_of_another_color );
		}
		return status;
	}


	/* Read the file into a big buffer. */
/*	readParam.l1descInx = fd;
	readParam.len       = fileLength;	*/
	readParam_of_another_color.l1descInx = fd;
	readParam_of_another_color.len       = fileLength;

	data.len            = fileLength;
	data.buf            = (void*)malloc( fileLength );

	status = rsDataObjRead( rsComm, &readParam_of_another_color, &data );
	if ( status < 0 )
	{
		rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
			"%s:  could not read file, status = %d",
			messageBase, status );
		free( (char*)data.buf );

		/* Try to close the file we opened, ignoring errors. */
		if ( fileWasOpened )
		{
		/*	closeParam.l1descInx = fd;	*/
			closeParam_of_another_color.l1descInx = fd;
			rsDataObjClose( rsComm, &closeParam_of_another_color );
		}
		return status;
	}


	/* Close the file we opened. */
	if ( fileWasOpened )
	{
	/*	closeParam.l1descInx = fd;	*/
		closeParam_of_another_color.l1descInx = fd;
		status = rsDataObjClose( rsComm, &closeParam_of_another_color );
		if ( status < 0 )
		{
			rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
				"%s:  could not close file, status = %d",
				messageBase, status );
			free( (char*)data.buf );
			return status;
		}
		fd = -1;
	}


	/* Allocate ImageMagick structures. */
	errors = AcquireExceptionInfo( );
	info   = AcquireImageInfo( );

	/* Guess the file's format based upon properties */
	/* and a file name extension, if any.  A NULL is */
	/* returned if there is no guess and then we let */
	/* ImageMagick guess the format on its own. */
	format = _ImageGuessFormat( file );
	if ( format != NULL )
	{
		strncpy( info->magick, format, MaxTextExtent );
		free( (char*)format );
	}


	/* Interpret the data as an image. */
	file->image = BlobToImage( info, data.buf, data.len, errors );
	CatchException( errors );
	free( (char*)data.buf );
	if ( file->image == NULL )
	{
		status = ACTION_FAILED_ERR;
		rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
			"%s:  could not parse image file, status = %d",
			messageBase, status );
		DestroyExceptionInfo( errors );
		DestroyImageInfo( info );
		return status;
	}

	return 0;
}


/**
 * @brief		Open or create and write an image file.
 *
 * If the file isn't already opened, it is opened, creating it if
 * necessary.  The given image is written to the file.  If the
 * file was opened, it is closed.
 *
 * @param[in]	rsComm		the communications link
 * @param[in]	messageBase	the base message for errors
 * @param[in]	file		the ImageFileParameter_t* for the file
 * @return			the status code, 0 on success
 */
int
_ImageWriteFile( rsComm_t* rsComm, char* messageBase,
	ImageFileParameter_t* file )
{
/*	dataObjWriteInp_t writeParam;
	dataObjCloseInp_t closeParam; */
	dataObjInp_t openParam;

	openedDataObjInp_t closeParam_of_another_color;
	openedDataObjInp_t writeParam_of_another_color;


	ImageInfo* info       = NULL;
	ExceptionInfo* errors = NULL;

	int status;
	int fd = -1;
	int fileWasOpened = FALSE;
	unsigned long fileLength = 0;
	char* format = NULL;
	bytesBuf_t data;

	memset( &openParam,   0, sizeof(dataObjInp_t) );
/*	memset( &writeParam,  0, sizeof(dataObjWriteInp_t) );
	memset( &closeParam,  0, sizeof(dataObjCloseInp_t) );	*/
	memset( &data,        0, sizeof(bytesBuf_t) );
	memset( &closeParam_of_another_color,  0, sizeof(openedDataObjInp_t) );
	memset( &writeParam_of_another_color,  0, sizeof(openedDataObjInp_t) );



	/* Allocate ImageMagick structures. */
	errors = AcquireExceptionInfo( );
	info   = AcquireImageInfo( );


	/* Guess the file's format based upon properties */
	/* and a file name extension, if any.  A NULL is */
	/* returned if there is no guess and then we let */
	/* ImageMagick guess the format on its own.  This */
	/* will probably generate an error. */
	format = _ImageGuessFormat( file );
	if ( format != NULL )
	{
		strncpy( info->magick, format, MaxTextExtent );
		free( (char*)format );
	}

/**** TODO:  add properties ****/


	/* Convert the image into a raw byte buffer in the */
	/* selected file format. */
	data.buf = ImageToBlob( info, file->image, &fileLength, errors );
	data.len = (int)fileLength;
	CatchException( errors );
	if ( data.buf == NULL )
	{
		status = ACTION_FAILED_ERR;
		rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
			"%s:  could not convert to format %s, status = %d",
			messageBase,
			(info->magick == NULL) ? "unknown" : info->magick, status );
		DestroyExceptionInfo( errors );
		DestroyImageInfo( info );
		return status;
	}


	/* Free ImageMagick structures, except for the image. */
	/* The caller is responsible for freeing the image. */
	DestroyExceptionInfo( errors );
	DestroyImageInfo( info );


	/* If the file argument is a path, create the file. */
	if ( file->type == IMAGEFILEPARAMETER_PATH )
	{
		rstrcpy( openParam.objPath, file->path, MAX_NAME_LEN );
		openParam.openFlags  = O_WRONLY|O_CREAT;
		addKeyVal( &openParam.condInput, DATA_TYPE_KW, "generic" );

		status = rsDataObjOpen( rsComm, &openParam );
		if ( status < 0 )
		{
			rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
				"%s:  could not create file, status = %d",
				messageBase, status );
			return status;
		}
		fd = status;
		fileWasOpened = TRUE;
	}

	/* If the file argument is a data object, create the file. */
	else if ( file->type == IMAGEFILEPARAMETER_OBJECT )
	{
		status = rsDataObjOpen( rsComm, file->dataObject );
		if ( status < 0 )
		{
			rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
				"%s:  could not create file, status = %d",
				messageBase, status );
			return status;
		}
		fd = status;
		fileWasOpened = TRUE;
	}

	/* Otherwise the file argument was a file descriptor */
	/* and the file is already open.  We'll write at whatever */
	/* location the file pointer is at in the file. */
	else
	{
		fd = file->fd;
		fileWasOpened = FALSE;
	}


	/* Write the buffer to the file */
	writeParam_of_another_color.l1descInx = fd;
	writeParam_of_another_color.len       = data.len;
	status = rsDataObjWrite( rsComm, &writeParam_of_another_color, &data );
	if ( status < 0 )
	{
		rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
			"%s:  could not write file, status = %d",
			messageBase, status );
		free( (char*)data.buf );
		return status;
	}
	free( (char*)data.buf );


	/* Close the file if we opened it. */
	if ( fileWasOpened )
	{
	/*	closeParam.l1descInx = fd;	*/
		closeParam_of_another_color.l1descInx = fd;
		status = rsDataObjClose( rsComm, &closeParam_of_another_color );
		if ( status < 0 )
		{
			rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, status,
				"%s:  could not close file, status = %d",
				messageBase, status );
			return status;
		}
		fd = -1;
		fileWasOpened = FALSE;
	}

	return 0;
}


/**
 * @brief		Get the chosen image file format for
 * 			a file parameter.
 *
 * Without reading the file, the file format if the file is guessed
 * based upon user-supplied properties or the file's filename extension.
 * If neither of these yields a file format name, a NULL is returned.
 *
 * @param[in]	file		the file parameter to look at
 * @return			the guessed file format
 */
char*
_ImageGuessFormat( ImageFileParameter_t* file )
{
	/* Look at the file's property list, if any. */
	if ( file->properties != NULL )
	{
		/* Look for a file format property. */
		char* value = getValByKey( file->properties, PROPERTY_IMAGE_FORMAT );
		if ( value != NULL )
			return value;
	}

	/* Look at the file's name, if any. */
	if ( file->type == IMAGEFILEPARAMETER_PATH )
	{
		char* dot = rindex( file->path, '.' );
		if ( dot != NULL && *(dot+1) != '\0' )
			return strdup( dot+1 );
	}

	/* Unknown format. */
	return NULL;
}

/**
 * @brief		Get the properties of the image.
 *
 * The image's properties are assembled from two sources:
 *
 * 	@li The image's tags (such as EXIF)
 * 	@li The image's characteristics
 *
 * Some image file formats will have no tags, but all images
 * will have characteristics obtained by other data in the
 * file (such as color depth and file format).
 *
 * Properties from these two sources are added to the given
 * property list.
 *
 * @param[in]	file		the image file
 * @param[out]	list		the returned property list
 * @return			the status code, 0 on success
 */
int
_ImageGetProperties( ImageFileParameter_t* file, keyValPair_t* list )
{
	char* value = NULL;
	char* name  = NULL;
	char buffer[1000];
	const int lenPrefix = strlen( PROPERTY_IMAGE_PREFIX );

	/*
	 * Get ImageMagick's well-defined properties, and particularly
	 * those from EXIF.
	 */
	GetImageProperty( file->image, "exif:*" );
	GetImageProperty( file->image, "8bim:*" );
	GetImageProperty( file->image, "fx:*" );
	GetImageProperty( file->image, "iptc:*" );
	GetImageProperty( file->image, "pixel:*" );
	GetImageProperty( file->image, "xmp:*" );
	GetImageProperty( file->image, "*" );

	ResetImagePropertyIterator( file->image );
	for ( name = GetNextImageProperty( file->image ); name != NULL;
		name = GetNextImageProperty( file->image ) )
	{
		char* newname = NULL;
		int len = 0;
		int i;

		/* Get the value. */
		value = (char*)GetImageProperty( file->image, name );
		if ( value == NULL || *value == '\0' )
			continue;	/* Null values ignored */

		/* Prefix all property names with the image prefix. */
		len = strlen( name ) + lenPrefix;	/* 'image.' + name */
		newname = (char*)malloc( len + 1 );	/* newname + '\0' */
		strcpy( newname, PROPERTY_IMAGE_PREFIX );
		strcat( newname, name );
/* Should this be freed?
		free( (char*)name );
*/

		/* Replace all ':' separators with '.' to be */
		/* compliant with iRODS property conventions. */
		for ( i = 0; i < len; i++ )
		{
			if ( newname[i] == ':' )
				newname[i] = '.';
		}

		/* Add the name and value. */
		addKeyVal( list, newname, value );
	}


	/*
	 * Add generic image properties to cover the cases where an
	 * image has no EXIF properties, but we still want to know
	 * the image's attributes.  We intentionally do not name
	 * these properties EXIF so that we can distinguish between
	 * those that come from a file's EXIF content and those we
	 * are inferring by examining the image data itself.
	 */
	if ( file->image->magick != NULL )
	{
		/* Image file format.  Should be, but isn't */
		/* always in EXIF. */
		addKeyVal( list, PROPERTY_IMAGE_FORMAT, file->image->magick );
	}

	switch ( file->image->compression )
	{
	default:
	case UndefinedCompression:	value = NULL;		break;
	case NoCompression:		value = "none";		break;
	case BZipCompression:		value = "bzip";		break;
	case FaxCompression:		value = "fax";		break;
	case Group4Compression:		value = "group4";	break;
	case JPEGCompression:		value = "jpeg";		break;
	case JPEG2000Compression:	value = "jpeg2000";	break;
	case LosslessJPEGCompression:	value = "jpeglossless";	break;
	case LZWCompression:		value = "lzw";		break;
	case RLECompression:		value = "rle";		break;
	case ZipCompression:		value = "zip";		break;
	}
	if ( value != NULL )
	{
		/* exif:Compression (but as a number, not a string) */
		addKeyVal( list, PROPERTY_IMAGE_COMPRESSION, value );
	}

	if ( file->image->quality > 0 )
	{
		sprintf( buffer, "%D", file->image->quality );
		addKeyVal( list, PROPERTY_IMAGE_COMPRESSIONQUALITY, buffer );
	}

	switch ( file->image->interlace )
	{
	default:
	case UndefinedInterlace:	value = NULL;		break;
	case NoInterlace:		value = "none";		break;
	case LineInterlace:		value = "line";		break;
	case PlaneInterlace:		value = "plane";	break;
	case PartitionInterlace:	value = "partition";	break;
	case GIFInterlace:		value = "gif";		break;
	case JPEGInterlace:		value = "jpeg";		break;
	case PNGInterlace:		value = "png";		break;
	}
	if ( value != NULL )
	{
		/* No exif equivalent. */
		addKeyVal( list, PROPERTY_IMAGE_INTERLACE, value );
	}

	switch ( file->image->colorspace )
	{
	default:
	case UndefinedColorspace:	value = NULL;		break;
	case RGBColorspace:		value = "rgb";		break;
	case GRAYColorspace:		value = "gray";		break;
	case TransparentColorspace:	value = "transparent";	break;
	case OHTAColorspace:		value = "ohta";		break;
	case LABColorspace:		value = "lab";		break;
	case XYZColorspace:		value = "xyz";		break;
	case YCbCrColorspace:		value = "ycbcr";	break;
	case YCCColorspace:		value = "ycc";		break;
	case YIQColorspace:		value = "yiq";		break;
	case YPbPrColorspace:		value = "ypbpr";	break;
	case YUVColorspace:		value = "yuv";		break;
	case CMYKColorspace:		value = "cmyk";		break;
	case sRGBColorspace:		value = "srgb";		break;
	case HSBColorspace:		value = "hsb";		break;
	case HSLColorspace:		value = "hsl";		break;
	case HWBColorspace:		value = "hwb";		break;
	case Rec601LumaColorspace:	value = "rec601luma";	break;
	case Rec601YCbCrColorspace:	value = "rec601ycbcr";	break;
	case Rec709LumaColorspace:	value = "rec709luma";	break;
	case Rec709YCbCrColorspace:	value = "rec709ycbcr";	break;
	}
	if ( value != NULL )
	{
		/* exif:ColorSpace (but as a number, not a string) */
		addKeyVal( list, PROPERTY_IMAGE_COLORSPACE, value );
	}

	if ( file->image->depth > 0 )
	{
		/* No exif equivalent.  Must be derived from content. */
		sprintf( buffer, "%D", file->image->depth );
		addKeyVal( list, PROPERTY_IMAGE_DEPTH, buffer );
	}

	if ( file->image->colors > 0 )
	{
		/* No exif equivalent.  Must be derived from color table. */
		sprintf( buffer, "%D", file->image->colors );
		addKeyVal( list, PROPERTY_IMAGE_COLORS, buffer );
	}

	if ( file->image->gamma > 0.0 )
	{
		/* No exif equivalent. */
		sprintf( buffer, "%F", file->image->gamma );
		addKeyVal( list, PROPERTY_IMAGE_GAMMA, buffer );
	}

	if ( file->image->x_resolution > 0.0 )
	{
		/* exif:XResolution */
		sprintf( buffer, "%F", file->image->x_resolution );
		addKeyVal( list, PROPERTY_IMAGE_XRESOLUTION, buffer );
	}

	if ( file->image->y_resolution > 0.0 )
	{
		/* exif:YResolution */
		sprintf( buffer, "%F", file->image->y_resolution );
		addKeyVal( list, PROPERTY_IMAGE_YRESOLUTION, buffer );
	}

	switch ( file->image->units )
	{
	default:
	case UndefinedResolution:		value = NULL;		break;
	case PixelsPerInchResolution:		value = "pixelsperinch";	break;
	case PixelsPerCentimeterResolution:	value = "pixelspercentimeter";	break;
	}
	if ( value != NULL )
	{
		/* exif:ResolutionUnit (but as a number, not a string) */
		addKeyVal( list, PROPERTY_IMAGE_RESOLUTIONUNIT, value );
	}

	switch ( file->image->orientation )
	{
	default:
	case UndefinedOrientation:	value = NULL;		break;
	case TopLeftOrientation:	value = "topleft";	break;
	case TopRightOrientation:	value = "topright";	break;
	case BottomRightOrientation:	value = "bottomright";	break;
	case BottomLeftOrientation:	value = "bottomleft";	break;
	case LeftTopOrientation:	value = "lefttop";	break;
	case RightTopOrientation:	value = "righttop";	break;
	case RightBottomOrientation:	value = "rightbottom";	break;
	case LeftBottomOrientation:	value = "leftbottom";	break;
	}
	if ( value != NULL )
	{
		/* exif:Orientation (but as a number, not a string) */
		addKeyVal( list, PROPERTY_IMAGE_ORIENTATION, value );
	}

	if ( file->image->rows > 0 )
	{
		/* exif:ExifImageLength */
		sprintf( buffer, "%D", file->image->rows );
		addKeyVal( list, PROPERTY_IMAGE_ROWS, buffer );
	}

	if ( file->image->columns > 0 )
	{
		/* exif:ExifImageWidth */
		sprintf( buffer, "%D", file->image->columns );
		addKeyVal( list, PROPERTY_IMAGE_COLUMNS, buffer );
	}

	return 0;
}
