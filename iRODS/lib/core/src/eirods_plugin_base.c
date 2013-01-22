


// =-=-=-=-=-=-=-
// e-irods includes
#include "eirods_plugin_base.h"

namespace eirods {

    // =-=-=-=-=-=-=-
    // public - constructor 
    plugin_base::plugin_base( std::string _c ) : context_(_c){
    } // ctor

    // =-=-=-=-=-=-=-
    // public - destructor 
    plugin_base::~plugin_base(  ) {
    } // dtor

    // =-=-=-=-=-=-=-
    // public - default implementation
    error plugin_base::post_disconnect_maintenance_operation( pdmo_base*& _op ) {
       _op = 0;
       return ERROR( -1, "no defined operation" );
        
    } // post_disconnect_maintenance_operation

}; // namespace eirods



