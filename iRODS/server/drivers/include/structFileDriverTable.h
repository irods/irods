/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to structFiles in the COPYRIGHT directory ***/

/* structFileDriverTable.h - header structFile for the global structFile driver table
 */



#ifndef STRUCT_FILE_DRIVER_TABLE_H
#define STRUCT_FILE_DRIVER_TABLE_H

#include "rods.h"
#include "structFileDriver.h"
#include "haawSubStructFileDriver.h"
#include "tarSubStructFileDriver.h"
#include "miscServerFunct.h"

// =-=-=-=-=-=-=-
// JMC - moving from intNoSupport to no-op fcns with matching signatures for g++
#include "fileDriverNoOpFunctions.h"
//#define NO_SUB_STRUCT_FILE_FUNCTIONS  intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,longNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport,intNoSupport, intNoSupport 
#define NO_SUB_STRUCT_FILE_FUNCTIONS  noSupportStructFileCreate, noSupportStructFileOpen, noSupportStructFileRead, noSupportStructFileWrite, noSupportStructFileClose, noSupportStructFileUnlink, noSupportStructFileStat, noSupportStructFileFstat, noSupportStructFileLseek, noSupportStructFileRename, noSupportStructFileMkdir, noSupportStructFileRmdir, noSupportStructFileOpendir, noSupportStructFileReaddir, noSupportStructFileClosedir, noSupportStructFileTruncate, noSupportStructeSync, noSupportStructeExtract

structFileDriver_t StructFileDriverTable[] = {
#ifdef HAAW_STRUCT_FILE
    {HAAW_STRUCT_FILE_T, haawSubStructFileCreate, haawSubStructFileOpen, 
      haawSubStructFileRead, haawSubStructFileWrite, haawSubStructFileClose, 
      haawSubStructFileUnlink, haawSubStructFileStat, haawSubStructFileFstat, 
      haawSubStructFileLseek, haawSubStructFileRename, haawSubStructFileMkdir,
      haawSubStructFileRmdir, haawSubStructFileOpendir, haawSubStructFileReaddir,
      haawSubStructFileClosedir, haawSubStructFileTruncate, haawStructFileSync,
      haawStructFileExtract},
#else
    {HAAW_STRUCT_FILE_T, NO_SUB_STRUCT_FILE_FUNCTIONS},
#endif
#ifdef TAR_STRUCT_FILE
    {TAR_STRUCT_FILE_T, tarSubStructFileCreate, tarSubStructFileOpen, 
      tarSubStructFileRead, tarSubStructFileWrite, tarSubStructFileClose, 
      tarSubStructFileUnlink, tarSubStructFileStat, tarSubStructFileFstat, 
      tarSubStructFileLseek, tarSubStructFileRename, tarSubStructFileMkdir,
      tarSubStructFileRmdir, tarSubStructFileOpendir, tarSubStructFileReaddir,
      tarSubStructFileClosedir, tarSubStructFileTruncate, tarStructFileSync,
      tarStructFileExtract},
#else
    {TAR_STRUCT_FILE_T, NO_SUB_STRUCT_FILE_FUNCTIONS},
#endif
};

int NumStructFileDriver = sizeof (StructFileDriverTable) / sizeof (structFileDriver_t);

#endif	/* STRUCT_FILE_DRIVER_TABLE_H */
