/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include "debug.hpp"
#include "locks.hpp"
#include "filesystem.hpp"
#include "utils.hpp"

int lockMutex( mutex_type **mutex ) {
    char sem_name[1024];
    getResourceName( sem_name, SEM_NAME );

    *mutex = new boost::interprocess::named_mutex( boost::interprocess::open_or_create, sem_name );
    ( *mutex )->lock();
    return 0;
}
void unlockMutex( mutex_type **mutex ) {
    ( *mutex )->unlock();
    delete *mutex;
}
/* This function can be used during initialization to remove a previously held mutex that has not been released.
 * This should only be used when there is no other process using the mutex */
void resetMutex( mutex_type **mutex ) {
    char sem_name[1024];
    getResourceName( sem_name, SEM_NAME );
    boost::interprocess::named_mutex::remove( sem_name );
}


