#include "irods_api_number_validator.hpp"

#include "procApiRequest.h"
#include "rodsErrorTable.h"

#include <tuple>

namespace irods {

std::tuple<bool, int> is_api_number_supported(int _api_number)
{
    if (auto ec = apiTableLookup(_api_number); SYS_UNMATCHED_API_NUM == ec) {
        return {false, ec};
    }

    return {true, 0};
}

} // namespace irods

