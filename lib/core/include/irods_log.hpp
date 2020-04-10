#ifndef __IRODS_LOG_HPP__
#define __IRODS_LOG_HPP__

#include "irods_error.hpp"
#include "rodsLog.h"
#include "stringOpr.h"
#include "irods_exception.hpp"

#include <ctime>
#include <string>

#include <sys/time.h>

namespace irods {
    void log( const error& );
    void log( int, const std::string& );
    void log( int, const char* );
    void log(const irods::exception& e);
}

#endif // __IRODS_LOG_HPP__
