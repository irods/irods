/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef RULEENGINELOCKS_HPP
#define RULEENGINELOCKS_HPP
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include "irods_error.hpp"

typedef boost::interprocess::named_mutex mutex_type;
void unlockMutex( mutex_type **mutex );
int lockMutex( mutex_type **mutex );
void resetMutex();
irods::error getMutexName( std::string &mutex_name );

#endif
