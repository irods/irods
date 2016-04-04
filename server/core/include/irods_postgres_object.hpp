#ifndef __IRODS_POSTGRES_OBJECT_HPP__
#define __IRODS_POSTGRES_OBJECT_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "irods_database_object.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {

// =-=-=-=-=-=-=-
// @brief
    const std::string POSTGRES_DATABASE_PLUGIN( "postgres" );

// =-=-=-=-=-=-=-
// @brief postgres object class
    class postgres_object : public database_object {
        public:
            // =-=-=-=-=-=-=-
            // Constructors
            postgres_object();
            postgres_object( const postgres_object& );

            // =-=-=-=-=-=-=-
            // Destructors
            virtual ~postgres_object();

            // =-=-=-=-=-=-=-
            // Operators
            virtual postgres_object& operator=( const postgres_object& );

            // =-=-=-=-=-=-=-
            /// @brief Comparison operator
            virtual bool operator==( const postgres_object& _rhs ) const;

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

    }; // postgres_object

// =-=-=-=-=-=-=-
// helpful typedef for sock comm interface & factory
    typedef boost::shared_ptr< postgres_object > postgres_object_ptr;

}; // namespace irods

#endif // __IRODS_POSTGRES_OBJECT_HPP__



