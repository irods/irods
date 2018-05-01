#include "irods_sql_logger.hpp"
#include "irods_error.hpp"
#include "irods_log.hpp"

#include "rodsLog.h"

#include <sstream>

namespace irods {

    sql_logger::sql_logger(
        const std::string& _function_name,
        bool _logSQL ) {
        name_ = _function_name;
        count_ = 0;
        log_sql_ = _logSQL;
    }

    sql_logger::~sql_logger( ) {
        // TODO - stub
    }

    void sql_logger::log( ) {
        if ( log_sql_ ) {
            if ( count_ == 0 ) {
                irods::log( LOG_SQL, name_ );
            }
            else {
                std::stringstream ss( std::stringstream::in );
                ss << name_ << " SQL " << count_ << " ";
                std::string log_string = ss.str();
                irods::log( LOG_SQL, log_string );
            }
            ++count_;
        }
    }

}; // namespace irods
