
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
#include "h5Dataset.h"
#include "h5Object.h"
#include "h5String.h"
#include "hdf5.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

/* Retrive dataspace and datatype information for this dataset */
int H5Dataset_init(H5Dataset *d);

/* converts data value to strings */
int H5Dataset_value_to_string(H5Dataset *d, hid_t tid, hid_t sid);

void  H5Dataset_readPalette(H5Dataset *d, hid_t did);
void  H5Dataset_check_image(H5Dataset *d, hid_t did);

/*------------------------------------------------------------------------------
 * Purpose: Create a new dataset
 * Parameters: H5Dataset* d-- The dataset to be created
 * Return: Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
/* TODO at H5Dataset -- this version only implements read-only functions */
int H5Dataset_create(H5Dataset* d)
{
    int ret_value = 0;

    THROW_UNSUPPORTED_OPERATION(d->error,
        "H5Dataset_create(H5Dataset* d) is not implemented",
        ret_value, done);

done:
    return ret_value;
}


/*------------------------------------------------------------------------------
 * Purpose: Delete a dataset from file
 * Parameters: H5Dataset* d-- Dataset to be deleted
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
/* TODO at H5Dataset -- this version only implements read-only functions */
int H5Dataset_delete(H5Dataset* d)
{
    int ret_value = 0;


    THROW_UNSUPPORTED_OPERATION(d->error,
        "H5Dataset_delete(H5Dataset* d) is not implemented.",
        ret_value, done);

done:
    return ret_value;
}


/*------------------------------------------------------------------------------
 * Purpose: Read data from file 
 * Parameters: H5Dataset* d -- The dataset whose value to be retrieved from file
 *     to server: { opID, fid, fullpath, type, space }                      *
 *     to client: { value, error }                                          *
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
int H5Dataset_read(H5Dataset* ind, H5Dataset* outd)
{
    int ret_value=0, i=0;
    hid_t did=-1, sid=-1, tid=-1, msid=-1, ftid=-1;
    unsigned int npoints=1;
    hsize_t start[H5S_MAX_RANK], stride[H5S_MAX_RANK], count[H5S_MAX_RANK], mdims[1];
    time_t t0=0, t1=0;
    H5Eclear();

    if (ind->space.rank <=0 )
        H5Dataset_init(ind);

    t0 = time(NULL);

    if ( (did = H5Dopen(ind->fid, ind->fullpath)) < 0)
        THROW_H5LIBRARY_ERROR(ind->error,ret_value, done);

    /* TODO at H5Dataset -- we need to convert the datatype to the datatype which is passed 
     *  from client machine so that the data reading from file will be right for 
     *  the client. The byte order of the client machine, server machine, and data
     *  in the file can be all different.
     * 
     *  For the first version, we assume the server machine and client machine
     *  have the same byte order.
     */ 
    ftid = H5Dget_type(did);
    tid = H5Tget_native_type(ftid, H5T_DIR_ASCEND);
    H5Tclose(ftid);

    /* TODO at H5Dataset -- to support different byte order between the server and client */
    if (ind->type.order != get_machine_endian())
        THROW_ERROR(ind->error,
            "H5Dataset_read", "The byte order is different between the server and client",
            ret_value, done);


    sid = msid = H5S_ALL;
    npoints = 1;
    for (i=0; i<ind->space.rank; i++)
        npoints *= ind->space.count[i];
    ind->space.npoints = npoints;
    mdims[0] = npoints;
    
    /* cannot select hyperslab for scalar data point or 
       1D array with a single data point 
     */
    if ( (ind->space.rank>0) && (ind->space.rank * ind->space.dims[0]>1) )
    {
        sid = H5Dget_space(did);
        for (i=0; i<ind->space.rank; i++)
        {
            start[i] = ind->space.start[i];
            stride[i] = ind->space.stride[i];
            count[i] = ind->space.count[i];
        }
        if (H5Sselect_hyperslab(sid, H5S_SELECT_SET, start, stride, count, NULL)<0)
            THROW_H5LIBRARY_ERROR(ind->error,ret_value, done);

        msid = H5Screate_simple(1, mdims, NULL);
    }

    if (ind->value)
        H5Dataset_freeBuffer(ind->value, ind->space, ind->type, ind->nvalue);

    if (NULL == (ind->value = malloc(npoints*ind->type.size)) )
        THROW_ERROR(ind->error, "H5Dataset_read",
            "unable to allocate memory to read data from file",
            ret_value, done);
   
    ind->nvalue = npoints*ind->type.size; 
    ind->tclass = ind->type.tclass; 
    if ( H5Dread(did, tid, msid, sid, H5P_DEFAULT, ind->value) < 0)
    {
        free (ind->value);
        ind->value = NULL;
        THROW_H5LIBRARY_ERROR(ind->error,ret_value, done);
    } 
  
    /* convert compound and variable length data into strings
     * so that the client application is able to display it
     */ 
    if ( (H5DATATYPE_VLEN == ind->type.tclass )
         || (H5DATATYPE_COMPOUND == ind->type.tclass)
         || (H5DATATYPE_STRING == ind->type.tclass )
       )
    {
        H5Dataset_value_to_string(ind, tid, msid);
    }

done:
    if (msid > 0) H5Sclose(msid);
    if (sid > 0 ) H5Sclose(sid);
    if (tid > 0 ) H5Tclose(tid);
    if (did > 0 ) H5Dclose(did);

    t1 = time(NULL);

#ifndef HDF5_LOCAL
    memset (outd, 0, sizeof (H5Dataset));

    /* pass on the value */
    outd->nvalue = ind->nvalue;
    outd->value = ind->value;
    outd->tclass = ind->tclass;
    outd->error = ind->error;
    outd->time = (long)(t1-t0);

    ind->value = NULL;
    ind->nvalue = 0;
#endif

    return ret_value;
}


/*------------------------------------------------------------------------------
 * Purpose: Write data value into file 
 * Parameters: H5Dataset* d -- The dataset whose value is to be written into file
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
/* TODO at H5Dataset -- this version only implements read-only functions */
int H5Dataset_write(H5Dataset* d)
{
    int ret_value = 0;


    THROW_UNSUPPORTED_OPERATION(d->error,
        "H5Dataset_write(H5Dataset* d) is not implemented",
        ret_value, done);

done:
    return ret_value;
}

/* Purpose: retrieve datatype and dataspace information from file 
 * Parameters: H5Dataset *d -- The dataset to be initialized 
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 */
int H5Dataset_init(H5Dataset *d)
{
    int ret_value=0, i=0;
    hsize_t dims[H5S_MAX_RANK];
    hid_t did=-1, sid=-1, tid=-1, ftid=-1;
    unsigned int npoints;

    assert (d);
    H5Eclear();

    if (d->space.rank > 0)
        goto done; /* already called */

    if ( (did = H5Dopen(d->fid, d->fullpath)) < 0)
        THROW_H5LIBRARY_ERROR(d->error,ret_value, done);

    ftid = H5Dget_type(did);
    tid = H5Tget_native_type(ftid, H5T_DIR_ASCEND);
    if ( ftid > 0) H5Tclose(ftid);

    d->tclass = d->type.tclass = (H5Datatype_class_t)H5Tget_class(tid);
    d->type.size = H5Tget_size(tid);

    if  ( d->type.tclass == H5DATATYPE_INTEGER 
         || d->type.tclass == H5DATATYPE_FLOAT 
         || d->type.tclass == H5DATATYPE_BITFIELD 
         || d->type.tclass == H5DATATYPE_REFERENCE 
         || d->type.tclass == H5DATATYPE_ENUM ) 
    {
        d->type.order = (H5Datatype_order_t)H5Tget_order(tid);

        if (d->type.tclass == H5DATATYPE_INTEGER)
        {
            d->type.sign = (H5Datatype_sign_t)H5Tget_sign(tid);

            /* check palette for image */
            H5Dataset_readPalette(d, did);

            H5Dataset_check_image(d, did);
        }
    }
    else if (d->type.tclass == H5DATATYPE_COMPOUND) {
        d->type.nmembers = H5Tget_nmembers(tid );
        d->type.mnames = (char **)malloc(d->type.nmembers*sizeof(char*));
        d->type.mtypes = (int *)malloc(d->type.nmembers*sizeof(int));
        for (i=0; i<d->type.nmembers; i++) {
            hid_t mtid = -1;
            int mtype = 0, mclass, msign, msize;
            d->type.mnames[i] = H5Tget_member_name(tid, i);
            mtid = H5Tget_member_type(tid, i); 
            mclass = H5Tget_class(mtid);
            msign = H5Tget_sign(mtid);
            msize = H5Tget_size(mtid);
            mtype = mclass<<28 | msign<<24 | msize;
            d->type.mtypes[i] = mtype;
            H5Tclose(mtid);
        }
    }

    sid = H5Dget_space(did);
    d->space.rank = H5Sget_simple_extent_ndims(sid);
    if ( H5Sget_simple_extent_dims(sid, dims, NULL) < 0 )
        THROW_H5LIBRARY_ERROR(d->error,ret_value, done);

    if (d->space.rank<=0)
    {
        d->space.rank = 1;
        dims[0] = 1;
    }

    npoints = 1; 
    for (i=0; i<d->space.rank; i++)
    {
        d->space.dims[i] = dims[i];
        d->space.start[i] = 0;
        d->space.stride[i] = 1;
        d->space.count[i] = dims[i];
        npoints *= dims[i];
    }
    d->space.npoints = npoints;

done:
    if (sid > 0 ) H5Sclose(sid);
    if (tid > 0 ) H5Tclose(tid);
    if (did > 0 ) H5Dclose(did);
    return ret_value;
}

/*------------------------------------------------------------------------------
 * Purpose: converts data value into strings
 * Parameters: H5Dataset* d-- The dataset its value to be vonverted.
               hid_t tid -- the datatype identifier
 * Return: Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
int H5Dataset_value_to_string(H5Dataset *d, hid_t tid, hid_t sid)
{
    int ret_value=0;
    unsigned int i=0;
    char **strs;
    unsigned char *vp=NULL;
    h5str_t h5str;
    size_t offset=0, tsize=0, valuelen=0;
    void *value;

    assert(d);
    assert(d->value);
    value = d->value;
    vp = (unsigned char *)d->value;
    d->value = (char **)malloc(d->space.npoints*sizeof(char *));
    assert(d->value);
    strs = (char**)d->value;

    offset = 0;
    tsize = H5Tget_size(tid);
    memset(&h5str, 0, sizeof(h5str_t));
    h5str_new(&h5str, 4*tsize);

    d->nvalue = 0;
    for (i=0; i<d->space.npoints; i++)
    {
        h5str_empty(&h5str);
        ret_value = h5str_sprintf(&h5str, tid, vp + offset, " || ");
        if (ret_value > 0)
        {
            valuelen = strlen(h5str.s)+1;
            strs[i] = (char *)malloc(valuelen);
            strcpy(strs[i], h5str.s);
            /* d->nvalue += valuelen; XXXX changed by MW */
            d->nvalue ++;
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

/* read attribute value */
char* H5Dataset_read_attr_value(hid_t aid)
{
    hid_t asid=-1, atid=-1;
    int i, rank;
    hsize_t *dims;
    size_t size=1;
    char *attr_buf=NULL;
  
    asid = H5Aget_space(aid);
    atid = H5Aget_type(aid);
    rank = H5Sget_simple_extent_ndims(asid);

    if (rank > 0) {
        dims = (hsize_t *)malloc(rank * sizeof(hsize_t));
        H5Sget_simple_extent_dims(asid, dims, NULL);
        for (i=0; i<rank; i++) {
            size *= (size_t)dims[i];
            free(dims);
        }
        size *= H5Tget_size(atid);
        attr_buf = (char *)malloc(size);

        if (H5Aread( aid, atid, attr_buf) < 0) {
            free(attr_buf);
            attr_buf = NULL;
        }

    }
    
    if( atid > 0) H5Tclose(atid);
    if (asid > 0) H5Sclose(asid);
 
    return attr_buf;
}
/* 
 * check if the dataset is an image
*/
void H5Dataset_check_image(H5Dataset *d, hid_t did)
{
    hid_t aid=-1;
    char *buf;

    if (!d)
        return;

    aid = H5Aopen_name(did, "CLASS");
    if (aid > 0) {
        buf = (char *) H5Dataset_read_attr_value(aid);
        if (buf) {
            if(strncmp(buf, "IMAGE", 5)==0)
                d->time |= H5D_IMAGE_FLAG;
            free(buf);
        }
        H5Aclose(aid);
    }

    aid = H5Aopen_name(did, "IMAGE_SUBCLASS");
    if (aid > 0) {
        buf = (char *) H5Dataset_read_attr_value(aid);
        if (buf) {
            if(strncmp(buf, "IMAGE_TRUECOLOR", 15)==0)
                d->time |= H5D_IMAGE_TRUECOLOR_FLAG;
            free(buf);
        }
        H5Aclose(aid);
    }

    aid = H5Aopen_name(did, "INTERLACE_MODE");
    if (aid > 0) {
        buf = (char *) H5Dataset_read_attr_value(aid);
        if (buf) {
            if(strncmp(buf, "INTERLACE_PIXEL", 15)==0)
                d->time |= H5D_IMAGE_INTERLACE_PIXEL_FLAG;
            else if(strncmp(buf, "INTERLACE_PLANE", 15)==0)
                d->time |= H5D_IMAGE_INTERLACE_PLANE_FLAG;
            free(buf);
        }
        H5Aclose(aid);
    }
}

/* reads the references of palettes into an array
 * Each reference requires  eight bytes storage. Therefore, the array length
 * is 8*numberOfPalettes.
*/
hobj_ref_t* H5Dataset_get_paletteRef(hid_t did)
{
    hid_t aid=-1;
    hobj_ref_t *ref_buf=NULL;
 
    aid = H5Aopen_name(did, "PALETTE");

    if (aid > 0) {

        ref_buf = (hobj_ref_t *) H5Dataset_read_attr_value(aid);
        H5Aclose(aid);
    }
 
    return ref_buf;
}

void  H5Dataset_readPalette(H5Dataset *d, hid_t did)
{
    hid_t pal_id=-1, tid=-1;
    hobj_ref_t *refs;

    if (!d || did<=0 ) return;

    refs = H5Dataset_get_paletteRef(did);
    if (refs) {
        // use the fist palette
        pal_id =  H5Rdereference(d->fid, H5R_OBJECT, refs);
        tid = H5Dget_type(pal_id);
        if (H5Dget_storage_size(pal_id) <= 768) 
        {
            H5Attribute *attr;
            d->nattributes = 1;
            d->attributes = (H5Attribute*)malloc(sizeof(H5Attribute));
            attr = &(d->attributes[0]);
            H5Attribute_ctor(attr);
            attr->value = (unsigned char *)malloc(3*256); 
            memset(attr->value, 0, 768);
            attr->nvalue = 768;
            attr->name = (char*)malloc(20);
            strcpy(attr->name, PALETTE_VALUE);
            H5Dread( pal_id, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, attr->value);
        }

        if (tid > 0) H5Tclose(tid);
        if (pal_id > 0) H5Dclose(pal_id);
        free(refs);
    }

}

int H5Dataset_read_attribute(H5Dataset* ind, H5Dataset* outd)
{
    int ret_value=0, i=0, n=0;
    hid_t did=-1;

    if ( (did = H5Dopen(ind->fid, ind->fullpath)) < 0)
        THROW_H5LIBRARY_ERROR(ind->error,ret_value, done);

    if ( (n = H5Aget_num_attrs(did)) <=0 )
        goto done;

    ind->nattributes = n;
    ind->attributes = (H5Attribute *)malloc(n*sizeof(H5Attribute));
    for (i=0; i<n; i++) {
        H5Attribute_ctor(&(ind->attributes[i]));
        ind->attributes[i].fid = ind->fid;
        ind->attributes[i].obj_path = (char *)malloc(strlen(ind->fullpath)+1);
        strcpy(ind->attributes[i].obj_path, ind->fullpath);
        ind->attributes[i].obj_type = H5OBJECT_DATASET;
    }
    
    /* read the attribute value into memory */
    H5File_read_attribute(did, ind->attributes);

done:
    if (did > 0 ) H5Dclose(did);

#ifndef HDF5_LOCAL
    memset (outd, 0, sizeof (H5Dataset));


    /* pass on the value */
    outd->tclass = ind->tclass;
    outd->error = ind->error;
    outd->nattributes = ind->nattributes;
    outd->attributes = ind->attributes;

    ind->attributes = NULL;
    ind->nattributes = 0;
#endif

    return ret_value;
}

