
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

#include "h5Attribute.h"
#include "h5Dataset.h"
#include "h5String.h"
#include "hdf5.h"
#include <assert.h>

int H5Attribute_value_to_string(H5Attribute *a, hid_t tid, hid_t sid);

/* initialize the attribute such as name, type, space and value */
int H5Attribute_init(H5Attribute *a, hid_t aid);

/*------------------------------------------------------------------------------
 * Purpose: Read data value of an attribute from file
 * Parameters: H5Attribute* a -- The attribute whose value is read from file
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
int H5Attribute_read(H5Attribute *a)
{
    int ret_value=0, i=0;
    unsigned int npoints;
    hid_t aid=-1, loc_id=-1, tid=-1, ftid=-1, sid=-1;

    assert(a);
    H5Eclear();

    if (a->obj_type == H5OBJECT_DATASET)
        loc_id = H5Dopen(a->fid, a->obj_path);
    else if (a->obj_type == H5OBJECT_GROUP)
        loc_id = H5Gopen(a->fid, a->obj_path);
    else
        THROW_ERROR(a->error, "H5Attribute_read", 
        "Object must be either H5Dataset or H5Group",
        ret_value, done);

	if (loc_id < 0)
        THROW_H5LIBRARY_ERROR(a->error, ret_value, done);

    if ( (aid = H5Aopen_name(loc_id, a->name)) < 0)
        THROW_H5LIBRARY_ERROR(a->error, ret_value, done);

    if ( (ftid = H5Aget_type(aid)) < 0)
        THROW_H5LIBRARY_ERROR(a->error, ret_value, done);

    tid = H5Tget_native_type(ftid, H5T_DIR_ASCEND);
    H5Tclose(ftid);

    /* TODO at H5Attribute -- to support different byte order between the server and client */
    if (a->type.order != get_machine_endian())
        THROW_ERROR(a->error, "H5Attribute_read", 
        "Different byte-order between server and client",
        ret_value, done);

    npoints = 1;
    for (i=0; i<a->space.rank; i++)
        npoints *= a->space.dims[i];
    a->space.npoints = npoints;

    if (a->value)
        H5Dataset_freeBuffer(a->value, a->space, a->type, a->nvalue);

    if (NULL == (a->value = malloc(npoints*a->type.size)) )
        THROW_ERROR(a->error, "H5Attribute_read", 
        "unable to allocate memory to read data from file",
        ret_value, done);

    a->nvalue = npoints*a->type.size;
    if ( H5Aread(aid, tid, a->value) < 0)
    {
        free (a->value);
        a->value = NULL;
        THROW_H5LIBRARY_ERROR(a->error, ret_value, done);
    }

    /* convert compound and variable length data into strings
     * so that the client application is able to display it
     */
    if ( (H5DATATYPE_VLEN == a->type.tclass )
         || (H5DATATYPE_COMPOUND == a->type.tclass)
         || (H5DATATYPE_STRING == a->type.tclass )
       )
    {
        sid = H5Aget_space(aid);
        H5Attribute_value_to_string(a, tid, sid);
        H5Sclose(sid);
    }


done:
    if (tid > 0 ) H5Tclose(tid); 
    if (aid > 0) H5Aclose(aid);

    if (loc_id > 0)
    {
        if (a->obj_type == H5OBJECT_DATASET)
            H5Dclose(loc_id);
        else if (a->obj_type == H5OBJECT_GROUP)
            H5Gclose(loc_id);
    }

    return ret_value;
}

/*------------------------------------------------------------------------------
 * Purpose: Create an attribute of a group or dataset in file
 * Parameters: H5Attribute* a -- The attribute to create
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
/* TODO at H5Attribute -- this version only implements read-only functions */
int H5Attribute_create(H5Attribute *a)
{
    int ret_value = 0;


    THROW_UNSUPPORTED_OPERATION(a->error, 
        "H5Attribute_create(H5Attribute *a) is not implemented",
        ret_value, done);

done:
    return ret_value;
}

/*------------------------------------------------------------------------------
 * Purpose: Delete an attribute from a group or dataset in file
 * Parameters: H5Attribute* a -- The attribute to delete
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
/* TODO at H5Attribute -- this version only implements read-only functions */
int H5Attribute_delete(H5Attribute *a)
{
    int ret_value = 0;


    THROW_UNSUPPORTED_OPERATION(a->error, 
        "H5Attribute_delete(H5Attribute *a) is not implemented",
        ret_value, done);

done:
    return ret_value;
}

/*------------------------------------------------------------------------------
 * Purpose: Write data value of an attribute into file
 * Parameters: H5Attribute* a -- The attibute whose value is written into file
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
/* TODO at H5Attribute -- this version only implements read-only functions */
int H5Attribute_write(H5Attribute *a)
{
    int ret_value = 0;


    THROW_UNSUPPORTED_OPERATION(a->error, 
        "H5Attribute_write(H5Attribute *a) is not implemented",
        ret_value, done);

done:
    return ret_value;
}

/*------------------------------------------------------------------------------
 * Purpose: converts data value into strings
 * Parameters: H5AAttribute -- The attriubte its value to be vonverted.
               hid_t tid -- the datatype identifier
 * Return: Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
int H5Attribute_value_to_string(H5Attribute *a, hid_t tid, hid_t sid)
{
    int ret_value=0;
    unsigned int i=0;
    char **strs;
    unsigned char *vp=NULL;
    h5str_t h5str;
    size_t offset=0, tsize=0, valuelen=0;
    void *value;

    assert(a);
    assert(a->value);
    value = a->value;
    vp = (unsigned char *)a->value;
    a->value = (char **)malloc(a->space.npoints*sizeof(char *));
    assert(a->value);
    strs = (char**)a->value;

    offset = 0;
    tsize = H5Tget_size(tid);
    memset(&h5str, 0, sizeof(h5str_t));
    h5str_new(&h5str, 4*tsize);

    a->nvalue = 0;
    for (i=0; i<a->space.npoints; i++)
    {
        h5str_empty(&h5str);
        ret_value = h5str_sprintf(&h5str, tid, vp + offset, ", ");
        if (ret_value > 0)
        {
            valuelen = strlen(h5str.s)+1;
            strs[i] = (char *)malloc(valuelen);
            strcpy(strs[i], h5str.s);
            /* a->nvalue += valuelen; */
            a->nvalue++;
        }
        offset += tsize;
    }
    h5str_free(&h5str);

    /* reclaim memory allocated to store variable length data */
    if (H5Tdetect_class(tid, H5T_VLEN) > 0)
        H5Dvlen_reclaim(tid, sid, H5P_DEFAULT, value);
    free (value);

    return ret_value;
}


int H5Attribute_init(H5Attribute *a, hid_t aid) {
    int ret_value=0, i=0;
    hsize_t dims[H5S_MAX_RANK];
    hid_t sid=-1, tid=-1, ftid=-1;
    unsigned int npoints;

    if (NULL == a || aid < 0) {
        ret_value = -1;
        goto done;
    }

    if (a->space.rank > 0)
        goto done; /* already called */

    ftid = H5Aget_type(aid);
    tid = H5Tget_native_type(ftid, H5T_DIR_ASCEND);
    if (ftid > 0) H5Tclose(ftid);

    a->name = (char *)malloc(MAX_NAME_LEN);
    H5Aget_name(aid, MAX_NAME_LEN, a->name);

    a->tclass = a->type.tclass = (H5Datatype_class_t)H5Tget_class(tid);
    a->type.size = H5Tget_size(tid);

    if  ( a->type.tclass == H5DATATYPE_INTEGER
         || a->type.tclass == H5DATATYPE_FLOAT
         || a->type.tclass == H5DATATYPE_BITFIELD
         || a->type.tclass == H5DATATYPE_REFERENCE
         || a->type.tclass == H5DATATYPE_ENUM ) {
        a->type.order = (H5Datatype_order_t)H5Tget_order(tid);
        if (a->type.tclass == H5DATATYPE_INTEGER) {
            a->type.sign = (H5Datatype_sign_t)H5Tget_sign(tid);
        }
    }
    else if (a->type.tclass == H5DATATYPE_COMPOUND) {
        a->type.nmembers = H5Tget_nmembers(tid );
        a->type.mnames = (char **)malloc(a->type.nmembers*sizeof(char*));
        a->type.mtypes = (int *)malloc(a->type.nmembers*sizeof(int));
        for (i=0; i<a->type.nmembers; i++) {
            a->type.mnames[i] = H5Tget_member_name(tid, i);
            a->type.mtypes[i] = H5Tget_class(H5Tget_member_type(tid, i));
        }
    }

    sid = H5Aget_space(aid);
    a->space.rank = H5Sget_simple_extent_ndims(sid);
    if ( H5Sget_simple_extent_dims(sid, dims, NULL) < 0 )
        THROW_H5LIBRARY_ERROR(a->error,ret_value, done);

    if (a->space.rank<=0)
    {
        a->space.rank = 1;
        dims[0] = 1;
    }

    npoints = 1;
    for (i=0; i<a->space.rank; i++)
    {
        a->space.dims[i] = dims[i];
        a->space.start[i] = 0;
        a->space.stride[i] = 1;
        a->space.count[i] = dims[i];
        npoints *= dims[i];
    }
    a->space.npoints = npoints;

done:
    if (sid > 0 ) H5Sclose(sid);
    if (tid > 0 ) H5Tclose(tid);

    return ret_value;
}

