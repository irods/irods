/* For copyright information please refer to files in the COPYRIGHT directory
 */
#include "locks.hpp"
#include "rodsConnect.h"
//#include "filesystem.hpp"
//#include "utils.hpp"
#include "irods_log.hpp"
#include "irods_server_properties.hpp"

int lockWriteMutex( const char* _inst_name, mutex_type **mutex ) {
    std::string mutex_name;
    irods::error ret = getMutexName( _inst_name, mutex_name );
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "lockMutex: call to getMutexName failed" );
        return -1;
    }

    try {
        *mutex = new mutex_type( boost::interprocess::open_or_create, mutex_name.c_str() );
    }
    catch ( const boost::interprocess::interprocess_exception& ) {
        rodsLog( LOG_ERROR, "boost::interprocess::named_mutex threw a boost::interprocess::interprocess_exception." );
        return -1;
    }
    try {
        ( *mutex )->lock();
    }
    catch ( const boost::interprocess::interprocess_exception& ) {
        rodsLog( LOG_ERROR, "lock threw a boost::interprocess::interprocess_exception." );
        return -1;
    }
    return 0;
}

int lockReadMutex( const char* _inst_name, mutex_type **mutex ) {
    std::string mutex_name;
    irods::error ret = getMutexName( _inst_name, mutex_name );
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "lockMutex: call to getMutexName failed" );
        return -1;
    }

    try {
        *mutex = new mutex_type( boost::interprocess::open_or_create, mutex_name.c_str() );
    }
    catch ( const boost::interprocess::interprocess_exception& ) {
        rodsLog( LOG_ERROR, "boost::interprocess::named_mutex threw a boost::interprocess::interprocess_exception." );
        return -1;
    }
    try {
        ( *mutex )->lock_sharable();
    }
    catch ( const boost::interprocess::interprocess_exception& ) {
        rodsLog( LOG_ERROR, "lock threw a boost::interprocess::interprocess_exception." );
        return -1;
    }
    return 0;
}

void unlockWriteMutex( const char* _inst_name, mutex_type **mutex ) {
    try {
        ( *mutex )->unlock();
    }
    catch ( const boost::interprocess::interprocess_exception& ) {
        rodsLog( LOG_ERROR, "unlock threw a boost::interprocess::interprocess_exception." );
    }
    delete *mutex;
}

void unlockReadMutex( const char* _inst_name, mutex_type **mutex ) {
    try {
        ( *mutex )->unlock_sharable();
    }
    catch ( const boost::interprocess::interprocess_exception& ) {
        rodsLog( LOG_ERROR, "unlock threw a boost::interprocess::interprocess_exception." );
    }
    delete *mutex;
}

/* This function can be used during initialization to remove a previously held mutex that has not been released.
 * This should only be used when there is no other process using the mutex */
void resetMutex(const char* _inst_name) {
    std::string mutex_name;
    irods::error ret = getMutexName( _inst_name, mutex_name );
    if ( !ret.ok() ) {
        rodsLog( LOG_ERROR, "resetMutex: call to getMutexName failed" );
    }

    mutex_type::remove( mutex_name.c_str() );
}

irods::error getMutexName( const char* _inst_name, std::string &mutex_name ) {
    try {
        const auto& mutex_name_salt = irods::get_server_property<const std::string>(irods::CFG_RE_CACHE_SALT_KW);
        mutex_name = "irods_re_cache_mutex_";
        mutex_name += _inst_name;
        mutex_name += "_";
        mutex_name += mutex_name_salt;
    } catch ( const irods::exception& e ) {
        rodsLog( LOG_ERROR, "getMutexName: failed to retrieve re cache salt from server_properties\n%s", e.what() );
        return irods::error(e);
    }
    return SUCCESS();
}
