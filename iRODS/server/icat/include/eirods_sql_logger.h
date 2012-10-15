/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _sql_logger_H_
#define _sql_logger_H_

#include <string>

namespace eirods {

    class sql_logger {
    public:
	sql_logger(const std::string& _function_name, bool logSQL);
	virtual ~sql_logger(void);

	void log(void);
	
    private:
	unsigned int count_;
	std::string name_;
	bool log_sql_;
    };
}; // namespace eirods

#endif // _sql_logger_H_
