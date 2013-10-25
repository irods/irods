/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _auth_object_H_
#define _auth_object_H_

#include "eirods_error.h"
#include "eirods_first_class_object.h"

#include "rcConnect.h"

// boost includes
#include <boost/shared_ptr.hpp>

namespace eirods {

    // =-=-=-=-=-=-=-
    // constants for kvp string encoding
    const std::string AUTH_USER_KEY( "auth_user" );
    const std::string AUTH_SCHEME_KEY( "auth_scheme" );
/**
 * @brief Class for representing authorization scheme objects
 */
    class auth_object : public first_class_object {
    public:
        /// @brief Ctor
        auth_object(rError_t* _r_error);
        auth_object( const auth_object& );
        virtual ~auth_object();

        /// @brief assignment operator
        virtual auth_object&  operator=(const auth_object& _rhs);

        /// @brief Accessor for the rError pointer
        virtual rError_t* r_error(void) const { return r_error_; }
        
        /// @brief Comparison operator
        virtual bool operator==(const auth_object& _rhs) const;

        /// @brief Plugin resolution operation
        virtual error resolve(const std::string& _plugin_name, plugin_ptr& _plugin) = 0;
        virtual error get_re_vars(keyValPair_t&) = 0; 
        
    protected:
        rError_t*   r_error_;
    };

    /// @brief Helpful typedef
    typedef boost::shared_ptr<auth_object> auth_object_ptr;
    
}; // namespace eirods

#endif // _auth_object_H_
