/* For copyright information please refer to files in the COPYRIGHT directory
 */

#ifndef RULEENGINELOCKS_HPP
#define RULEENGINELOCKS_HPP
#include <boost/interprocess/sync/named_sharable_mutex.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include "irods_error.hpp"

typedef boost::interprocess::named_sharable_mutex mutex_type;
void unlockReadMutex( const char*, mutex_type **mutex );
int lockReadMutex(  const char*, mutex_type **mutex );
void unlockWriteMutex( const char*, mutex_type **mutex );
int lockWriteMutex(  const char*, mutex_type **mutex );
void resetMutex( const char* );
irods::error getMutexName( const char*, std::string &mutex_name );

#endif
