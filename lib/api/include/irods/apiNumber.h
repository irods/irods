/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* apiNumber.h - header file for API number assignment
 */

#ifndef API_NUMBER_H__
#define API_NUMBER_H__

#define API_NUMBER(NAME, VALUE) static const int NAME = VALUE;

#include "irods/apiNumberData.h"

#undef API_NUMBER

#endif  // API_NUMBER_H__
