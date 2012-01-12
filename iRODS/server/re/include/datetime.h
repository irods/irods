/* For copyright information please refer to files in the COPYRIGHT directory
 */
#ifndef DATETIME_H
#define DATETIME_H
/*#define __USE_XOPEN*/
#include <time.h>
#include "debug.h"
#ifndef DEBUG
#include "rodsType.h"
#endif
#if defined(USE_BOOST)
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/filesystem.hpp>
#endif
typedef time_t time_type;
#define time_type_gt(mtim, timestamp) \
		(mtim > timestamp)
#define time_type_set(mtim, timestamp) \
		mtim = timestamp;
#define time_type_initializer ((time_type) 0)
/*#elif defined( _POSIX_VERSION )
typedef struct timespec time_type;
#define time_type_gt(mtim, timestamp) \
		((mtim).tv_sec > (timestamp).tv_sec || \
		((mtim).tv_sec == (timestamp).tv_sec && (mtim).tv_nsec > (timestamp).tv_nsec))
#define time_type_set(mtim, timestamp) \
		(mtim).tv_sec = (timestamp).tv_sec; \
		(mtim).tv_nsec = (timestamp).tv_nsec;
#define time_type_initializer {0, 0}
#endif*/

int strttime(char* timestr, char* timeformat, rodsLong_t* t);
int ttimestr(char* buf, int n, char* timeformat, rodsLong_t* t);
#endif
