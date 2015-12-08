#ifndef ___IRODS_NETWORK_PLUGIN_HPP__
#define ___IRODS_NETWORK_PLUGIN_HPP__

// =-=-=-=-=-=-=-
#include "irods_plugin_base.hpp"
#include "irods_network_types.hpp"

#include <iostream>

namespace irods {

// =-=-=-=-=-=-=-
    /**
     * \author Jason M. Coposky
     * \brief
     *
     **/
    class network : public plugin_base {
        public:
            // =-=-=-=-=-=-=-
            // public - ctor
            network(
                    const std::string& _inst,
                    const std::string& _ctx ) :
                plugin_base(
                        _inst,
                        _ctx ) {
                } // ctor

            // =-=-=-=-=-=-=-
            // public - dtor
            virtual ~network( ) {
            } // dtor

            // =-=-=-=-=-=-=-
            // public - cctor
            network(const network& _rhs) :
                plugin_base( _rhs ) {
                } // cctor

            // =-=-=-=-=-=-=-
            // public - assignment
            network& operator=(
                    const network& _rhs ) {
                if ( &_rhs == this ) {
                    return *this;
                }

                plugin_base::operator=( _rhs );

                return *this;

            } // operator=

    }; // class network

}; // namespace irods


#endif // ___IRODS_NETWORK_PLUGIN_HPP__



