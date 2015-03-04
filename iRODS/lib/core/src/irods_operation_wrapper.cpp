// =-=-=-=-=-=-=-
// My Includes
#include "irods_operation_wrapper.hpp"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>

// =-=-=-=-=-=-=-
// Boost Includes

namespace irods {

// =-=-=-=-=-=-=-
// public - ctor
    operation_wrapper::operation_wrapper( ) : operation_( 0 ) {
    } // default ctor

// =-=-=-=-=-=-=-
// public - ctor with opreation
    operation_wrapper::operation_wrapper(
        oper_rule_exec_mgr_ptr _rule_exec,
        const std::string&     _inst_name,
        const std::string&     _op_name,
        plugin_operation _op ) :
        instance_name_( _inst_name ),
        operation_name_( _op_name ),
        operation_( _op ) {
    } // ctor

// =-=-=-=-=-=-=-
// public - cctor
    operation_wrapper::operation_wrapper(
        const operation_wrapper& _rhs ) {
        operation_      = _rhs.operation_;
        instance_name_  = _rhs.instance_name_;
        operation_name_ = _rhs.operation_name_;
    } // cctor

// =-=-=-=-=-=-=-
// public - dtor
    operation_wrapper::~operation_wrapper( ) {
    } // dtor

// =-=-=-=-=-=-=-
// public - assignment for stl container
    operation_wrapper& operation_wrapper::operator=(
        const operation_wrapper& _rhs ) {
        instance_name_  = _rhs.instance_name_;
        operation_name_ = _rhs.operation_name_;
        operation_      = _rhs.operation_;
        return *this;
    } // operator=

// END operation_wrapper
// =-=-=-=-=-=-=-

}; // namespace irods



