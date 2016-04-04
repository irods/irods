#ifndef __IRODS_NETWORK_OBJECT_HPP__
#define __IRODS_NETWORK_OBJECT_HPP__

// =-=-=-=-=-=-=-
#include "irods_first_class_object.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {
// =-=-=-=-=-=-=-
// network object base class
    class network_object : public first_class_object {
        public:
            // =-=-=-=-=-=-=-
            // Constructors
            network_object();
            network_object( const rcComm_t& );
            network_object( const rsComm_t& );
            network_object( const network_object& );

            // =-=-=-=-=-=-=-
            // Destructors
            virtual ~network_object();

            // =-=-=-=-=-=-=-
            // Operators
            virtual network_object& operator=( const network_object& );

            // =-=-=-=-=-=-=-
            /// @brief Comparison operator
            virtual bool operator==( const network_object& _rhs ) const;

            // =-=-=-=-=-=-=-
            // plugin resolution operation
            virtual error resolve(
                const std::string&, // plugin interface
                plugin_ptr& ) = 0;  // resolved plugin

            // =-=-=-=-=-=-=-
            // convertion to client comm ptr
            virtual error to_client( rcComm_t* );

            // =-=-=-=-=-=-=-
            // convertion to client comm ptr
            virtual error to_server( rsComm_t* );

            // =-=-=-=-=-=-=-
            // accessor for rule engine variables
            virtual error get_re_vars( rule_engine_vars_t& );

            // =-=-=-=-=-=-=-
            // Accessors
            virtual int socket_handle() const {
                return socket_handle_;
            }

            // =-=-=-=-=-=-=-
            // Mutators
            virtual void socket_handle( int _s ) {
                socket_handle_ = _s;
            }

        private:
            // =-=-=-=-=-=-=-
            // Attributes
            int socket_handle_; // socket descriptor

    }; // network_object

// =-=-=-=-=-=-=-
// helpful typedef for sock comm interface & factory
    typedef boost::shared_ptr< network_object > network_object_ptr;

}; // namespace irods

#endif // __IRODS_NETWORK_OBJECT_HPP__



