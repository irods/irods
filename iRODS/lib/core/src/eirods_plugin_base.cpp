


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_plugin_base.hpp"

extern "C" {
    /// =-=-=-=-=-=-=-
    /// @brief definition of plugin interface version
    double get_plugin_interface_version() {
        static const double EIRODS_PLUGIN_INTERFACE_VERSION = 1.0;
        return EIRODS_PLUGIN_INTERFACE_VERSION;
    }
}

namespace eirods {
    // =-=-=-=-=-=-=-
    // public - constructor
    plugin_base::plugin_base( 
        const std::string& _n,
        const std::string& _c ) :
        context_( _c ),
        instance_name_( _n ),
        interface_version_( get_plugin_interface_version() ) {
    } // ctor

    // =-=-=-=-=-=-=-
    // public - copy constructor
    plugin_base::plugin_base( 
        const plugin_base& _rhs ) :
        context_( _rhs.context_ ),
        instance_name_( _rhs.instance_name_ ),
        interface_version_( _rhs.interface_version_ ) {
    } // cctor

    // =-=-=-=-=-=-=-
    // public - assignment operator
    plugin_base& plugin_base::operator=( 
        const plugin_base& _rhs ) {
        instance_name_     = _rhs.instance_name_;
        context_           = _rhs.context_;
        interface_version_ = _rhs.interface_version_;

        return *this;

    } // operator=

    // =-=-=-=-=-=-=-
    // public - destructor
    plugin_base::~plugin_base(  ) {
    } // dtor

    // =-=-=-=-=-=-=-
    // public - default implementation
    error plugin_base::post_disconnect_maintenance_operation( pdmo_type& _op ) {
       return ERROR( EIRODS_NO_PDMO_DEFINED, "no defined operation" );

    } // post_disconnect_maintenance_operation

    // =-=-=-=-=-=-=-
    // public - interface to determine if a PDMO is necessary
    error plugin_base::need_post_disconnect_maintenance_operation( bool& _b ) {
        _b = false;
        return SUCCESS();

    } // need_post_disconnect_maintenance_operation


    // =-=-=-=-=-=-=-
    // public - add an operation to the map given a key.  provide the function name such that it
    //          may be loaded from the shared object later via delay_load
    error plugin_base::add_operation( std::string _op,
                                      std::string _fcn_name ) {
        // =-=-=-=-=-=-=-
        // check params
        if( _op.empty() ) {
            std::stringstream msg;
            msg << "empty operation [" << _op << "]";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

        if( _fcn_name.empty() ) {
            std::stringstream msg;
            msg << "empty function name [" << _fcn_name << "]";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // add operation string to the vector
        ops_for_delay_load_.push_back( std::pair< std::string, std::string >( _op, _fcn_name ) );

        return SUCCESS();

    } //  add_operation

    // =-=-=-=-=-=-=-
    // public - get a list of all the available operations
    error plugin_base::enumerate_operations( std::vector< std::string >& _ops ) {
       for( size_t i = 0; i < ops_for_delay_load_.size(); ++i ) {
           _ops.push_back( ops_for_delay_load_[ i ].first );
       }

       return SUCCESS();

    } // enumerate_operations

}; // namespace eirods



