#ifndef IRODS_MSI_ASSERTION_MACROS_HPP
#define IRODS_MSI_ASSERTION_MACROS_HPP

#include "rodsLog.h"

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_TEST_BEGIN(test_title) \
  try {                                  \
    rodsLog(LOG_NOTICE, ">>> TEST BEGIN: [" #test_title "]");

// clang-format off
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_TEST_END                                             \
    rodsLog(LOG_NOTICE, "--- TEST PASSED ---");                        \
    return 0;                                                          \
  }                                                                    \
  catch (const std::exception& e) {                                    \
    rodsLog(LOG_ERROR, e.what());                                      \
  }                                                                    \
  catch (...) {                                                        \
    rodsLog(LOG_ERROR, "Caught an unknown exception during testing."); \
  }                                                                    \
                                                                       \
  rodsLog(LOG_ERROR, "--- TEST FAILED ---");                           \
  return 1;
// clang-format on

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IRODS_MSI_ASSERT(x)                                                    \
  if (x) {                                                                     \
    rodsLog(LOG_NOTICE, "ASSERTION PASSED [%s:%d]: " #x, __FILE__, __LINE__);  \
  }                                                                            \
  else {                                                                       \
    rodsLog(LOG_ERROR, "ASSERTION FAILED [%s:%d]: " #x, __FILE__, __LINE__);   \
    return 1;                                                                  \
  }

#endif // IRODS_MSI_ASSERTION_MACROS_HPP
