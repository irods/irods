#ifndef _AUTH_HPP_
#define _AUTH_HPP_

#include "irods_error.hpp"
#include "irods_auth_types.hpp"
#include "irods_load_plugin.hpp"
#include "dlfcn.h"

#include <utility>
#include <boost/any.hpp>

namespace irods {

    /**
     * @brief Base class for auth plugins
     */
    class auth : public plugin_base {
        public:
            auth(
                const std::string& _inst,
                const std::string& _ctx ) :
                plugin_base( _inst, _ctx ) {

            }

            virtual ~auth() {
            }

            auth(
                const auth& _rhs ) :
                plugin_base( _rhs ) {
            }

            auth& operator=(
                const auth& _rhs ) {
                if ( &_rhs == this ) {
                    return *this;
                }

                plugin_base::operator=( _rhs );

                return *this;
            }
    };

}; // namespace irods

#endif // _AUTH_HPP_
