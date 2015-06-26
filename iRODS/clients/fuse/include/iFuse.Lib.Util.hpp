/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#ifndef IFUSE_LIB_UTIL_HPP
#define	IFUSE_LIB_UTIL_HPP

#include <time.h>

#ifndef UNUSED
#define UNUSED(expr) (void)(expr)
#endif

time_t iFuseLibGetCurrentTime();
void iFuseLibGetStrCurrentTime(char *buff);
double IFuseLibDiffTimeSec(time_t end, time_t beginning);

#endif	/* IFUSE_LIB_UTIL_HPP */

