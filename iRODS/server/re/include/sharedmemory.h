/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H
#include "debug.h"

#ifdef USE_BOOST
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#else
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#define SHMMAX 30000000
#define SHM_BASE_ADDR ((void *)0x80000000)
#define shm_rname "SHM"
unsigned char *prepareServerSharedMemory();
void detachSharedMemory();
int removeSharedMemory();
unsigned char *prepareNonServerSharedMemory();
#endif /* SHAREDMEMORY_H */
