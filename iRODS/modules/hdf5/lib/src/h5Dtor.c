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

#include "H5Ipublic.h"
#include "h5Attribute.h"
#include "h5Dataset.h"
#include "h5File.h"
#include "h5Group.h"
#include <assert.h>
#include <stdio.h>

/* Reclaim the memory space set by malloc during the server operation */

void H5Attribute_dtor(H5Attribute* a)
{
    assert(a);

    if (a->name)
    {
        free (a->name);
        a->name = NULL;
    }

    if (a->obj_path)
    {
        free (a->obj_path);
        a->obj_path = NULL;
    }

    if (a->value)
        H5Dataset_freeBuffer(a->value, a->space, a->type, a->nvalue);

    H5Datatype_dtor(&(a->type));
}

void H5Dataset_dtor(H5Dataset* d)
{
    int i;

    assert(d);

    if (d->fullpath)
    {
        free (d->fullpath);
        d->fullpath = NULL;
    }

    if (d->value)
        H5Dataset_freeBuffer(d->value, d->space, d->type,  d->nvalue);

    if (d->attributes)
    {
        for (i=0; i<d->nattributes; i++)
            H5Attribute_dtor(&d->attributes[i]);
        free(d->attributes);
        d->attributes = NULL;
    }

    /* clean datatype memory space: H5Datatype does not have destructor */
    H5Datatype_dtor(&(d->type));
}

void H5File_dtor(H5File* f)
{
    assert(f);

    if (f->filename)
    {
        free(f->filename);
        f->filename = NULL;
    }

    if (f->root)
    {
        H5Group_dtor(f->root);
        free (f->root);
    }
}

void H5Group_dtor(H5Group* g)
{
    int i;
    H5Group *member_g;
    H5Dataset *member_d;

    assert(g);

    if (g->fullpath)
    {
        free (g->fullpath);
        g->fullpath = NULL;
    }

    if (g->attributes)
    {
        for (i=0; i<g->nattributes; i++)
            H5Attribute_dtor(&g->attributes[i]);
        free(g->attributes);
        g->attributes = NULL;
    }

    if (g->groups)
    {
        for (i=0; i<g->ngroups; i++)
        {
            member_g = &g->groups[i];
            if (member_g)
            {
                H5Group_dtor(member_g);
                /* free (member_g); */
            }
        }
        free (g->groups);
    }

    if (g->datasets)
    {
        for (i=0; i<g->ndatasets; i++)
        {
            member_d = &g->datasets[i];
            if (member_d)
            {
                H5Dataset_dtor(member_d);
                /* free (member_d); */
            }
        }
        free (g->datasets);
    }
}

void H5Datatype_dtor(H5Datatype* t)
{
    int i=0;

    assert(t);

    if (t->mtypes)
    {
        free (t->mtypes);
        t->mtypes = NULL;
    }

    if (t->mnames)
    {
        for (i=0; i<t->nmembers; i++)
        {
            if (t->mnames[i])
            {
                free (t->mnames[i]);
                t->mnames[i] = NULL;
            }
        }

        free (t->mnames);
        t->mnames = NULL;
    }
}

void H5Dataset_freeBuffer(void *value, H5Dataspace space, H5Datatype type,
int nvalue)
{
    int i=0;
    char ** strs=0;

    if (value)
    {
        if (    ( H5DATATYPE_VLEN == type.tclass )
             || ( H5DATATYPE_COMPOUND == type.tclass )
             || ( H5DATATYPE_STRING == type.tclass )
           )
        {
            strs = (char **)value;
            for (i=0; i<nvalue; i++)
            {
                if (strs[i]) free(strs[i]);
            }
        }

        free (value);
    }
}


