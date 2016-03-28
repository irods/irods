/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef SHAREDMEMORY_HPP
#define SHAREDMEMORY_HPP

#include <assert.h>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "irods_error.hpp"
#include <string>

#define SHMMAX 30000000
#define SHM_BASE_ADDR ((void *)0x80000000)
unsigned char *prepareServerSharedMemory( const std::string& );
void detachSharedMemory( const std::string& );
int removeSharedMemory( const std::string& );
unsigned char *prepareNonServerSharedMemory( const std::string& );
irods::error getSharedMemoryName( const std::string&, std::string &shared_memory_name );
#endif /* SHAREDMEMORY_H */
