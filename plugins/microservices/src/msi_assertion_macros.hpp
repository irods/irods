#ifndef IRODS_MSI_ASSERTION_MACROS_HPP
#define IRODS_MSI_ASSERTION_MACROS_HPP

#include "irods/irods_logger.hpp"

using log_msi_test_internal = irods::experimental::log::microservice;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_TEST_BEGIN(test_title) \
  try {                                  \
    log_msi_test_internal::info(">>> TEST BEGIN: [" #test_title "]");

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_TEST_END                                                       \
    log_msi_test_internal::info("--- TEST PASSED ---");                          \
    return 0;                                                                    \
  }                                                                              \
  catch (const std::exception& e) {                                              \
    log_msi_test_internal::error(e.what());                                      \
  }                                                                              \
  catch (...) {                                                                  \
    log_msi_test_internal::error("Caught an unknown exception during testing."); \
  }                                                                              \
                                                                                 \
  log_msi_test_internal::error("--- TEST FAILED ---");                           \
  return 1;
// clang-format on

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_ASSERT(x)                                                            \
  if (x) {                                                                             \
    log_msi_test_internal::info("ASSERTION PASSED [{}:{}]: " #x, __FILE__, __LINE__);  \
  }                                                                                    \
  else {                                                                               \
    log_msi_test_internal::error("ASSERTION FAILED [{}:{}]: " #x, __FILE__, __LINE__); \
    return 1;                                                                          \
  }

#endif // IRODS_MSI_ASSERTION_MACROS_HPP
