#ifndef IRODS_MSI_ASSERTION_MACROS_HPP
#define IRODS_MSI_ASSERTION_MACROS_HPP

#include "irods_at_scope_exit.hpp"
#include "rodsLog.h"

#include <cstring>

#define IRODS_MSI_TEST_CASE(func) \
  if (func()) { return -1; }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_TEST_BEGIN(test_title)                             \
  int msi_test_error_code = 0;                                       \
                                                                     \
  irods::at_scope_exit print_pass_or_failed{[&msi_test_error_code] { \
    if (msi_test_error_code) {                                       \
      rodsLog(LOG_NOTICE, "--- TEST FAILED ---");                    \
    }                                                                \
    else {                                                           \
      rodsLog(LOG_NOTICE, "--- TEST PASSED ---");                    \
    }                                                                \
  }};                                                                \
                                                                     \
  try {                                                              \
    rodsLog(LOG_NOTICE, ">>> TEST BEGIN: [" #test_title "]");

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_TEST_END                                              \
    return msi_test_error_code;                                         \
  }                                                                     \
  catch (const std::exception& e) {                                     \
    rodsLog(LOG_NOTICE, e.what());                                      \
    return msi_test_error_code = -1;                                    \
  }                                                                     \
  catch (...) {                                                         \
    rodsLog(LOG_NOTICE, "Caught an unknown exception during testing."); \
    return msi_test_error_code = -1;                                    \
  }                                                                     \
// clang-format on

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_ASSERT(x)                                                    \
  if (x) {                                                                     \
    rodsLog(LOG_NOTICE, "ASSERTION PASSED [%s:%d]: " #x, __FILE__, __LINE__);  \
  }                                                                            \
  else {                                                                       \
    rodsLog(LOG_NOTICE, "ASSERTION FAILED [%s:%d]: " #x, __FILE__, __LINE__);  \
    return msi_test_error_code = -1;                                           \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_THROWS(expr, ex_type)                                          \
  try {                                                                          \
    expr;                                                                        \
    rodsLog(LOG_NOTICE, "ASSERTION FAILED [%s:%d]: " #expr, __FILE__, __LINE__); \
    return msi_test_error_code = -1;                                             \
  }                                                                              \
  catch (const ex_type&) {                                                       \
    rodsLog(LOG_NOTICE, "ASSERTION PASSED [%s:%d]: " #expr, __FILE__, __LINE__); \
  }                                                                              \
  catch (...) {                                                                  \
    rodsLog(LOG_NOTICE, "ASSERTION FAILED [%s:%d]: " #expr, __FILE__, __LINE__); \
    return msi_test_error_code = -1;                                             \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_THROWS_MSG(expr, ex_type, ex_msg)                                \
  try {                                                                            \
    expr;                                                                          \
    rodsLog(LOG_NOTICE, "ASSERTION FAILED [%s:%d]: " #expr, __FILE__, __LINE__);   \
    return msi_test_error_code = -1;                                               \
  }                                                                                \
  catch (const ex_type& e) {                                                       \
    if (!std::strstr(e.what(), ex_msg)) {                                          \
      rodsLog(LOG_NOTICE, "ASSERTION FAILED [%s:%d]: " #expr, __FILE__, __LINE__); \
      return msi_test_error_code = -1;                                             \
    }                                                                              \
    rodsLog(LOG_NOTICE, "ASSERTION PASSED [%s:%d]: " #expr, __FILE__, __LINE__);   \
  }                                                                                \
  catch (...) {                                                                    \
    rodsLog(LOG_NOTICE, "ASSERTION FAILED [%s:%d]: " #expr, __FILE__, __LINE__);   \
    return msi_test_error_code = -1;                                               \
  }

#endif // IRODS_MSI_ASSERTION_MACROS_HPP
