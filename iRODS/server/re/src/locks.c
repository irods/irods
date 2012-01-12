/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include "debug.h"
#include "locks.h"
#include "filesystem.h"
#include "utils.h"

int lockMutex(mutex_type **mutex) {
	char sem_name[1024];
	getResourceName(sem_name, SEM_NAME);
#ifdef USE_BOOST
	*mutex = new boost::interprocess::named_mutex(boost::interprocess::open_or_create, sem_name);
	(*mutex)->lock();
	return 0;
#else
	*mutex = sem_open(sem_name,O_CREAT,0644,1);
	if(*mutex == SEM_FAILED)
    {
      perror("unable to create semaphore");
      sem_unlink(SEM_NAME);
      return -1;
    } else {
      int v;
      sem_getvalue(*mutex, &v);
      /* printf("sem val0: %d\n", v); */
      sem_wait(*mutex);
      sem_getvalue(*mutex, &v);
      /* printf("sem val1: %d\n", v); */
      sem_close(*mutex);
      return 0;
    }
#endif
}
void unlockMutex(mutex_type **mutex) {
#ifdef USE_BOOST
	(*mutex)->unlock();
	delete *mutex;
#else
	char sem_name[1024];
	getResourceName(sem_name, SEM_NAME);
	int v;
	*mutex = sem_open(sem_name,O_CREAT,0644,1);
	sem_getvalue(*mutex, &v);
	/* printf("sem val2: %d\n", v); */
	sem_post(*mutex);
	sem_getvalue(*mutex, &v);
	/* printf("sem val3: %d\n", v); */
	sem_close(*mutex);
#endif
}
/* This function can be used during intialization to unlock previous held mutex that has not been released.
 * This can only be used when there is no other process using the mutex */
void resetMutex(mutex_type **mutex) {
	char sem_name[1024];
	getResourceName(sem_name, SEM_NAME);
#ifdef USE_BOOST
	try {
		mutex_type *mutex0 = new boost::interprocess::named_mutex(boost::interprocess::open_only, sem_name);
		mutex0->unlock();
		delete mutex0;
		boost::interprocess::named_mutex::remove(sem_name);
	} catch (boost::interprocess::interprocess_exception e) {

	}
#else
	sem_unlink(sem_name);
#endif
}


