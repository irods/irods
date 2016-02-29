#ifndef __IRODS_DATABASE_OBJECT_HPP__
#define __IRODS_DATABASE_OBJECT_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "irods_first_class_object.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {
// =-=-=-=-=-=-=-
// network object base class
    class database_object : public first_class_object {
        public:
            // =-=-=-=-=-=-=-
            // Constructors
            database_object();
            database_object( const database_object& );

            // =-=-=-=-=-=-=-
            // Destructors
            virtual ~database_object();

            // =-=-=-=-=-=-=-
            // Operators
            virtual database_object& operator=( const database_object& );

            // =-=-=-=-=-=-=-
            /// @brief Comparison operator
            virtual bool operator==( const database_object& _rhs ) const;

            // =-=-=-=-=-=-=-
            // plugin resolution operation
            virtual error resolve(
                const std::string&, // plugin interface
                plugin_ptr& ) = 0;  // resolved plugin

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

    }; // database_object

// =-=-=-=-=-=-=-
// helpful typedef for sock comm interface & factory
    typedef boost::shared_ptr< database_object > database_object_ptr;

}; // namespace irods

#endif // __IRODS_DATABASE_OBJECT_HPP__



