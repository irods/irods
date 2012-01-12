
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

#ifndef _H5ATTIBUTE_H
#define _H5ATTIBUTE_H

#define  PALETTE_VALUE "_PALETTE_VALUE"

/* Define basic structure for H5Attribute */

#include "h5Object.h"
#include "h5Datatype.h"
#include "h5Dataspace.h"

/* operation id for H5Attribute */
typedef enum H5Attribute_op_t
{
    H5ATTIBUTE_OP_ERROR         = -1,
    H5ATTIBUTE_OP_CREATE        = 0,
    H5ATTIBUTE_OP_DELETE        = 1,
    H5ATTIBUTE_OP_READ          = 2,
    H5ATTIBUTE_OP_WRITE         = 3
} H5Attribute_op_t;

typedef struct H5Attribute
{
    int opID;               /* the id of operation to take on the server */
    int fid;                /* the file ID from H5Fopen() */
    char* name;             /* the name of the attribute */
    char* obj_path;         /* the full path and name of the object attribute is attached to */
    int obj_type;           /* the type of object attribute is attached to*/
    H5Datatype_class_t tclass;  /* same as class in H5Datatype. Put here for packing */
    H5Datatype type;
    H5Dataspace space;
    unsigned int nvalue;       /* the number of bytes in the buffer *value */
    void*    value;         /* the value of the attribute */
    H5Error error;            /* strings used to carry error report from server operation */
} H5Attribute;

#ifdef __cplusplus
extern "C" {
#endif

/*constructor: always call the constructor of the structure before use it */
void H5Attribute_ctor(H5Attribute *a);

/*destructor: always call the destructor of the structure after use it */
void H5Attribute_dtor(H5Attribute* a);

/* Define operation functions needed and implemented only on the server side */
int H5Attribute_init(H5Attribute *a, hid_t aid);
int H5Attribute_create(H5Attribute* a);
int H5Attribute_delete(H5Attribute* a);
int H5Attribute_write(H5Attribute* a);
int H5Attribute_read(H5Attribute* a);

#ifdef __cplusplus
}
#endif

#endif

