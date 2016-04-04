#include "irods_get_l1desc.hpp"
#include "rsGlobalExtern.hpp"

namespace irods {

    l1desc_t& get_l1desc( int _idx ) {
        return L1desc[ _idx ];
    }

}; // namespace irods

