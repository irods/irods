/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */


#include "eirods_sql_logger.h"
#include "eirods_error.h"
#include "eirods_log.h"

#include "rodsLog.h"

#include <sstream>

namespace eirods {

    sql_logger::sql_logger(
	const std::string& _function_name,
	bool _logSQL) {
	name_ = _function_name;
	count_ = 0;
	log_sql_ = _logSQL;
    }

    sql_logger::~sql_logger(void) {
	// TODO - stub
    }

    void sql_logger::log(void) {
	if(log_sql_) {
	    if(count_ == 0) {
                eirods::log(LOG_SQL, name_);
	    } else {
		std::stringstream ss(std::stringstream::in);
		ss << name_ << " SQL " << count_ << " ";
		std::string log_string = ss.str();
                eirods::log(LOG_SQL, log_string);
	    }
	    ++count_;
	}
    }

}; // namespace eirods
