/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef SHAREDMEMORY_HPP
#define SHAREDMEMORY_HPP
#include "debug.hpp"

#include <assert.h>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "irods_error.hpp"

#define SHMMAX 30000000
#define SHM_BASE_ADDR ((void *)0x80000000)
unsigned char *prepareServerSharedMemory();
void detachSharedMemory();
int removeSharedMemory();
unsigned char *prepareNonServerSharedMemory();
irods::error getSharedMemoryName( std::string &shared_memory_name );
#endif /* SHAREDMEMORY_H */
