#include "irods/private/irods_object_oper.hpp"

namespace irods {

    object_oper::object_oper(
        const file_object& _file_object,
        const std::string& _operation ) :
        file_object_( _file_object ),
        operation_( _operation ) {
    }

    object_oper::~object_oper( void ) {
        // TODO - stub
    }

}; // namespace irods
