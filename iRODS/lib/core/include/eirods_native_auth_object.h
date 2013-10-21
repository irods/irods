/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __NATIVE_AUTH_OBJECT_H__
#define __NATIVE_AUTH_OBJECT_H__

#include "eirods_error.h"
#include "eirods_auth_object.h"

#include "rcConnect.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace eirods {

    /// =-=-=-=-=-=-=-
    /// @brief constant defining the native auth scheme string
    const std::string AUTH_NATIVE_SCHEME( "native" );

    /// =-=-=-=-=-=-=-
    /// @brief object for a native irods authenticaion sceheme
    class native_auth_object : public auth_object {
    public:
        /// =-=-=-=-=-=-=-
        /// @brief Ctor
        native_auth_object(rError_t* _r_error);
        virtual ~native_auth_object();

        /// =-=-=-=-=-=-=-
        /// @brief assignment operator
        virtual native_auth_object&  operator=(const native_auth_object& );

        /// =-=-=-=-=-=-=-
        /// @brief Comparison operator
        virtual bool operator==(const native_auth_object& ) const;

        /// =-=-=-=-=-=-=-
        /// @brief Plugin resolution operation
        virtual error resolve(
                          const std::string&, // interface for which to resolve
                          plugin_ptr& );      // ptr to resolved plugin
  
        /// =-=-=-=-=-=-=-
        /// @brief serialize object to key-value pairs
        virtual error get_re_vars(keyValPair_t&); 

        /// =-=-=-=-=-=-=-
        /// @brief auth results accessor
        std::string auth_results() const { return auth_results_; }
         
        private:
        /// =-=-=-=-=-=-=-
        /// @brief results from the auth request
        std::string auth_results_;
            
    }; // class native_auth_object

    /// @brief Helpful typedef
    typedef boost::shared_ptr<native_auth_object> native_auth_object_ptr;
    
}; // namespace eirods

#endif // __NATIVE_AUTH_OBJECT_H__
