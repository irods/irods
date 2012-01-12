
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

/* Define functions to retrive error information from error stack */

#include "hdf5.h"
#include "h5Object.h"


#define H5E_NSLOTS  32  /*number of slots in an error stack      */

/* An error stack */
typedef struct H5E_t {
    int nused;                      /*num slots currently used in stack  */
    H5E_error_t slot[H5E_NSLOTS];   /*array of error records         */
    H5E_auto_t auto_func;           /* Function for 'automatic' error reporting */
    void *auto_data;                /* Callback data for 'automatic' error reporting */
} H5E_t;

H5E_t  H5E_stack_g[1];
#define H5E_get_my_stack()	(H5E_stack_g+0)


#ifdef __cplusplus
extern "C" {
#endif


/* Get the description of the current major error */
const char* getMajorError()
{
    return H5Eget_major(getMajorErrorNumber());
}

/* Get the description of the current minor error */
const char* getMinorError()
{
    return H5Eget_minor(getMinorErrorNumber());
}

H5E_major_t getMajorErrorNumber()
{
    H5E_t	*estack = H5E_get_my_stack ();
    H5E_error_t *err_desc;
    H5E_major_t maj_num = H5E_NONE_MAJOR;
    if (estack && estack->nused>0)
    {
        err_desc = estack->slot+0;
        maj_num = err_desc->maj_num;
    }
    return maj_num;
}

H5E_minor_t getMinorErrorNumber()
{
    H5E_t	*estack = H5E_get_my_stack ();
    H5E_error_t *err_desc;
    H5E_minor_t min_num = H5E_NONE_MINOR;
    if (estack && estack->nused>0)
    {
        err_desc = estack->slot+0;
        min_num = err_desc->min_num;
    }
    return min_num;
}


#ifdef __cplusplus
}
#endif

