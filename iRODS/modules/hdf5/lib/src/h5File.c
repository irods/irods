
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
#include "hdf5.h"
#include "h5Dataset.h"
#include <assert.h>

/* retrieve the file structure and store it in the root group */
int H5File_get_structure(H5File* f);

/* recursively retrieve the hierarchical structure of the file */
int H5File_depth_first(H5Group* g);

/* retrieve attributes for a given object id */
int H5File_read_attribute(hid_t loc_id, H5Attribute *attrs);

/* The following two functions are used to retrieve all the information once.
 * since each call of H5Gget_objname_by_idx() takes about one second.
 * 1,000,000 calls take 12 days. Instead of calling it in a loop,
 * we use only one call to get all the information, which takes about
 * two seconds
 */
herr_t obj_info_all(hid_t loc_id, const char *name, void *opdata);
herr_t H5Gget_obj_info_all( hid_t loc_id, const char *group_name,  
    char **objname, int *type, unsigned long *objno0, unsigned long *objno1 );
int H5File_has_loop(const H5Group *g, const unsigned long ids[]);

/*------------------------------------------------------------------------------
 * Purpose: Open an existing file and retrieve the file structure (root group)
 * Parameters: H5File* f -- The file to open 
 *     to server: { opID, filename }                                        *
 *     to client: { fid, root, error }                                      *
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */

int H5File_open(H5File* inf, H5File *outf)
{
    int ret_value = 0;
    hid_t fid=-1;

    H5Eclear();

    /* TODO at H5File_open: read-only file access for this version */
    if ( (fid = H5Fopen(inf->filename, H5F_ACC_RDONLY, H5P_DEFAULT)) < 0)
        THROW_H5LIBRARY_ERROR(inf->error,ret_value, done);

    inf->fid = fid;

    H5File_get_structure(inf);

#ifndef HDF5_LOCAL
    memset (outf, 0, sizeof (H5File));

    /* Copy the ones we are sending. Will do something different later  */

    outf->fid = inf->fid;
    outf->root = inf->root;
    inf->root = NULL;
    outf->error = inf->error;
#endif

done:
    if (ret_value >= 0)
	return inf->fid;
    else
        return ret_value;
}

/*------------------------------------------------------------------------------
 * Purpose: Close a file and release all its resources
 * Parameters: H5File* f -- The file to close 
 *     to server: { opID, fid }                                             *
 *     to client: { error }                                                 *
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
int H5File_close(H5File* f, H5File*outf)
{
    int ret_value=0, i=0, n;
    hid_t fid=-1, *pids;

    assert( f && (f->fid>0) );
    H5Eclear();
    pids = NULL;

    /* close all open datasets */
    fid = (hid_t)f->fid;
    n = H5Fget_obj_count(fid, H5F_OBJ_DATASET);
    if (n > 0) {
        pids = (hid_t *) malloc(sizeof(hid_t)*n);
        assert(pids);
        H5Fget_obj_ids(fid, H5F_OBJ_DATASET, n, pids);
        for (i=0; i<n; i++) 
            H5Dclose(pids[i]);
         free(pids);
    }

    /* close all open groups */
    n = H5Fget_obj_count(fid, H5F_OBJ_GROUP );
    if (n > 0) {
        pids = (hid_t *) malloc(sizeof(hid_t)*n);
        H5Fget_obj_ids(fid, H5F_OBJ_GROUP , n, pids);
        for (i=0; i<n; i++) 
            H5Gclose(pids[i]); 
        free(pids);
    }

    H5Fflush(fid, H5F_SCOPE_GLOBAL);
    
    if ( H5Fclose(fid) < 0)
        THROW_H5LIBRARY_ERROR(f->error,ret_value, done);

done:
#ifndef HDF5_LOCAL
    memset (outf, 0, sizeof (H5File));
    outf->error = f->error;
#endif
    return ret_value;
}

/*------------------------------------------------------------------------------
 * Purpose: Create a new file  
 * Parameters: H5File* f -- The file to create 
 * Return:  Returns a non-negative value if successful; otherwise returns a negative value.
 *------------------------------------------------------------------------------
 */
/* TODO at H5File -- this version only implements read-only functions */
int H5File_create(H5File* f)
{
    int ret_value = 0;

    THROW_UNSUPPORTED_OPERATION(f->error,
        "H5File_create(H5File *f) is not implemented",
        ret_value, done);


done:
    return ret_value;
}


/* retrieve the file structure and store it in the root group */
int H5File_get_structure(H5File* f)
{
    int ret_value=0;

    assert(f);

    f->root = (H5Group *) malloc(sizeof (H5Group));
    assert (f->root);

    H5Group_ctor(f->root);
  
    f->root->fid = f->fid;
    f->root->fullpath = (char *)malloc(2);
    strcpy(f->root->fullpath, "/");
    f->root->objID[0] = f->root->objID[1] = 0;
    f->root->parent = NULL;

    ret_value = H5File_depth_first(f->root);

    return ret_value;
}

/* recursively retrieve the hierarchical structure of the file */
int H5File_depth_first(H5Group* parent)
{
    int ret_value=0, obj_type=-1, *mtypes=0;
    hsize_t nmembers=0, ngroups=0, ndatasets=0, i=0, gidx=0, didx=0;
    unsigned long *objno0=0, *objno1=0;
    size_t len=0;
    hid_t gid=-1, fid=-1;
    char **mnames=0;
    H5Group *g=0;
    H5Dataset *d=0;

    assert (parent);

    fid = (hid_t)parent->fid;
    if ( (gid=H5Gopen(fid, parent->fullpath)) < 0)
       THROW_H5LIBRARY_ERROR(parent->error,ret_value, done);

    if (H5Gget_num_objs(gid, &nmembers) < 0)
        THROW_H5LIBRARY_ERROR(parent->error,ret_value, done);

    if (nmembers <=0)
        goto done; 

    mtypes = (int *)malloc((int)nmembers*sizeof(int));
    mnames = (char **)malloc((int)nmembers*sizeof(char*));
    objno0 = (unsigned long *) malloc((int)nmembers*sizeof(unsigned long));
    objno1 = (unsigned long *) malloc((int)nmembers*sizeof(unsigned long));

    if ( H5Gget_obj_info_all
        (fid, parent->fullpath, mnames, mtypes, objno0, objno1) < 0 
       )
        THROW_H5LIBRARY_ERROR(parent->error,ret_value, done);

    ngroups = ndatasets = 0;
    for (i=0; i<nmembers; i++)
    {
        obj_type = mtypes[i];
        if (obj_type == H5G_GROUP)
            ngroups++;
        else if (obj_type == H5G_DATASET)
            ndatasets++; 
    }

    parent->ngroups = ngroups;
    parent->ndatasets = ndatasets;

    if (ngroups>0)
        parent->groups = (H5Group *)malloc((int)ngroups*sizeof(H5Group));

    if (ndatasets>0)
        parent->datasets = (H5Dataset *)malloc((int)ndatasets*sizeof(H5Dataset));

    gidx=didx=0;
    for (i=0; i<nmembers; i++)
    {
        obj_type = mtypes[i];
        len = strlen(mnames[i])+strlen(parent->fullpath)+2;
        if (obj_type == H5G_GROUP)
        {
            g = (H5Group*)&parent->groups[gidx];
            H5Group_ctor(g);
            g->fid = fid;
            g->fullpath = (char*)malloc(len);
            strcpy(g->fullpath, parent->fullpath);
            if (strlen(parent->fullpath)>1) strcat(g->fullpath, "/");
            strcat(g->fullpath, mnames[i]);
            g->objID[0] = objno0[i];
            g->objID[1] = objno1[i];
            g->parent = parent;
            /* break the loop */
            if (!H5File_has_loop(g, (const long unsigned int*)g->objID))
                ret_value = H5File_depth_first( g );
            gidx++;
        } else if (obj_type == H5G_DATASET)
        {
            d = (H5Dataset*)&parent->datasets[didx];
            H5Dataset_ctor(d);
            d->fid = fid;
            d->fullpath = (char*)malloc(len*2);
            strcpy(d->fullpath, parent->fullpath);
            if (strlen(parent->fullpath)>1) strcat(d->fullpath, "/");
            strcat(d->fullpath, mnames[i]);
            d->objID[0] = objno0[i];
            d->objID[1] = objno1[i];
            H5Dataset_init(d);
            didx++;
        } 
    }    

done:
    if (mnames)
    {
        for (i=0; i<nmembers; i++)
        {
            if (mnames[i]) free(mnames[i]);
        }
        free (mnames);
    }
    if (mtypes) free (mtypes);
    if (objno0) free (objno0);
    if (objno1) free (objno1);
    if (gid > 0) H5Gclose(gid);
    return ret_value;
}

herr_t obj_info_all(hid_t loc_id, const char *name, void *opdata)
{
    H5G_stat_t statbuf;
    info_all_t* info = (info_all_t*)opdata;

    if (H5Gget_objinfo(loc_id, name, 0, &statbuf) < 0)
    {
        *(info->type+info->count) = -1;
        *(info->objname+info->count) = NULL;
        *(info->objno0+info->count) = 0;
        *(info->objno1+info->count) = 0;
    } else {
        *(info->type+info->count) = statbuf.type;
        *(info->objname+info->count) = (char *)strdup(name);
        *(info->objno0+info->count) = statbuf.objno[0];
        *(info->objno1+info->count) = statbuf.objno[1];
    }
    info->count++;

    return 0;
}

herr_t H5Gget_obj_info_all( hid_t loc_id, const char *group_name, char **objname,
int *type, unsigned long *objno0, unsigned long *objno1)
{
    info_all_t info;
    info.objname = objname; 
    info.type = type;
    info.count = 0;
    info.objno0 = objno0;
    info.objno1 = objno1;

    if(H5Giterate(loc_id, group_name, 0, obj_info_all, (void *)&info)<0)
        return -1;

    return 0;
}

/* Check if this group forms a loop.
   because HDF5 objects are uniuniquely identifed by object ids, a loop can be
   detected by trace back to its parent group recursively if there is a group
   with the same objet ids.  */
int H5File_has_loop(const H5Group *g, const unsigned long ids[])
{
    int ret_value=0;
    H5Group *parent;

    assert(g);

    if ( (parent  = g->parent) )
    {
        if ( (parent->objID[0] == ids[0]) && (parent->objID[1] == ids[1]) )
            ret_value = 1;
        else
            ret_value = H5File_has_loop(parent, ids);
    } 

    return ret_value;
}

int H5File_read_attribute(hid_t loc_id, H5Attribute *attrs)
{
    int ret_value=0, n, i, rank;
    hid_t aid=-1;
    H5Attribute *a;
    
    if (NULL == attrs || loc_id<0) {
	ret_value = -1;
	goto done;
    }

    n = H5Aget_num_attrs(loc_id);
    for (i=0; i<n; i++) {
        a = &(attrs[i]);
        aid = H5Aopen_idx(loc_id, i);
        H5Attribute_init(a, aid);
        H5Attribute_read(a);
        H5Aclose(aid);
    }

done:
    return ret_value;
}
