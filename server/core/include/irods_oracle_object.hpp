#ifndef __IRODS_ORACLE_OBJECT_HPP__
#define __IRODS_ORACLE_OBJECT_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "irods_database_object.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {

// =-=-=-=-=-=-=-
// @brief
    const std::string ORACLE_DATABASE_PLUGIN( "oracle" );

// =-=-=-=-=-=-=-
// @brief oracle object class
    class oracle_object : public database_object {
        public:
            // =-=-=-=-=-=-=-
            // Constructors
            oracle_object();
            oracle_object( const oracle_object& );

            // =-=-=-=-=-=-=-
            // Destructors
            virtual ~oracle_object();

            // =-=-=-=-=-=-=-
            // Operators
            virtual oracle_object& operator=( const oracle_object& );

            // =-=-=-=-=-=-=-
            /// @brief Comparison operator
            virtual bool operator==( const oracle_object& _rhs ) const;

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

    }; // oracle_object

// =-=-=-=-=-=-=-
// helpful typedef for sock comm interface & factory
    typedef boost::shared_ptr< oracle_object > oracle_object_ptr;

}; // namespace irods

#endif // __IRODS_ORACLE_OBJECT_HPP__



