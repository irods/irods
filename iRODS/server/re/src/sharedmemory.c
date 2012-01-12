/* For copyright information please refer to files in the COPYRIGHT directory
 */

#include <fcntl.h>
#include "debug.h"
#include "sharedmemory.h"
#include "utils.h"
#include "filesystem.h"


#ifdef USE_BOOST
static boost::interprocess::shared_memory_object *shm_obj = NULL;
static boost::interprocess::mapped_region *mapped = NULL;
#else
int shmid = - 1;
void *shmBuf = NULL;
#endif

unsigned char *prepareServerSharedMemory() {
	char shm_name[1024];
	getResourceName(shm_name, shm_rname);
#ifdef USE_BOOST
	shm_obj = new boost::interprocess::shared_memory_object(boost::interprocess::open_or_create, shm_name, boost::interprocess::read_write);
	boost::interprocess::offset_t size;
	if(shm_obj->get_size(size) && size==0) {
		shm_obj->truncate(SHMMAX);
	}
	mapped = new boost::interprocess::mapped_region(*shm_obj, boost::interprocess::read_write);
	unsigned char *shmBuf = (unsigned char *) mapped->get_address();
	return shmBuf;
#else
		shmid = shm_open(shm_name, O_RDWR | O_CREAT, 0600);
		if(shmid!= -1) {
			if(ftruncate(shmid, SHMMAX) == -1) {
				close(shmid);
				shm_unlink(shm_name);
				return NULL;
			}
			shmBuf = mmap(SHM_BASE_ADDR, SHMMAX, PROT_READ | PROT_WRITE, MAP_SHARED, shmid , 0);
			if(shmBuf == MAP_FAILED) {
				close(shmid);
				shm_unlink(shm_name);
				return NULL;
			}
			return (unsigned char *) shmBuf;
		} else {
			return NULL;
		}
#endif
}

void detachSharedMemory() {
#ifdef USE_BOOST
	delete mapped;
	delete shm_obj;
#else
	munmap(shmBuf, SHMMAX);
	close(shmid);
#endif
}

int removeSharedMemory() {
	char shm_name[1024];
	getResourceName(shm_name, shm_rname);
#ifdef USE_BOOST
	if(!boost::interprocess::shared_memory_object::remove(shm_name)) {
		return SHM_UNLINK_ERROR;
	}

#else
	if(shm_unlink(shm_name) == -1) {
		return SHM_UNLINK_ERROR;
	}
#endif
	return 0;
}

unsigned char *prepareNonServerSharedMemory() {
	char shm_name[1024];
	getResourceName(shm_name, shm_rname);
#ifdef USE_BOOST

    try {
    	shm_obj = new boost::interprocess::shared_memory_object(boost::interprocess::open_only, shm_name, boost::interprocess::read_only);
    	mapped = new boost::interprocess::mapped_region(*shm_obj, boost::interprocess::read_only);
    	unsigned char *buf = (unsigned char *) mapped->get_address();
    	return buf;
    } catch(boost::interprocess::interprocess_exception e) {
    	return NULL;
    }
#else
	shmid = shm_open(shm_name, O_RDONLY, 0400);
	if(shmid!= -1) {
		shmBuf = mmap(SHM_BASE_ADDR, SHMMAX, PROT_READ, MAP_SHARED, shmid, 0);
		if(shmBuf == MAP_FAILED) { /* not server process and shm is successfully allocated */
			close(shmid);
			return NULL;
		}
		return (unsigned char *) shmBuf;
	} else {
		return NULL;
	}
#endif
}
