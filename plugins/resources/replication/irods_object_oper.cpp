#include "irods_object_oper.hpp"

namespace irods {

    object_oper::object_oper(
        const file_object& _file_object,
        std::string  _operation ) :
        file_object_( _file_object ),
        operation_(std::move( _operation )) {
    }

    object_oper::~object_oper( ) {
        // TODO - stub
    }

}; // namespace irods
