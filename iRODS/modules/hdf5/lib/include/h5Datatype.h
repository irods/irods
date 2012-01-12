
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

#ifndef _H5DATATYPE_H
#define _H5DATATYPE_H

/* Define basic structure for H5Datatype object */

/* these are the various classes of datatypes borrowed from H5T_class_t */
typedef enum H5Datatype_class_t {
    H5DATATYPE_NO_CLASS         = -1,  /*error                              */
    H5DATATYPE_INTEGER          = 0,   /*integer types                      */
    H5DATATYPE_FLOAT            = 1,   /*floating-point types               */
    H5DATATYPE_TIME             = 2,   /*date and time types                */
    H5DATATYPE_STRING           = 3,   /*character string types             */
    H5DATATYPE_BITFIELD         = 4,   /*bit field types                    */
    H5DATATYPE_OPAQUE           = 5,   /*opaque types                       */
    H5DATATYPE_COMPOUND         = 6,   /*compound types                     */
    H5DATATYPE_REFERENCE        = 7,   /*reference types                    */
    H5DATATYPE_ENUM             = 8,   /*enumeration types                  */
    H5DATATYPE_VLEN             = 9,   /*Variable-Length types              */
    H5DATATYPE_ARRAY            = 10,  /*Array types                        */
    H5DATATYPE_NCLASSES                /*this must be last                  */
} H5Datatype_class_t;

/* Byte orders borrowed from H5T_order_t */
typedef enum H5Datattype_order_t {
    H5DATATYPE_ORDER_ERROR      = -1,  /*error                               */
    H5DATATYPE_ORDER_LE         = 0,   /*little endian                       */
    H5DATATYPE_ORDER_BE         = 1,   /*bit endian                          */
    H5DATATYPE_ORDER_VAX        = 2,   /*VAX mixed endian                    */
    H5DATATYPE_ORDER_NONE       = 3    /*no particular order                 */
    /*H5DATATYPE_ORDER_NONE must be last */
} H5Datatype_order_t;

/* Types of integer sign schemes borrowed from H5_sign_t */
typedef enum H5Datatype_sign_t {
    H5DATATYPE_SGN_ERROR        = -1,  /*error                                */
    H5DATATYPE_SGN_NONE         = 0,   /*this is an unsigned type             */
    H5DATATYPE_SGN_2            = 1,   /*two's complement                     */
    H5DATATYPE_NSGN             = 2    /*this must be last!                   */
} H5Datatype_sign_t;

/* client needs to set the byte order so that the sever can check 
 * if the byte order matches 
 */
typedef struct H5Datatype
{
    H5Datatype_class_t tclass;
    H5Datatype_order_t order;
    H5Datatype_sign_t sign;
    unsigned int size;

    /* For compound dataaet only. The following fields are expanded to atomic
       types. For example, T is a compound datatype has member of A, B(int) 
       and C(float), and A is compound with members X(unsign int) and Y(char), 
       then:
       nmembers = 4;
       mnames[] = {"A.X", "A.Y", "B", "C"}; 

       memeber dataype is packed into 32-bit integer as 4bit_class+4bit_sign+24bit_size, 
           i.e. mtype = mclass<<28 | msign<<24 | msize
       To extract mtype information, 
           int mclass = (0XFFFFFFF & mtype)>>28;
           int msign = (0XFFFFFFF & mtype)>>24;
           int msize = (0XFFFFFF & mtype);
    */
    int nmembers;          /* number of members of the compound datatype */
    int *mtypes;          /* datatypes of members */
    char** mnames;        /* names of members */
} H5Datatype;

#ifdef __cplusplus
extern "C" {
#endif

/*constructor: always call the constructor of the structure before use it */
void H5Datatype_ctor(H5Datatype* t);

/*destructor: always call the destructor of the structure before use it */
void H5Datatype_dtor(H5Datatype* t);

int get_machine_endian(void);

#ifdef __cplusplus
}
#endif

#endif

