
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

#include "clH5Handler.h"
#include "h5Object.h"
#include "h5File.h"
#include "h5Group.h"
#include "h5Dataset.h"
#include <assert.h>

/*------------------------------------------------------------------------------
 * Purpose: send client request to the server and process the response from server 
 * Parameters:  -- void *obj, The object to request 
 *                 int objID, The ID to identify the type of the object 
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
int h5ObjRequest(rcComm_t *conn, void *obj, int objID)
{
    int ret_value=0;
    H5File *f = 0;
    H5Dataset *d=0;
    H5Group *g=0;

    assert( obj );

    if (H5OBJECT_FILE == objID)
    {

        f = (H5File *)obj;
        if (H5FILE_OP_OPEN == f->opID) 
            ret_value = clH5File_open(conn, f);
        else if (H5FILE_OP_CLOSE == f->opID)
            ret_value = clH5File_close(conn, f);
        /* TODO at h5Handler -- more actions needs to be added here */
    }
    else if (H5OBJECT_DATASET == objID)
    {
        d = (H5Dataset *)obj;
        if (H5DATASET_OP_READ == d->opID)
            ret_value = clH5Dataset_read (conn, d);
        else if (H5DATASET_OP_READ_ATTRIBUTE == d->opID)
	    ret_value = clH5Dataset_read_attribute(conn, d);

        /* TODO at h5Handler -- more actions needs to be added here */
    }
    else if (H5OBJECT_GROUP == objID)
    {
        g = (H5Group *)obj;
        if (H5GROUP_OP_READ_ATTRIBUTE == g->opID)
            ret_value = clH5Group_read_attribute(conn, g);
    }

    return ret_value;
}
