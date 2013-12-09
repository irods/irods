/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef _gsi_object_H_
#define _gsi_object_H_

#include "irods_error.hpp"

#include <gssapi.h>

namespace irods {

// constant key for gsi auth object
    const std::string GSI_AUTH_PLUGIN( "GSI" );

    /**
     * @brief Auth object for GSI authentication
     */
    class gsi_object : public auth_object {
    public:
        /// @brief Constructor
        gsi_object( rError_t* _r_error );
        virtual ~gsi_object();

        // Accessors

        /// @brief Returns the GSI credentials
        virtual gss_cred_id_t creds( void ) const {
            return creds_;
        }

        // Mutators

        /// @brief Sets the GSI credentials
        virtual void creds( gss_cred_id_t _creds ) {
            creds_ = _creds;
        }

        // Methods

        /// @brief undocumented
        error resolve( const std::string& _name, plugin_ptr& _plugin ); // resolve plugin

        /// @brief Comparison operator
        bool operator==( const gsi_object& _rhs ) const;


    private:
        gss_cred_id_t creds_;
    };

    typedef boost::shared_ptr<gsi_object> gsi_object_ptr;

}; // namespace irods

#endif // _gsi_object_H_
