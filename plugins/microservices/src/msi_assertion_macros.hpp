#ifndef IRODS_MSI_ASSERTION_MACROS_HPP
#define IRODS_MSI_ASSERTION_MACROS_HPP

/// \file
///
/// This file is only for testing purposes!
/// It MUST NOT be used outside of microservice-based unit tests.

#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_logger.hpp"

#include <cstring>

using log_msi_test_internal = irods::experimental::log::microservice;

// clang-format off

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_TEST_CASE(func, rei) \
  if (func(rei)) {                     \
      return -1;                       \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_TEST_BEGIN(test_title)                              \
  int msi_test_error_code = 0;                                        \
                                                                      \
  irods::at_scope_exit print_pass_or_failed{[&msi_test_error_code] {  \
    if (msi_test_error_code) {                                        \
      log_msi_test_internal::info("--- TEST FAILED ---");             \
    }                                                                 \
    else {                                                            \
      log_msi_test_internal::info("--- TEST PASSED ---");             \
    }                                                                 \
  }};                                                                 \
                                                                      \
  try {                                                               \
    log_msi_test_internal::info(">>> TEST BEGIN: [" #test_title "]");

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_TEST_END                                                      \
    return msi_test_error_code;                                                 \
  }                                                                             \
  catch (const std::exception& e) {                                             \
    log_msi_test_internal::info(e.what());                                      \
    return msi_test_error_code = -1;                                            \
  }                                                                             \
  catch (...) {                                                                 \
    log_msi_test_internal::info("Caught an unknown exception during testing."); \
    return msi_test_error_code = -1;                                            \
  }                                                                             \

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_ASSERT(expr)                                                              \
  if (expr) {                                                                               \
    log_msi_test_internal::info("ASSERTION PASSED [{}:{}]: {}", __FILE__, __LINE__, #expr); \
  }                                                                                         \
  else {                                                                                    \
    log_msi_test_internal::info("ASSERTION FAILED [{}:{}]: {}", __FILE__, __LINE__, #expr); \
    return msi_test_error_code = -1;                                                        \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_THROWS(expr, ex_type)                                                     \
  try {                                                                                     \
    expr;                                                                                   \
    log_msi_test_internal::info("ASSERTION FAILED [{}:{}]: {}", __FILE__, __LINE__, #expr); \
    return msi_test_error_code = -1;                                                        \
  }                                                                                         \
  catch (const ex_type&) {                                                                  \
    log_msi_test_internal::info("ASSERTION PASSED [{}:{}]: {}", __FILE__, __LINE__, #expr); \
  }                                                                                         \
  catch (...) {                                                                             \
    log_msi_test_internal::info("ASSERTION FAILED [{}:{}]: {}", __FILE__, __LINE__, #expr); \
    return msi_test_error_code = -1;                                                        \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_THROWS_MSG(expr, ex_type, ex_msg)                                           \
  try {                                                                                       \
    expr;                                                                                     \
    log_msi_test_internal::info("ASSERTION FAILED [{}:{}]: {}", __FILE__, __LINE__, #expr);   \
    return msi_test_error_code = -1;                                                          \
  }                                                                                           \
  catch (const ex_type& e) {                                                                  \
    if (!std::strstr(e.what(), ex_msg)) {                                                     \
      log_msi_test_internal::info("ASSERTION FAILED [{}:{}]: {}", __FILE__, __LINE__, #expr); \
      return msi_test_error_code = -1;                                                        \
    }                                                                                         \
    log_msi_test_internal::info("ASSERTION PASSED [{}:{}]: {}", __FILE__, __LINE__, #expr);   \
  }                                                                                           \
  catch (...) {                                                                               \
    log_msi_test_internal::info("ASSERTION FAILED [{}:{}]: {}", __FILE__, __LINE__, #expr);   \
    return msi_test_error_code = -1;                                                          \
  }

// clang-format on

#endif // IRODS_MSI_ASSERTION_MACROS_HPP
