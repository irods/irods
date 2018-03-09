#ifndef _GENERIC_AUTH_OBJECT_HPP_
#define _GENERIC_AUTH_OBJECT_HPP_

#include "irods_error.hpp"
#include "irods_auth_object.hpp"
#include "irods_stacktrace.hpp"

#include "rcMisc.h"

namespace irods {

    /**
     * @brief Auth object for generic authentication
     */
    class generic_auth_object : public auth_object {
        public:
            /// @brief Constructor
            generic_auth_object( const std::string& _type, rError_t* _r_error );
            generic_auth_object( const generic_auth_object& _rhs ); virtual ~generic_auth_object();

            /// @brief Plugin resolution operator
            virtual error resolve( const std::string& _name, plugin_ptr& _plugin ); // resolve plugin

            /// @brief Comparison operator
            virtual bool operator==( const generic_auth_object& _rhs ) const;
            
            /// @brief Assignment operator
            virtual generic_auth_object& operator=( const generic_auth_object& _rhs ); 
            
            /// @brief serialize to key-value pairs
            virtual error get_re_vars( rule_engine_vars_t& );
            
            /// @brief get the socket number
            virtual int sock( void ) const {
                return sock_;
            }

            /// @brief set the socket number
            virtual void sock( int s ) {
                sock_ = s;
            }

            
        private:
            std::string type_;
            int sock_;

    };

    typedef boost::shared_ptr<generic_auth_object> generic_auth_object_ptr;

}; // namespace irods

#endif // _GENERIC_AUTH_OBJECT_HPP_
