#ifndef IRODS_GET_GRID_CONFIGURATION_VALUE_H
#define IRODS_GET_GRID_CONFIGURATION_VALUE_H

/// \file

#include "irods/plugins/api/grid_configuration_types.h"

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Given a (namespace, option_name) tuple, returns the option_value from the R_GRID_CONFIGURATION table.
///
/// \param[in]  _comm   A pointer to a RcComm.
/// \param[in]  _input  An object holding criteria used to locate a table entry.
/// \param[out] _output An object holding the option value.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.3.0
int rc_get_grid_configuration_value(struct RcComm* _comm,
                                    const struct GridConfigurationInput* _input,
                                    struct GridConfigurationOutput** _output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_GET_GRID_CONFIGURATION_VALUE_H

