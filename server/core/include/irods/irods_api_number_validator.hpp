#ifndef IRODS_API_NUMBER_VALIDATOR_HPP
#define IRODS_API_NUMBER_VALIDATOR_HPP

#include <tuple>

namespace irods {

std::tuple<bool, int> is_api_number_supported(int _api_number);

} // namespace irods

#endif // IRODS_API_NUMBER_VALIDATOR_HPP
