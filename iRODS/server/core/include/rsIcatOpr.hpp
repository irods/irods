/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rsIcatOpr.h - common header file rsIcatOpr.c
 */



#ifndef RS_ICAT_OPR_HPP
#define RS_ICAT_OPR_HPP

#include "rods.hpp"
#include "getRodsEnv.hpp"

int
connectRcat();
int
disconnectRcat();
int
resetRcat();
#endif	/* RS_ICAT_OPR_H */
