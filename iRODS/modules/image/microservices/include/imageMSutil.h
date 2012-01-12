/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * @file	imageMSutil.h
 *
 * @brief	Utility functions for imageMS.c.
 */

#ifndef IMAGEMSUTIL_H
#define IMAGEMSUTIL_H


#include "fileLseek.h"



/**
 * Store information about an image file parameter.  The information
 * includes a parameter type indicating a file path, file descriptor,
 * or file object type.  Each of these has an associated field containing
 * the parameter value.
 */
typedef struct ImageFileParameter
{
	int type;			/* The parameter type */
#define IMAGEFILEPARAMETER_PATH		0
#define IMAGEFILEPARAMETER_FD		1
#define IMAGEFILEPARAMETER_OBJECT	2

	/* From the input */
	char* path;			/* The file path */
	int fd;				/* The file descriptor */
	dataObjInp_t *dataObject;	/* The file object */

	/* From additional parameters */
	keyValPair_t* properties;	/* The file properties */

	/* The image, if appropriate. */
	Image* image;			/* The image */
} ImageFileParameter_t;





int _ImageGetFileParameter( rsComm_t* rsComm, char* messageBase, msParam_t* param, ImageFileParameter_t* result );
int _ImageGetPropertyListParameter( rsComm_t* rsComm, char* messageBase, msParam_t* param, keyValPair_t** result );
int _ImageReadFile( rsComm_t* rsComm, char* messageBase, ImageFileParameter_t* file );
int _ImageWriteFile( rsComm_t* rsComm, char* messageBase, ImageFileParameter_t* file );
char* _ImageGuessFormat( ImageFileParameter_t* file );
int _ImageGetProperties( ImageFileParameter_t* file, keyValPair_t* list );
int _ImageGetDouble( rsComm_t* rsComm, char* messageBase, msParam_t* param, double* result );



int rsDataObjOpen (rsComm_t *rsComm, dataObjInp_t *dataObjInp);
int rsDataObjLseek (rsComm_t *rsComm, openedDataObjInp_t *dataObjLseekInp, fileLseekOut_t **dataObjLseekOut);
int rsDataObjClose (rsComm_t *rsComm, openedDataObjInp_t *dataObjCloseInp);
int rsDataObjWrite (rsComm_t *rsComm, openedDataObjInp_t *dataObjWriteInp, bytesBuf_t *dataObjWriteInpBBuf);
int rsDataObjRead (rsComm_t *rsComm, openedDataObjInp_t *dataObjReadInp, bytesBuf_t *dataObjReadOutBBuf);



#endif	/* IMAGEMSUTIL_H */
