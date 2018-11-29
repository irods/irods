#include "irods_api_number_validator.hpp"

#include "procApiRequest.h"
#include "apiNumber.h"
#include "rodsErrorTable.h"

#include <tuple>
#include <vector>
#include <algorithm>

namespace {

std::vector<int> make_unsupported_api_number_list()
{
    std::vector api_numbers{
        GET_XMSG_TICKET_AN,
        SEND_XMSG_AN,
        RCV_XMSG_AN
    };

    std::sort(std::begin(api_numbers), std::end(api_numbers));

    return api_numbers;
}

} // anonymous namespace

namespace irods {

std::tuple<bool, int> is_api_number_supported(int _api_number)
{
    if (auto ec = apiTableLookup(_api_number); SYS_UNMATCHED_API_NUM == ec) {
        return {false, ec};
    }

    static const auto unsupported_api_numbers = make_unsupported_api_number_list();

    auto b = std::cbegin(unsupported_api_numbers);
    auto e = std::cend(unsupported_api_numbers);

    if (std::binary_search(b, e, _api_number)) {
        return {false, SYS_NOT_SUPPORTED};
    }

    return {true, 0};
}

} // namespace irods

