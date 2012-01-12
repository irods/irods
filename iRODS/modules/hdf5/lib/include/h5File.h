
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

#ifndef _H5FILE_H
#define _H5FILE_H

/* defines the basic structure for H5File object */

#include "h5Group.h"
#include "h5Attribute.h"
#include "rodsClient.h"

/* operation id for H5File */
typedef enum H5File_op_t
{
    H5FILE_OP_ERROR     = -1,
    H5FILE_OP_OPEN        = 0,
    H5FILE_OP_CLOSE        = 1,
    H5FILE_OP_CREATE    = 2
} H5File_op_t;

/****************************************************************************
 *              Fields to be transferred between client and server          *
 * H5FILE_OP_OPEN                                                           *
 *     to server: { opID, filename }                                        *
 *     to client: { fid, root, error }                                      *
 * H5FILE_OP_CLOSE                                                          *
 *     to server: { opID, fid }                                             *
 *     to client: { error }                                                 * 
 ****************************************************************************/
typedef struct H5File 
{
    int opID;            /* the id of operation to take on the server */
    char* filename;      /* the name of the remote file */
    int fid;             /* the file ID from H5Fopen() */
    H5Group *root;       /* the root group that holds the whole file structure */
    H5Error error;       /* strings used to carry error report from server operation */
#if 0	/* long is 64 bits in 64 arch */
    long  time;          /* number of seconds used to finish the hdf5 request on server */
#endif
    int  time;          /* number of seconds used to finish the hdf5 request on server */
} H5File;

/* The following structure is used to hold all the information.
 * since each call of H5Gget_objname_by_idx() takes about one second.
 * 1,000,000 calls take 12 days. Instead of calling it in a loop,
 * we use only one call to get all the information, which takes about
 * two seconds
 */
typedef struct info_all_t 
{
    char **objname; 
    int *type;
    int count;
    unsigned long *objno0;
    unsigned long *objno1;
} info_all_t;


#ifdef __cplusplus
extern "C" {
#endif

/*constructor: always call the constructor of the structure before use it */
void H5File_ctor(H5File* f);

/*destructor: always call the destructor of the structure before use it */
void H5File_dtor(H5File* f);

/* define the operation functions needed and implemented only on the server side */
int H5File_open(H5File* inf, H5File *outf);
int H5File_close(H5File* f, H5File *outf);
int H5File_create(H5File* f);
int H5File_read_attribute(hid_t loc_id, H5Attribute *attrs);

/* definition for flag in clH5File_open */
#define LOCAL_H5_OPEN	0x1	/* open the H5File at the server locally */
/* define the client handling functions 
*/
int clH5File_open(rcComm_t *conn, H5File* f);
int _clH5File_open(rcComm_t *conn, H5File* f,  H5File** outf, int flag);
int clH5File_close(rcComm_t *conn, H5File* f);
int _clH5File_close (rcComm_t *conn, H5File* inf, H5File** outf);

#ifdef __cplusplus
}
#endif


#endif

