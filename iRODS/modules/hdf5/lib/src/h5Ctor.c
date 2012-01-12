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
#include <string.h>
#include <stdio.h>

/* set member variables to 0 to make sure that pointers are not
 * set to some random address by default
 */

void H5Attribute_ctor(H5Attribute* a)
{
    assert(a);
    memset(a, 0, sizeof(H5Attribute));
    H5Datatype_ctor(&(a->type));

    /* add initialization code here */
}

void H5Dataset_ctor(H5Dataset* d)
{
    assert(d);

    memset(d, 0, sizeof(H5Dataset));
    H5Datatype_ctor(&(d->type));

    /* add initialization code here */
}

void H5File_ctor(H5File* f)
{
    assert(f);

    memset(f, 0, sizeof(H5File));

    /* add initialization code here */
}

void H5Group_ctor(H5Group* g)
{
    assert(g);

    memset(g, 0, sizeof(H5Group));

    /* add initialization code here */
}

void H5Datatype_ctor(H5Datatype* t)
{
    assert(t);

    memset(t, 0, sizeof(H5Datatype));

    /* add initialization code here */
}

int get_machine_endian(void)
{
    int order = H5DATATYPE_ORDER_LE;
    long one = 1;

    if ( !(*((char *)(&one))) )
        order = H5DATATYPE_ORDER_BE;

    return order;
}

