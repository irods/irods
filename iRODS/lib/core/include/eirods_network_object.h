


#ifndef __EIRODS_NETWORK_OBJECT_H__
#define __EIRODS_NETWORK_OBJECT_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_first_class_object.h"

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

namespace eirods {
    // =-=-=-=-=-=-=-
    // network object base class
    class network_object : public first_class_object {
    public:
        // =-=-=-=-=-=-=-
        // Constructors
        network_object();
        network_object( const rsComm_t& );
        network_object( const network_object& );

        // =-=-=-=-=-=-=-
        // Destructors
        virtual ~network_object();

        // =-=-=-=-=-=-=-
        // Operators
        virtual network_object& operator=( const network_object& );

        /// @brief Comparison operator
        virtual bool operator==( const network_object& _rhs ) const;
        
        // =-=-=-=-=-=-=-
        // plugin resolution operation
        virtual error resolve( resource_manager&, resource_ptr& ) = 0;
        virtual error resolve( network_manager&,  network_ptr&  ) = 0;
        
        // =-=-=-=-=-=-=-
        // accessor for rule engine variables
        virtual error get_re_vars( keyValPair_t& );

        // =-=-=-=-=-=-=-
        // Accessors
        virtual int socket_handle() const { return socket_handle_; }

    private:
        // =-=-=-=-=-=-=-
        // Attributes 
        int socket_handle_; // socket descriptor

    }; // network_object

}; // namespace eirods

#endif // __EIRODS_NETWORK_OBJECT_H__



