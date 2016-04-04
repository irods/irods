#ifndef ___IRODS_RESC_PLUGIN_HPP__
#define ___IRODS_RESC_PLUGIN_HPP__

// =-=-=-=-=-=-=-
#include "irods_plugin_base.hpp"
#include "irods_resource_constants.hpp"
#include "irods_resource_types.hpp"

#include <iostream>
#include <utility>
#include <boost/any.hpp>

namespace irods {

    const std::string RESC_CHILD_MAP_PROP( "resource_child_map_property" );
    const std::string RESC_PARENT_PROP( "resource_parent_property" );
    typedef lookup_table< std::pair< std::string, resource_ptr > > resource_child_map;

// =-=-=-=-=-=-=-
    /**
     * \author Jason M. Coposky
     * \brief
     *
     **/
    class resource : public plugin_base {
        public:
            // =-=-=-=-=-=-=-
            /// @brief Constructors
            resource(
                const std::string& _inst,
                const std::string& _ctx ) :
                    plugin_base(
                        _inst,
                        _ctx ) {
                properties_.set(
                    RESC_CHILD_MAP_PROP,
                    &children_ );
                properties_.set(
                    RESC_PARENT_PROP,
                    parent_ );
            } // ctor

            // =-=-=-=-=-=-=-
            /// @brief Destructor
            virtual ~resource() {
            }

            // =-=-=-=-=-=-=-
            /// @brief copy ctor
            resource( const resource& _rhs ) :
                plugin_base( _rhs ) {
                children_ = _rhs.children_;
                parent_   = _rhs.parent_;
            } // cctor

            // =-=-=-=-=-=-=-
            /// @brief Assignment Operator - necessary for stl containers
            resource& operator=( const resource& _rhs ) {
                if ( &_rhs == this ) {
                    return *this;
                }
                plugin_base::operator=( _rhs );
                children_ = _rhs.children_;
                parent_   = _rhs.parent_;
                return *this;
            }

            // =-=-=-=-=-=-=-
            /// @brief interface to add and remove children using the zone_name::resource_name
            virtual error add_child( const std::string&, const std::string&, resource_ptr );
            virtual error remove_child( const std::string& );
            virtual size_t num_children() {
                return children_.size();
            }
            virtual bool has_child(
                const std::string& _name ) {
                return children_.has_entry( _name );
            }
            virtual void children( std::vector<std::string>& );

            // =-=-=-=-=-=-=-
            /// @brief interface to get and set a resource's parent pointer
            virtual error set_parent( const resource_ptr& );
            virtual error get_parent( resource_ptr& );

        protected:
            // =-=-=-=-=-=-=-
            /// @brief Pointers to Child and Parent Resources
            resource_child_map  children_;
            resource_ptr        parent_;

    }; // class resource

}; // namespace irods


#endif // ___IRODS_RESC_PLUGIN_HPP__



