


// =-=-=-=-=-=-=-
// irods includes
#include "eirods_virtual_path.h"

// =-=-=-=-=-=-=-
// NOTE :: eventually move all knowledge of path management
//      :: policy, validation etc here
namespace eirods {
    /// =-=-=-=-=-=-=-
    /// @brief interface to get virtual path separator
    std::string get_virtual_path_separator() {
        const std::string PATH_SEPARATOR( "/" );
        return PATH_SEPARATOR;
    }

}; // namespace eirods





