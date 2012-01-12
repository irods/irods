
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

#ifndef _H5GROUP_H
#define _H5GROUP_H


/* defines the basic structure for H5Group object */

#include "H5Ipublic.h"
#include "h5Dataset.h"
#include "h5Object.h"

/* operation id for H5Group */
typedef enum H5Group_op_t
{
    H5GROUP_OP_ERROR         = -1,
    H5GROUP_OP_CREATE        = 0,
    H5GROUP_OP_DELETE        = 1,
    H5GROUP_OP_READ_ATTRIBUTE = 2
} H5Group_op_t;

typedef struct H5Group 
{
    int opID;                   /* the id of operation to take on the server */
    int fid;                    /* the file ID from H5Fopen() */
#if 0	/* unsigned long is 64 bits in 64 bit arch */
    unsigned long objID[2];     /* uniquely object identifer [from server] */ 
#endif
    int objID[2];     /* uniquely object identifer [from server] */ 
    char* fullpath;             /* the path + name, e.g. /hdfeos/swaths */
    struct H5Group *parent;     /* the parent group*/
    int ngroups;               /* number of group members in this group */
    struct H5Group *groups;   /* subgroup members of this group */
    int ndatasets;               /* number of dataset members in this group */
    H5Dataset *datasets;      /* dataset members of this group */
    int nattributes;            /* number of attributes of this group */
    H5Attribute *attributes;   /* attributes of this group */
    H5Error error;              /* strings used to carry error report from server operation */
#if 0	/* unsigned long is 64 bits in 64 bit arch */
    long  time;                /* number of seconds used to finish the hdf5 request on server */
#endif
    int  time;                /* number of seconds used to finish the hdf5 request on server */
} H5Group;

#ifdef __cplusplus
extern "C" {
#endif

/*constructor: always call the constructor of the structure before use it */
void H5Group_ctor(H5Group* g);

/*destructor: always call the destructor of the structure before use it */
void H5Group_dtor(H5Group* g);

/* Define operation functions needed and implemented only on the server side */
int H5Group_create(H5Group* g);
int H5Group_delete(H5Group* g);
int H5Group_read_attribute(H5Group *g, H5Group *outg);
int clH5Group_read_attribute(rcComm_t *conn, H5Group *g);
int _clH5Group_read_attribute(rcComm_t *conn, H5Group* ing, H5Group** outg);

#ifdef __cplusplus
}
#endif

#endif

