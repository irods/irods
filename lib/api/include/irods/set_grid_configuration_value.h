#ifndef IRODS_SET_GRID_CONFIGURATION_VALUE_H
#define IRODS_SET_GRID_CONFIGURATION_VALUE_H

/// \file

#include "irods/plugins/api/grid_configuration_types.h"

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Sets the option_value associated with the (namespace, option_name) tuple in the
/// R_GRID_CONFIGURATION table.
///
/// \param[in]  _comm  A pointer to a RcComm.
/// \param[in]  _input An object holding the tuple and new option value.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.3.0
int rc_set_grid_configuration_value(struct RcComm* _comm, const struct GridConfigurationInput* _input);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_SET_GRID_CONFIGURATION_VALUE_H

