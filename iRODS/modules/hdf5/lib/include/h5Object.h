
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "H5Epublic.h"

#ifndef _H5OBJECT_H
#define _H5OBJECT_H

#define MAX_ERROR_SIZE 80
#define HDF5_MAX_NAME_LEN   256

/* microservices parameter definition */
#define h5File_MS_T	"h5File_PI"
#define h5error_MS_T	"h5error_PI"
#define h5Group_MS_T	"h5Group_PI"
#define h5Attribute_MS_T	"h5Attribute_PI"
#define h5Dataset_MS_T	"h5Dataset_PI"
#define h5Datatype_MS_T	"h5Datatype_PI"

typedef enum H5Object_t {
    H5OBJECT_UNKNOWN = -1,  
    H5OBJECT_LINK,    
    H5OBJECT_GROUP,  
    H5OBJECT_DATASET,
    H5OBJECT_DATATYPE,
    H5OBJECT_DATASPACE,
    H5OBJECT_ATTRIBUTE,
    H5OBJECT_FILE
} H5Object_t;

typedef struct H5Object
{
    H5Object_t type;
    void *obj;
} H5Object;

typedef struct H5Error
{
    char major[MAX_ERROR_SIZE];
    char minor[MAX_ERROR_SIZE];
} H5Error;

/* THROW_ERROR macro, used to report error */
#define THROW_ERROR(_error, _major, _minor, _ret_val, _exit) { \
    strcpy(_error.major, _major); \
    strcpy(_error.minor, _minor); \
    _ret_val = -1; \
    goto _exit; \
}

/* THROW_H5LIBRARY_ERROR macro, used to report HDF5 library error */
#define THROW_H5LIBRARY_ERROR(_error, _ret_val, _exit) { \
    strcpy(_error.major, getMajorError()); \
    strcpy(_error.minor, getMinorError()); \
    _ret_val = -1; \
    goto _exit; \
}

/* THROW_UNSUPPORTED_OPERATION macro */
#define THROW_UNSUPPORTED_OPERATION(_error, _msg, _ret_val, _exit) { \
    strcpy(_error.major, "The operation is not supported by the server"); \
    strcpy(_error.minor, _msg); \
    _ret_val = -1; \
    goto _exit; \
}


#ifdef __cplusplus
extern "C" {
#endif

/* define the operation functions needed and implemented only on the server side */

const char* getMajorError(void);
const char* getMinorError(void);
H5E_minor_t getMinorErrorNumber(void);
H5E_major_t getMajorErrorNumber(void);

#ifdef __cplusplus
}
#endif

#endif


