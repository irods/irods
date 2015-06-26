/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include "iFuse.Lib.Util.hpp"

time_t iFuseLibGetCurrentTime() {
    return time(NULL);
}

double IFuseLibDiffTimeSec(time_t end, time_t beginning) {
    return difftime(end, beginning);
}
