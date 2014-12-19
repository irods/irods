#ifndef _IRODS_SQL_LOGGER_HPP_
#define _IRODS_SQL_LOGGER_HPP_

#include <string>

namespace irods {

    class sql_logger {
        public:
            sql_logger( const std::string& _function_name, bool logSQL );
            virtual ~sql_logger( void );

            void log( void );

        private:
            unsigned int count_;
            std::string name_;
            bool log_sql_;
    };
}; // namespace irods

#endif // _IRODS_SQL_LOGGER_HPP_
