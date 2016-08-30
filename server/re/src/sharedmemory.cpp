/* For copyright information please refer to files in the COPYRIGHT directory
 */

#include <fcntl.h>
#include "sharedmemory.hpp"
#include "rodsConnect.h"
#include "irods_server_properties.hpp"

namespace bi = boost::interprocess;

static std::map<std::string,std::unique_ptr<bi::shared_memory_object>> shm_obj;
static std::map<std::string,std::unique_ptr<bi::mapped_region>>        mapped; 

unsigned char *prepareServerSharedMemory( const std::string& _key ) {
    std::string shared_memory_name;
    irods::error ret = getSharedMemoryName( _key, shared_memory_name );
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "prepareServerSharedMemory: failed to get shared memory name" );
        return NULL;
    }

    try {
        bi::shared_memory_object* tmp_s = new bi::shared_memory_object(
                                            bi::open_or_create,
                                            shared_memory_name.c_str(),
                                            bi::read_write,
                                            0600 );
        shm_obj[_key] = std::unique_ptr<bi::shared_memory_object>(tmp_s);

        bi::offset_t size;
        if ( tmp_s->get_size( size ) && size == 0 ) {
            tmp_s->truncate( SHMMAX );
        }
        
        bi::mapped_region* tmp_m = new bi::mapped_region( *tmp_s, bi::read_write );
        unsigned char *shmBuf = ( unsigned char * ) tmp_m->get_address();

        std::memset( shmBuf, 0, tmp_m->get_size() );
        strncpy( (char*)shmBuf, "UNINITIALIZED", 15 );

        mapped[_key] = std::unique_ptr<bi::mapped_region>(tmp_m);


        return shmBuf;
    }
    catch ( const bi::interprocess_exception &e ) {
        rodsLog( LOG_ERROR, "prepareServerSharedMemory: failed to prepare shared memory. Exception caught [%s]", e.what() );
        return NULL;
    }
}

void detachSharedMemory( const std::string& _key ) {
    //delete mapped;
    //delete shm_obj;
}

int removeSharedMemory( const std::string& _key ) {
    std::string shared_memory_name;
    irods::error ret = getSharedMemoryName( _key, shared_memory_name );
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "removeSharedMemory: failed to get shared memory name" );
        return RE_SHM_UNLINK_ERROR;
    }

    if ( !bi::shared_memory_object::remove( shared_memory_name.c_str() ) ) {
        rodsLog( LOG_ERROR, "removeSharedMemory: failed to remove shared memory" );
        return RE_SHM_UNLINK_ERROR;
    }
    return 0;
}

unsigned char *prepareNonServerSharedMemory( const std::string& _key ) {
    std::string shared_memory_name;
    irods::error ret = getSharedMemoryName( _key, shared_memory_name );
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "prepareNonServerSharedMemory: failed to get shared memory name [%s]", shared_memory_name.c_str() );
        return NULL;
    }

    try {
        bi::shared_memory_object* tmp_s = new bi::shared_memory_object(
                                                  bi::open_only,
                                                  shared_memory_name.c_str(),
                                                  bi::read_only );
        shm_obj[_key] = std::unique_ptr<bi::shared_memory_object>(tmp_s);
        bi::mapped_region* tmp_m = new bi::mapped_region( *tmp_s, bi::read_only );
        unsigned char *buf = ( unsigned char * ) tmp_m->get_address();
        mapped[_key] = std::unique_ptr<bi::mapped_region>(tmp_m);
        return buf;
    }
    catch ( const bi::interprocess_exception &e ) {
        rodsLog( LOG_ERROR, "prepareNonServerSharedMemory: failed to get shared memory object [%s]. Exception caught [%s]", shared_memory_name.c_str(), e.what() );
        return NULL;
    }
}

irods::error getSharedMemoryName( const std::string& _key, std::string &shared_memory_name ) {
    try {
        const auto& shared_memory_name_salt = irods::get_server_property<const std::string>(irods::CFG_RE_CACHE_SALT_KW);
        shared_memory_name = "irods_re_cache_shared_memory_" + _key + "_" + shared_memory_name_salt;
    } catch ( const irods::exception& e ) {
        rodsLog( LOG_ERROR, "getSharedMemoryName: failed to retrieve re cache salt from server_properties\n%s", e.what() );
        return irods::error(e);
    }
    return SUCCESS();
}
