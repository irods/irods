/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is written by Illyoung Choi (iychoi@email.arizona.edu)      ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#ifndef IFUSECMDLINEOPT_HPP
#define	IFUSECMDLINEOPT_HPP

#include "iFuse.Lib.hpp"

void iFuseCmdOptsInit();
void iFuseCmdOptsDestroy();
void iFuseCmdOptsParse(int argc, char **argv);
void iFuseCmdOptsAdd(char *opt);
void iFuseGetOption(iFuseOpt_t *opt);
void iFuseGenCmdLineForFuse(int *fuse_argc, char ***fuse_argv);
void iFuseReleaseCmdLineForFuse(int fuse_argc, char **fuse_argv);

#endif	/* IFUSECMDLINEOPT_HPP */

