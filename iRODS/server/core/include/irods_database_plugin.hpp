#ifndef ___IRODS_DATABASE_PLUGIN_HPP__
#define ___IRODS_DATABASE_PLUGIN_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "irods_plugin_base.hpp"
#include "irods_database_types.hpp"

#include <iostream>

namespace irods {
    /**
     * \author Jason M. Coposky
     * \brief
     *
     **/
    class database : public plugin_base {
        public:
            // =-=-=-=-=-=-=-
            /// @brief Constructors
            database( const std::string& _inst,
                      const std::string& _ctx ) :
                        plugin_base(
                            _inst,
                            _ctx ) {
            }

            // =-=-=-=-=-=-=-
            /// @brief Destructor
            virtual ~database() {
            }

            // =-=-=-=-=-=-=-
            /// @brief copy ctor
            database( const database& _rhs ) :
                plugin_base(_rhs ) {
            }

            // =-=-=-=-=-=-=-
            /// @brief Assignment Operator - necessary for stl containers
            database& operator=( const database& _rhs ) {
                if(&_rhs == this) { return *this; }
                plugin_base::operator=( _rhs );
                return *this;
            }

    }; // class database



}; // namespace irods


#endif // ___IRODS_DATABASE_PLUGIN_HPP__



