/* For copyright information please refer to files in the COPYRIGHT directory
 */
#ifndef DATETIME_HPP
#define DATETIME_HPP
/*#define __USE_XOPEN*/
#include <time.h>
#include "debug.hpp"
#ifndef DEBUG
#include "irods/rodsType.h"
#endif
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

typedef time_t time_type;
#define time_type_gt(mtim, timestamp) \
		(mtim > timestamp)
#define time_type_set(mtim, timestamp) \
		mtim = timestamp;
#define time_type_initializer ((time_type) 0)

int strttime( char* timestr, char* timeformat, rodsLong_t* t );
int ttimestr( char* buf, int n, char* timeformat, rodsLong_t* t );
#endif
