
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

#include "h5File.h"
#include "h5Group.h"
#include "hdf5.h"
#include <assert.h>



/*------------------------------------------------------------------------------
 * Purpose: Creates a new group passed from the client.
 * Parameters: H5Group* g -- The group to be created.
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
/* TODO at H5Group --  this version only implements read-only functions */ 
int H5Group_create(H5Group* g)
{
    int ret_value = 0;

    THROW_UNSUPPORTED_OPERATION(g->error, 
        "H5Group_create(H5Group* g) is not implemented",
        ret_value, done);

done:
    return ret_value;
}



/*------------------------------------------------------------------------------
 * Purpose: Delete the given group from file 
 * Parameters: H5Group* g -- The group to be deleted.
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
/* TODO at H5Group --  this version only implements read-only functions */ 
int H5Group_delete(H5Group* g)
{
    int ret_value = 0;

    THROW_UNSUPPORTED_OPERATION(g->error, 
        "H5Group_delete(H5Group* g) is not implemented",
        ret_value, done);

done:
    return ret_value;

}

int H5Group_read_attribute(H5Group* ing, H5Group* outg)
{
    int ret_value=0, i=0, n=0;
    hid_t gid=-1;

    if ( (gid = H5Gopen(ing->fid, ing->fullpath)) < 0)
        THROW_H5LIBRARY_ERROR(ing->error,ret_value, done);

    if ( (n = H5Aget_num_attrs(gid)) <=0 )
        goto done;

    ing->nattributes = n;
    ing->attributes = (H5Attribute *)malloc(n*sizeof(H5Attribute));
    for (i=0; i<n; i++) {
        H5Attribute_ctor(&(ing->attributes[i]));
        ing->attributes[i].fid = ing->fid;
        ing->attributes[i].obj_path = (char *)malloc(strlen(ing->fullpath)+1);
        strcpy(ing->attributes[i].obj_path, ing->fullpath);
        ing->attributes[i].obj_type = H5OBJECT_GROUP;
    }

    /* read the attribute value into memory */
    H5File_read_attribute(gid, ing->attributes);

done:
    if (gid > 0 ) H5Gclose(gid);

#ifndef HDF5_LOCAL
    memset (outg, 0, sizeof (H5Group));


    /* pass on the value */
    outg->error = ing->error;
    outg->nattributes = ing->nattributes;
    outg->attributes = ing->attributes;

    ing->attributes = NULL;
    ing->nattributes = 0;
#endif

    return ret_value;
}

