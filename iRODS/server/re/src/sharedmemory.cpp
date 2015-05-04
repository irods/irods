/* For copyright information please refer to files in the COPYRIGHT directory
 */

#include <fcntl.h>
#include "debug.hpp"
#include "sharedmemory.hpp"
#include "utils.hpp"
#include "filesystem.hpp"
#include "irods_server_properties.hpp"

static boost::interprocess::shared_memory_object *shm_obj = NULL;
static boost::interprocess::mapped_region *mapped = NULL;

unsigned char *prepareServerSharedMemory() {
    std::string shared_memory_name;
    irods::error ret = getSharedMemoryName( shared_memory_name );
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "prepareServerSharedMemory: failed to get shared memory name" );
        return NULL;
    }

    try {
        shm_obj = new boost::interprocess::shared_memory_object( boost::interprocess::open_or_create, shared_memory_name.c_str(), boost::interprocess::read_write, 0600 );
        boost::interprocess::offset_t size;
        if ( shm_obj->get_size( size ) && size == 0 ) {
            shm_obj->truncate( SHMMAX );
        }
        mapped = new boost::interprocess::mapped_region( *shm_obj, boost::interprocess::read_write );
        unsigned char *shmBuf = ( unsigned char * ) mapped->get_address();
        return shmBuf;
    }
    catch ( const boost::interprocess::interprocess_exception &e ) {
        rodsLog( LOG_ERROR, "prepareServerSharedMemory: failed to prepare shared memory. Exception caught [%s]", e.what() );
        return NULL;
    }
}

void detachSharedMemory() {
    delete mapped;
    delete shm_obj;
}

int removeSharedMemory() {
    std::string shared_memory_name;
    irods::error ret = getSharedMemoryName( shared_memory_name );
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "removeSharedMemory: failed to get shared memory name" );
        return RE_SHM_UNLINK_ERROR;
    }

    if ( !boost::interprocess::shared_memory_object::remove( shared_memory_name.c_str() ) ) {
        rodsLog( LOG_ERROR, "removeSharedMemory: failed to remove shared memory" );
        return RE_SHM_UNLINK_ERROR;
    }
    return 0;
}

unsigned char *prepareNonServerSharedMemory() {
    std::string shared_memory_name;
    irods::error ret = getSharedMemoryName( shared_memory_name );
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "prepareNonServerSharedMemory: failed to get shared memory name [%s]", shared_memory_name.c_str() );
        return NULL;
    }

    try {
        shm_obj = new boost::interprocess::shared_memory_object( boost::interprocess::open_only, shared_memory_name.c_str(), boost::interprocess::read_only );
        mapped = new boost::interprocess::mapped_region( *shm_obj, boost::interprocess::read_only );
        unsigned char *buf = ( unsigned char * ) mapped->get_address();
        return buf;
    }
    catch ( const boost::interprocess::interprocess_exception &e ) {
        rodsLog( LOG_ERROR, "prepareNonServerSharedMemory: failed to get shared memory object [%s]. Exception caught [%s]", shared_memory_name.c_str(), e.what() );
        return NULL;
    }
}

irods::error getSharedMemoryName( std::string &shared_memory_name ) {
    std::string shared_memory_name_salt;
    irods::error ret = irods::server_properties::getInstance().get_property<std::string>( RE_CACHE_SALT_KW, shared_memory_name_salt );
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "getSharedMemoryName: failed to retrieve re cache salt from server_properties\n%s", ret.result().c_str() );
        return PASS( ret );
    }

    shared_memory_name = "irods_re_cache_shared_memory_" + shared_memory_name_salt;

    return SUCCESS();
}
