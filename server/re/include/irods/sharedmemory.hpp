#ifndef IRODS_SHAREDMEMORY_HPP
#define IRODS_SHAREDMEMORY_HPP

#include "irods/irods_error.hpp"

#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <string>

#define SHMMAX        30000000
#define SHM_BASE_ADDR ((void*) 0x80000000)

unsigned char *prepareServerSharedMemory( const std::string& );

int removeSharedMemory( const std::string& );

unsigned char *prepareNonServerSharedMemory( const std::string& );

irods::error getSharedMemoryName(const std::string&, std::string&);

#endif // IRODS_SHAREDMEMORY_HPP
