#ifndef __IRODS_GENERIC_DATABASE_OBJECT_HPP__
#define __IRODS_GENERIC_DATABASE_OBJECT_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "irods_database_object.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {


// =-=-=-=-=-=-=-
// @brief generic database object class
    class generic_database_object : public database_object {
        public:
            // =-=-=-=-=-=-=-
            // Constructors
            explicit generic_database_object(const std::string &);
            generic_database_object( const generic_database_object& );

            // =-=-=-=-=-=-=-
            // Destructors
            virtual ~generic_database_object();

            // =-=-=-=-=-=-=-
            // Operators
            virtual generic_database_object& operator=( const generic_database_object& );

            // =-=-=-=-=-=-=-
            /// @brief Comparison operator
            virtual bool operator==( const generic_database_object& _rhs ) const;

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
	    std::string type_;
    }; // generic_database_object

// =-=-=-=-=-=-=-
// helpful typedef for sock comm interface & factory
    typedef boost::shared_ptr< generic_database_object > generic_database_object_ptr;

}; // namespace irods

#endif // __IRODS_GENERIC_DATABASE_OBJECT_HPP__



