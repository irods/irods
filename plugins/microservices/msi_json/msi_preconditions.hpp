#ifndef IRODS_MSI_PRECONDITIONS_HPP
#define IRODS_MSI_PRECONDITIONS_HPP

/// \file
///
/// This file is only for microservices!
/// It MUST NOT be used outside of microservice-based code.

#include "rodsErrorTable.h"
#include "rodsLog.h"

#include <algorithm>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_REQUIRE_VALID_POINTER(ptr)                                                          \
  if (!(ptr)) {                                                                                       \
    rodsLog(LOG_ERROR, "Invalid argument: Expected valid pointer for [" #ptr "]. Received nullptr."); \
    return SYS_INVALID_INPUT_PARAM;                                                                   \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_REQUIRE_TYPE(ptr, ...)                                                                               \
  {                                                                                                                    \
    const auto types = {__VA_ARGS__};                                                                                  \
    const auto pred = [p = ptr](auto&& _type) { return (std::strcmp(p, _type) == 0); };                                \
    if (std::none_of(std::begin(types), std::end(types), pred)) {                                                      \
      rodsLog(LOG_ERROR, "Invalid argument: Type for " #ptr " must be one of [%s]. Received [%s]", #__VA_ARGS__, ptr); \
      return SYS_INVALID_INPUT_PARAM;                                                                                  \
    }                                                                                                                  \
  }

#endif // IRODS_MSI_PRECONDITIONS_HPP
