


#ifndef __EIRODS_TCP_OBJECT_H__
#define __EIRODS_TCP_OBJECT_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_network_object.h"

namespace eirods {
    // =-=-=-=-=-=-=-
    // TCP Network Object
    class tcp_object : public network_object {
    public:
        // =-=-=-=-=-=-=-
        // Constructors
        tcp_object();
        tcp_object( const rcComm_t& );
        tcp_object( const rsComm_t& );
        tcp_object( const tcp_object& );

        // =-=-=-=-=-=-=-
        // Destructors
        virtual ~tcp_object();

        // =-=-=-=-=-=-=-
        // Operators
        virtual tcp_object& operator=( const tcp_object& );

        /// @brief Comparison operator
        virtual bool operator==( const tcp_object& _rhs ) const;
        
        // =-=-=-=-=-=-=-
        // plugin resolution operation
        virtual error resolve( resource_manager&, resource_ptr& );
        virtual error resolve( network_manager&,  network_ptr&  );
        
        // =-=-=-=-=-=-=-
        // accessor for rule engine variables
        virtual error get_re_vars( keyValPair_t& );

        // =-=-=-=-=-=-=-
        // Accessors

    private:
        
    }; // class tcp_object

}; // namespace eirods

#endif // __EIRODS_TCP_OBJECT_H__
