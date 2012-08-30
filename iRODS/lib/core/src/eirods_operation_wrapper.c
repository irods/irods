


// =-=-=-=-=-=-=-
// My Includes
#include "eirods_operation_wrapper.h"
#include "eirods_load_plugin.h"
#include "eirods_ms_home.h"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>

// =-=-=-=-=-=-=-
// Boost Includes

namespace eirods {

    // =-=-=-=-=-=-=-
	// public - ctor
    operation_wrapper::operation_wrapper( ) : operation_( 0 ) {
	} // default ctor

    // =-=-=-=-=-=-=-
	// public - ctor with opreation
    operation_wrapper::operation_wrapper( resource_operation _op ) : operation_( _op ) {
	} // ctor

    // =-=-=-=-=-=-=-
	// public - cctor
    operation_wrapper::operation_wrapper( const operation_wrapper& _rhs ) {
		operation_ = _rhs.operation_;
	} // cctor
 
    // =-=-=-=-=-=-=-
	// public - dtor
    operation_wrapper::~operation_wrapper( ) {
	} // dtor
   
    // =-=-=-=-=-=-=-
	// public - assignment for stl container
	operation_wrapper& operation_wrapper::operator=( const operation_wrapper& _rhs ) {
		operation_ = _rhs.operation_;
		return *this;
	} // operator=
    
	// END operation_wrapper
    // =-=-=-=-=-=-=-

}; // namespace eirods



