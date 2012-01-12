
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

#ifndef _H5STRING_H
#define _H5STRING_H

#include <stddef.h>

/* structure to print data value into string */
typedef struct h5str_t {
    char        *s;
    size_t      max;  /* the allocated size of the string */
} h5str_t;

/* these functions are defined to deal with printing value to string */
void  h5str_new (h5str_t *str, size_t len);
void  h5str_free (h5str_t *str);
void  h5str_empty (h5str_t *str);
void  h5str_resize (h5str_t *str, size_t new_len);
char* h5str_append (h5str_t *str, const char* cstr);
int   h5str_sprintf(h5str_t *str, int tid, void *buf, const char* compound_delim);

#endif

