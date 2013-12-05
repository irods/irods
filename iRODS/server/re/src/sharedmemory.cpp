/* For copyright information please refer to files in the COPYRIGHT directory
 */

#include <fcntl.h>
#include "debug.hpp"
#include "sharedmemory.hpp"
#include "utils.hpp"
#include "filesystem.hpp"


static boost::interprocess::shared_memory_object *shm_obj = NULL;
static boost::interprocess::mapped_region *mapped = NULL;

unsigned char *prepareServerSharedMemory() {
    char shm_name[1024];
    getResourceName( shm_name, shm_rname );
    shm_obj = new boost::interprocess::shared_memory_object( boost::interprocess::open_or_create, shm_name, boost::interprocess::read_write );
    boost::interprocess::offset_t size;
    if ( shm_obj->get_size( size ) && size == 0 ) {
        shm_obj->truncate( SHMMAX );
    }
    mapped = new boost::interprocess::mapped_region( *shm_obj, boost::interprocess::read_write );
    unsigned char *shmBuf = ( unsigned char * ) mapped->get_address();
    return shmBuf;
}

void detachSharedMemory() {
    delete mapped;
    delete shm_obj;
}

int removeSharedMemory() {
    char shm_name[1024];
    getResourceName( shm_name, shm_rname );
    if ( !boost::interprocess::shared_memory_object::remove( shm_name ) ) {
        return RE_SHM_UNLINK_ERROR;
    }
    return 0;
}

unsigned char *prepareNonServerSharedMemory() {
    char shm_name[1024];
    getResourceName( shm_name, shm_rname );
    try {
        shm_obj = new boost::interprocess::shared_memory_object( boost::interprocess::open_only, shm_name, boost::interprocess::read_only );
        mapped = new boost::interprocess::mapped_region( *shm_obj, boost::interprocess::read_only );
        unsigned char *buf = ( unsigned char * ) mapped->get_address();
        return buf;
    }
    catch ( boost::interprocess::interprocess_exception e ) {
        return NULL;
    }
}
