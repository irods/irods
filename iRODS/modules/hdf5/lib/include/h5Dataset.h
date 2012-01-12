
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

#ifndef _H5DATASET_H
#define _H5DATASET_H

/* Define basic structure for H5Dataset */

#include "h5Object.h"
#include "h5Datatype.h"
#include "h5Dataspace.h"
#include "h5Attribute.h"
#include "rodsClient.h"

#define  H5D_IMAGE_FLAG 0x0001
#define  H5D_IMAGE_TRUECOLOR_FLAG 0x0002
#define  H5D_IMAGE_INTERLACE_PIXEL_FLAG 0x0004
#define  H5D_IMAGE_INTERLACE_PLANE_FLAG 0x0008

/* operation id for H5Dataset */
typedef enum H5Dataset_op_t
{
    H5DATASET_OP_ERROR            = -1,
    H5DATASET_OP_CREATE           = 0,
    H5DATASET_OP_DELETE           = 1,
    H5DATASET_OP_READ             = 2,
    H5DATASET_OP_WRITE            = 3,
    H5DATASET_OP_READ_ATTRIBUTE   = 4
} H5Dataset_op_t;

/****************************************************************************
 *              Fields to be transferred between client and server          *
 * H5DATASET_OP_READ                                                        *
 *     to server: { opID, fid, fullpath, type, space }                      *
 *     to client: { value, error }                                          *
 ****************************************************************************/
typedef struct H5Dataset
{
    int opID;                  /* the id of operation to take on the server */
    int fid;                   /* the file ID from H5Fopen() */
#if 0   /* unsigned long is 64 bit in 64 bit arch */
    unsigned long objID[2];    /* uniquely identify an object in a file [from server] */
#endif
    int objID[2];    /* uniquely identify an object in a file [from server] */
    H5Datatype_class_t tclass;  /* same as class in H5Datatype. Put here for packing */
    int nattributes;           /* number of attributes */
    char* fullpath;            /* the path + name, e.g. /hdfeos/swaths/data fields/channel */
    H5Attribute *attributes;  /* array of attributes */
    H5Datatype type;
    H5Dataspace space;
    unsigned int nvalue;       /* the number of bytes in the buffer *value */
#if 0   /* unsigned long is 64 bit in 64 bit arch */
    long  time;                /* number of seconds used to finish the hdf5 request on server */
#endif
    int  time;                /* number of seconds used to finish the hdf5 request on server */
    void*    value;            /* the data read from file or write to file */
    H5Error error;             /* strings used to carry error report from server operation */
} H5Dataset;

#ifdef __cplusplus
extern "C" {
#endif

/*constructor: always call the constructor of the structure before use it */
void H5Dataset_ctor(H5Dataset* d);

/*destructor: always call the destructor of the structure before use it */
void H5Dataset_dtor(H5Dataset* d);

/* free the current buffer */
void H5Dataset_freeBuffer(void *value, H5Dataspace space, H5Datatype type,
int nvalue);

/* Define operation functions needed and implemented only on the server side */
int H5Dataset_init(H5Dataset *d);
int H5Dataset_create(H5Dataset* d);
int H5Dataset_delete(H5Dataset* d);
int H5Dataset_write(H5Dataset* d);
int H5Dataset_read(H5Dataset* ind, H5Dataset *outd);
int clH5Dataset_read(rcComm_t *conn, H5Dataset* d);
int _clH5Dataset_read (rcComm_t *conn, H5Dataset* d, H5Dataset** outd);
int H5Dataset_read_attribute(H5Dataset* ind, H5Dataset* outd);
int clH5Dataset_read_attribute(rcComm_t *conn, H5Dataset* d);
int _clH5Dataset_read_attribute(rcComm_t *conn, H5Dataset* d, 
H5Dataset** outd);

#ifdef __cplusplus
}
#endif

#endif

