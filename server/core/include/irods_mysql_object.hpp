#ifndef __IRODS_MYSQL_OBJECT_HPP__
#define __IRODS_MYSQL_OBJECT_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "irods_database_object.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {

// =-=-=-=-=-=-=-
// @brief
    const std::string MYSQL_DATABASE_PLUGIN( "mysql" );

// =-=-=-=-=-=-=-
// @brief mysql object class
    class mysql_object : public database_object {
        public:
            // =-=-=-=-=-=-=-
            // Constructors
            mysql_object();
            mysql_object( const mysql_object& );

            // =-=-=-=-=-=-=-
            // Destructors
            virtual ~mysql_object();

            // =-=-=-=-=-=-=-
            // Operators
            virtual mysql_object& operator=( const mysql_object& );

            // =-=-=-=-=-=-=-
            /// @brief Comparison operator
            virtual bool operator==( const mysql_object& _rhs ) const;

            // =-=-=-=-=-=-=-
            // plugin resolution operation
            virtual error resolve(
                const std::string&, // plugin interface
                plugin_ptr& );      // resolved plugin

            // =-=-=-=-=-=-=-
            // accessor for rule engine variables
            virtual error get_re_vars( rule_engine_vars_t& );

            // =-=-=-=-=-=-=-
            // Accessors

            // =-=-=-=-=-=-=-
            // Mutators

        private:
            // =-=-=-=-=-=-=-
            // Attributes

    }; // mysql_object

// =-=-=-=-=-=-=-
// helpful typedef for sock comm interface & factory
    typedef boost::shared_ptr< mysql_object > mysql_object_ptr;

}; // namespace irods

#endif // __IRODS_MYSQL_OBJECT_HPP__



