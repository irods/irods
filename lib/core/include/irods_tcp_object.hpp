#ifndef __IRODS_TCP_OBJECT_HPP__
#define __IRODS_TCP_OBJECT_HPP__

// =-=-=-=-=-=-=-
#include "irods_network_object.hpp"

namespace irods {
// =-=-=-=-=-=-=-
// constant key for tcp network object
    const std::string TCP_NETWORK_PLUGIN( "tcp" );

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
            ~tcp_object() override;

            // =-=-=-=-=-=-=-
            // Operators
            virtual tcp_object& operator=( const tcp_object& );

            /// @brief Comparison operator
            virtual bool operator==( const tcp_object& _rhs ) const;

            // =-=-=-=-=-=-=-
            // plugin resolution operation
            error resolve(
                const std::string&, // plugin interface
                plugin_ptr& ) override;      // resolved plugin instance

            // =-=-=-=-=-=-=-
            // accessor for rule engine variables
            error get_re_vars( rule_engine_vars_t& ) override;

            // =-=-=-=-=-=-=-
            // Accessors

        private:

    }; // class tcp_object

/// =-=-=-=-=-=-=-
/// @brief typedef for shared tcp object ptr
    typedef boost::shared_ptr< tcp_object > tcp_object_ptr;

}; // namespace irods

#endif // __IRODS_TCP_OBJECT_HPP__
