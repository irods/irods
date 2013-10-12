/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef RULEENGINELOCKS_H
#define RULEENGINELOCKS_H
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#define SEM_NAME "irods_sem_re"

typedef boost::interprocess::named_mutex mutex_type;
void unlockMutex(mutex_type **mutex);
int lockMutex(mutex_type **mutex);
void resetMutex(mutex_type **mutex);

#endif
