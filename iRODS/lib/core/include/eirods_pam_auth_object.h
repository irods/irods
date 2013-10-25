/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __PAM_AUTH_OBJECT_H__
#define __PAM_AUTH_OBJECT_H__

#include "eirods_error.h"
#include "eirods_auth_object.h"

#include "rcConnect.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace eirods {

    /// =-=-=-=-=-=-=-
    /// @brief constant defining the native auth scheme string
    const std::string AUTH_PAM_SCHEME( "pam" );
    
    /// =-=-=-=-=-=-=-
    /// @brief constant defining the username key for a kvp string
    const std::string PAM_USER_KEY( "username" );
    
    /// =-=-=-=-=-=-=-
    /// @brief constant defining the ttl key for a kvp string
    const std::string PAM_TTL_KEY( "ttl" );
    
    /// =-=-=-=-=-=-=-
    /// @brief constant defining the password key for a kvp string
    const std::string PAM_PASSWORD_KEY( "password" );

    /// =-=-=-=-=-=-=-
    /// @brief object for a native irods authenticaion sceheme
    class pam_auth_object : public auth_object {
    public:
        /// =-=-=-=-=-=-=-
        /// @brief Ctor
        pam_auth_object(rError_t* _r_error);
        virtual ~pam_auth_object();
        pam_auth_object( const pam_auth_object& );

        /// =-=-=-=-=-=-=-
        /// @brief assignment operator
        virtual pam_auth_object&  operator=(const pam_auth_object& );

        /// =-=-=-=-=-=-=-
        /// @brief Comparison operator
        virtual bool operator==(const pam_auth_object& ) const;

        /// =-=-=-=-=-=-=-
        /// @brief Plugin resolution operation
        virtual error resolve(
                          const std::string&, // interface for which to resolve
                          plugin_ptr& );      // ptr to resolved plugin
  
        /// =-=-=-=-=-=-=-
        /// @brief serialize object to key-value pairs
        virtual error get_re_vars(keyValPair_t&); 

        /// =-=-=-=-=-=-=-
        /// @brief accessors
        std::string user_name() const { return user_name_; }
        std::string zone_name() const { return zone_name_; }
        
        /// =-=-=-=-=-=-=-
        /// @brief mutators
        void user_name( const std::string& _un ) { user_name_ = _un; }
        void zone_name( const std::string& _zn ) { zone_name_ = _zn; }

        private:
        /// =-=-=-=-=-=-=-
        /// @brief user name - from rcConn
        std::string user_name_;
        
        /// =-=-=-=-=-=-=-
        /// @brief zone name - from rcConn
        std::string zone_name_;

    }; // class pam_auth_object

    /// @brief Helpful typedef
    typedef boost::shared_ptr<pam_auth_object> pam_auth_object_ptr;
    
}; // namespace eirods

#endif // __PAM_AUTH_OBJECT_H__
