#ifndef _AUTH_OBJECT_HPP_
#define _AUTH_OBJECT_HPP_

#include "irods_error.hpp"
#include "irods_first_class_object.hpp"

#include "rcConnect.h"

// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {
    /**
     * @brief Class for representing authorization scheme objects
     */
    class auth_object : public first_class_object {
        public:
            /// @brief Ctor
            auth_object( rError_t* _r_error );
            auth_object( const auth_object& );
            virtual ~auth_object();

            /// @brief assignment operator
            virtual auth_object&  operator=( const auth_object& _rhs );

            /// @brief Accessor for the rError pointer
            virtual rError_t* r_error( void ) const {
                return r_error_;
            }
            virtual const std::string& request_result() const {
                return request_result_;
            }
            virtual void request_result( const std::string& _r ) {
                request_result_ = _r;
            }
            virtual const std::string& context() const {
                return context_;
            }
            virtual void context( const std::string& _c ) {
                context_ = _c;
            }
            virtual const std::string& user_name() const {
                return user_name_;
            }
            virtual const std::string& zone_name() const {
                return zone_name_;
            }
            virtual void user_name( const std::string& _un ) {
                user_name_ = _un;
            }
            virtual void zone_name( const std::string& _zn ) {
                zone_name_ = _zn;
            }

            /// @brief Comparison operator
            virtual bool operator==( const auth_object& _rhs ) const;

            /// @brief Plugin resolution operation
            virtual error resolve( const std::string& _plugin_name, plugin_ptr& _plugin ) = 0;
            virtual error get_re_vars( rule_engine_vars_t& ) = 0;

        protected:
            rError_t*   r_error_;

            /// =-=-=-=-=-=-=-
            // result passed to outgoing auth request
            // struct back to client - challenge for native,
            // password for pam etc
            std::string request_result_;

            /// =-=-=-=-=-=-=-
            /// @brief user name - from rcConn
            std::string user_name_;

            /// =-=-=-=-=-=-=-
            /// @brief zone name - from rcConn
            std::string zone_name_;

            /// =-=-=-=-=-=-=-
            /// @brief context string which might hold anything
            ///        coming from a client call
            std::string context_;
    };

/// @brief Helpful typedef
    typedef boost::shared_ptr<auth_object> auth_object_ptr;

}; // namespace irods

#endif // _AUTH_OBJECT_HPP_
